#include "byts/Byt.hpp"
#include "byts/World.hpp"
#include <cmath>
#include <algorithm>
#include <random>

namespace byts {

namespace {
inline float len2(sf::Vector2f v) noexcept { return v.x*v.x + v.y*v.y; }
inline sf::Vector2f clamp_speed(sf::Vector2f v, float max_speed) noexcept {
    const float max2 = max_speed * max_speed;
    float L2 = len2(v);
    if (L2 <= max2) return v;
    float inv = max_speed / std::sqrt(L2);
    return { v.x * inv, v.y * inv };
}
} // namespace

void Byt::set_id(std::size_t id) noexcept {
    id_ = id;
    std::seed_seq seed{ static_cast<unsigned>(0xB17), static_cast<unsigned>(id_) };
    rng_.seed(seed);
    std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);
    float a = angle_dist(rng_);
    wander_dir_ = { std::cos(a), std::sin(a) };
}

void Byt::sense_update(const World& world, Seconds dt) {
    sight_timer_   += dt;
    hearing_timer_ += dt;
    smell_timer_   += dt;    

    if (sight_timer_ >= cfg_.sight_cadence) {
        sight_timer_ = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, cfg_.sight_range, SenseMask::Visible, buf);
        seen_.clear();
        seen_.reserve(buf.size());
        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;
            seen_.push_back(Seen{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id });
        }
    }

    if (hearing_timer_ >= cfg_.hearing_cadence) {
        hearing_timer_ = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, cfg_.hearing_range, SenseMask::Audible, buf);
        heard_.clear();
        heard_.reserve(buf.size());
        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;
            heard_.push_back(Heard{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id });
        }
    }

    if (smell_timer_ >= cfg_.smell_cadence) {
        smell_timer_ = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, cfg_.smell_range, SenseMask::Smell, buf);
        smelled_.clear();
        smelled_.reserve(buf.size());
        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;
            smelled_.push_back(Smelled{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id });
        }
    }
    update_food_memory(dt);
}


void Byt::decide_mood() {
    if (brain_.stored_energy <= brain_.hunger_on) {
        mood_ = Mood::Hungry;
    }
    else if (brain_.social_need >= brain_.social_on) {
        mood_ = Mood::Lonely;
    }
    else {
        mood_ = Mood::Neutral;
    }
}

void Byt::decide_behavior() {
    switch (mood_) {
        case Mood::Neutral:  behavior_ = Behavior::Wander;        break;
        case Mood::Hungry:   behavior_ = Behavior::Forage;        break;
        case Mood::Lonely:   behavior_ = Behavior::SeekCompanion; break;
    }
}

void Byt::update_food_memory(Seconds dt) {
    if (foodTargetMemory_.active) {
        foodTargetMemory_.age += dt;
    }

    const Seen* nearest_seen = nullptr;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Food) {
            if (!nearest_seen || s.distance < nearest_seen->distance) {
                nearest_seen = &s;
            }
        }
    }

    if (nearest_seen) {
        foodTargetMemory_.active = true;
        foodTargetMemory_.id = nearest_seen->id;
        foodTargetMemory_.pos = nearest_seen->pos;
        foodTargetMemory_.age = 0.f;
        return;
    }

    const Smelled* nearest_smelled = nullptr;
    for (const auto& s : smelled_) {
        if (s.kind == ObjectKind::Food) {
            if (!nearest_smelled || s.distance < nearest_smelled->distance) {
                nearest_smelled = &s;
            }
        }
    }

    if (nearest_smelled) {
        foodTargetMemory_.active = true;
        foodTargetMemory_.id = nearest_smelled->id;
        foodTargetMemory_.pos = nearest_smelled->pos;
        foodTargetMemory_.age = 0.f;
        return;
    }

    if (foodTargetMemory_.active && foodTargetMemory_.age > memory_persistance_) {
        foodTargetMemory_.active = false;
    }
}

void Byt::add_energy(float amount) noexcept {
    brain_.stored_energy = std::min(1.f, brain_.stored_energy + amount);
}

sf::Vector2f Byt::do_wander(float dt) {
    wander_change_timer_ -= dt;

    if (wander_change_timer_ <= 0.f) {
        std::uniform_real_distribution<float> angle_dist(-0.8f, 0.8f);

        float angle = angle_dist(rng_);
        float cs = std::cos(angle);
        float sn = std::sin(angle);

        sf::Vector2f d = wander_dir_;
        wander_dir_ = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        float L2 = wander_dir_.x * wander_dir_.x + wander_dir_.y * wander_dir_.y;
        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            wander_dir_.x *= invL;
            wander_dir_.y *= invL;
        } else {
            wander_dir_ = {1.f, 0.f};
        }

        std::uniform_real_distribution<float> t_dist(0.6f, 1.8f);
        wander_change_timer_ = t_dist(rng_);
    }

    return { wander_dir_.x * gains_.wander, wander_dir_.y * gains_.wander };
}

sf::Vector2f Byt::do_forage(float dt) {
    // 1) Best evidence: visible food right now
    const Seen* nearest_seen = nullptr;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Food) {
            if (!nearest_seen || s.distance < nearest_seen->distance) {
                nearest_seen = &s;
            }
        }
    }

    if (nearest_seen) {
        return steer_towards(pos_, nearest_seen->pos, gains_.forage);
    }

    // 2) No visible food: investigate remembered location
    if (foodTargetMemory_.active) {
        sf::Vector2f to{
            foodTargetMemory_.pos.x - pos_.x,
            foodTargetMemory_.pos.y - pos_.y
        };

        float dist2 = to.x*to.x + to.y*to.y;

        // If we've basically reached the remembered location and still
        // don't see food, give up on this memory and switch to search.
        const float reached_radius = 12.f;
        if (dist2 <= reached_radius * reached_radius) {
            foodTargetMemory_.active = false;
        } else {
            return steer_towards(pos_, foodTargetMemory_.pos, gains_.forage * 0.8f);
        }
    }

    // 3) No current food and no valid memory: search, don't jitter
    // Hungry search should feel more purposeful than neutral wander.
    wander_change_timer_ -= dt;

    if (wander_change_timer_ <= 0.f) {
        // Smaller angle changes than neutral -> steadier search heading
        std::uniform_real_distribution<float> angle_dist(-0.45f, 0.45f);
        float angle = angle_dist(rng_);

        float cs = std::cos(angle);
        float sn = std::sin(angle);

        sf::Vector2f d = wander_dir_;
        wander_dir_ = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        float L2 = wander_dir_.x*wander_dir_.x + wander_dir_.y*wander_dir_.y;
        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            wander_dir_.x *= invL;
            wander_dir_.y *= invL;
        } else {
            wander_dir_ = {1.f, 0.f};
        }

        // Hold heading a bit longer while searching
        std::uniform_real_distribution<float> t_dist(0.9f, 1.8f);
        wander_change_timer_ = t_dist(rng_);
    }

    return { wander_dir_.x * gains_.forage * 0.55f,
             wander_dir_.y * gains_.forage * 0.55f };
}

sf::Vector2f Byt::do_seek_companion(float /*dt*/) {
    int n = 0; sf::Vector2f c{0.f,0.f};
    for (const auto& s : seen_) if (s.kind == ObjectKind::Byt) { ++n; c.x += s.pos.x; c.y += s.pos.y; }
    if (n == 0) return do_wander(0.f);

    c.x /= n; c.y /= n;
    sf::Vector2f to{ c.x - pos_.x, c.y - pos_.y };
    float L2 = to.x*to.x + to.y*to.y;
    if (L2 < 1e-6f) return {0.f, 0.f};
    float invL = 1.0f / std::sqrt(L2);
    return { to.x * invL * gains_.companion, to.y * invL * gains_.companion };
}

void Byt::step(Seconds dt) noexcept {
    brain_.stored_energy -= brain_.energy_drain_per_sec * dt;
    if (brain_.stored_energy < 0.f) {
        brain_.stored_energy = 0.f;
    }
    decide_mood();
    decide_behavior();

    sf::Vector2f force{0.f, 0.f};

    // ---- Always-on: gentle personal space (emergent “separation”) ----
    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;
        sf::Vector2f away{ pos_.x - s.pos.x, pos_.y - s.pos.y };
        float d2 = away.x*away.x + away.y*away.y;
        const float min_d2 = 16.f;
        if (d2 < min_d2) d2 = min_d2;

        float inv_d = 1.0f / std::sqrt(d2);
        away.x *= inv_d * inv_d;
        away.y *= inv_d * inv_d;

        force.x += away.x * gains_.personal_space;
        force.y += away.y * gains_.personal_space;
    }

    // ---- Behavior-specific steering ----
    sf::Vector2f b{0.f, 0.f};
    switch (behavior_) {
        case Behavior::Wander:        b = do_wander(dt);         break;
        case Behavior::Forage:        b = do_forage(dt);         break;
        case Behavior::SeekCompanion: b = do_seek_companion(dt); break;
    }

    force.x += b.x;
    force.y += b.y;

    vel_.x += force.x * dt;
    vel_.y += force.y * dt;
    vel_ = clamp_speed(vel_, max_speed_);
}

void Byt::integrate(Seconds dt) noexcept {
    const float drag_per_sec = 1.5f;
    float k = std::exp(-drag_per_sec * dt);
    vel_.x *= k; vel_.y *= k;

    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;
}

} // namespace byts
