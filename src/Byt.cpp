#include "byts/Byt.hpp"
#include "byts/World.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include "byts/Byt.hpp"

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

            if (it.kind == ObjectKind::Food) {
                add_or_update_memory(
                    MemoryKind::Food,
                    it.id,
                    {it.pos.x, it.pos.y},
                    static_cast<std::uint8_t>(SenseSource::Sight),
                    0.6f,
                    6.f,
                    20.f
                );
            }
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

            if (it.kind == ObjectKind::Food) {
                add_or_update_memory(
                    MemoryKind::Food,
                    it.id,
                    {it.pos.x, it.pos.y},
                    static_cast<std::uint8_t>(SenseSource::Smell),
                    0.25f,
                    10.f,
                    40.f
                );
            }
        }
    }
}

Byt::Memory* Byt::find_matching_memory(MemoryKind kind,
                                       std::optional<std::size_t> object_id,
                                       sf::Vector2f pos,
                                       float merge_radius)
{
    if (object_id.has_value()) {
        for (auto& m : memories_) {
            if (m.kind == kind && m.object_id == object_id) {
                return &m;
            }
        }
    }

    float best_dist2 = merge_radius * merge_radius;
    Memory* best = nullptr;

    for (auto& m : memories_) {
        if (m.kind != kind) continue;
        if (m.object_id.has_value()) continue;

        sf::Vector2f d{ m.pos.x - pos.x, m.pos.y - pos.y };
        float dist2 = d.x*d.x + d.y*d.y;

        if (dist2 < best_dist2) {
            best_dist2 = dist2;
            best = &m;
        }
    }

    return best;
}

void Byt::add_or_update_memory(MemoryKind kind,
                               std::optional<std::size_t> object_id,
                               sf::Vector2f pos,
                               std::uint8_t source_flag,
                               float confidence_boost,
                               float ttl,
                               float merge_radius)
{
    Memory* m = find_matching_memory(kind, object_id, pos, merge_radius);

    if (m) {
        // --- UPDATE EXISTING ---
        m->age = 0.f;
        m->ttl = std::max(m->ttl, ttl);

        // Confidence accumulates but clamps
        m->confidence = std::min(1.f, m->confidence + confidence_boost);

        // Position update
        if (object_id.has_value()) {
            // precise → overwrite
            m->pos = pos;
        } else {
            // fuzzy → blend
            float alpha = 0.3f;
            m->pos.x = m->pos.x * (1.f - alpha) + pos.x * alpha;
            m->pos.y = m->pos.y * (1.f - alpha) + pos.y * alpha;
        }

        m->sources |= source_flag;
    }
    else {
        // --- CREATE NEW ---
        Memory mem;
        mem.kind = kind;
        mem.nature = object_id.has_value() ? MemoryNature::Object
                                           : MemoryNature::Place;
        mem.object_id = object_id;
        mem.pos = pos;
        mem.confidence = confidence_boost;
        mem.age = 0.f;
        mem.ttl = ttl;
        mem.sources = source_flag;

        memories_.push_back(mem);
    }
}

void Byt::update_memories(float dt) {
    for (auto it = memories_.begin(); it != memories_.end(); ) {
        it->age += dt;

        // decay confidence
        it->confidence -= dt * 0.1f;

        if (it->age > it->ttl || it->confidence <= 0.f) {
            it = memories_.erase(it);
        } else {
            ++it;
        }
    }
}

void Byt::update_internal_needs(Seconds dt) {
    brain_.stored_energy -= brain_.energy_drain_per_sec * dt;
    if (brain_.stored_energy < 0.f) {
        brain_.stored_energy = 0.f;
    }

    brain_.social_need += brain_.social_rise_per_sec * dt;
    if (brain_.social_need > 1.f) {
        brain_.social_need = 1.f;
    }

    brain_.hunger_drive = 1.f - brain_.stored_energy;
    brain_.social_drive = brain_.social_need;
}

void Byt::choose_intention() {
    bool sees_food = false;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Food) {
            sees_food = true;
            break;
        }
    }

    bool has_food_memory = false;
    for (const auto& m : memories_) {
        if (m.kind == MemoryKind::Food) {
            has_food_memory = true;
            break;
        }
    }

    bool sees_byt = false;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Byt) {
            sees_byt = true;
            break;
        }
    }

    if (brain_.hunger_drive >= brain_.hunger_on) {
        if (sees_food) {
            intention_ = Intention::MoveToFoodVisible;
        } else if (has_food_memory) {
            intention_ = Intention::MoveToFoodMemory;
        } else {
            intention_ = Intention::SearchFood;
        }
        return;
    }

    if (brain_.social_drive >= brain_.social_on && sees_byt) {
        intention_ = Intention::SeekCompanion;
        return;
    }

    intention_ = Intention::Idle;
}

void Byt::add_energy(float amount) noexcept
{
    brain_.stored_energy = std::min(1.f, brain_.stored_energy + amount);
}

void Byt::step(Seconds dt) noexcept {
    update_memories(dt);
    update_internal_needs(dt);
    choose_intention();

    sf::Vector2f force{0.f, 0.f};

    // always-on separation
    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;

        sf::Vector2f away{ pos_.x - s.pos.x, pos_.y - s.pos.y };
        float d2 = away.x * away.x + away.y * away.y;
        const float min_d2 = 16.f;
        if (d2 < min_d2) d2 = min_d2;

        float inv_d = 1.0f / std::sqrt(d2);
        away.x *= inv_d * inv_d;
        away.y *= inv_d * inv_d;

        force.x += away.x * gains_.personal_space;
        force.y += away.y * gains_.personal_space;
    }

    sf::Vector2f b{0.f, 0.f};

    switch (intention_) {
        case Intention::Idle:
            b = steer_idle(dt);
            break;
        case Intention::SearchFood:
            b = steer_search_food(dt);
            break;
        case Intention::MoveToFoodVisible:
            b = steer_to_visible_food();
            break;
        case Intention::MoveToFoodMemory:
            b = steer_to_food_memory(dt);
            break;
        case Intention::SeekCompanion:
            b = steer_to_companion(dt);
            break;
    }

    force.x += b.x;
    force.y += b.y;

    vel_.x += force.x * dt;
    vel_.y += force.y * dt;
    vel_ = clamp_speed(vel_, max_speed_);
}

sf::Vector2f Byt::steer_idle(Seconds dt) {
    // countdown until next direction change
    wander_change_timer_ -= dt;

    if (wander_change_timer_ <= 0.f) {
        // small random rotation (radians)
        std::uniform_real_distribution<float> angle_dist(-0.6f, 0.6f);
        float angle = angle_dist(rng_);

        float cs = std::cos(angle);
        float sn = std::sin(angle);

        // rotate current direction
        sf::Vector2f d = wander_dir_;
        wander_dir_ = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        // normalise (safety)
        float L2 = wander_dir_.x * wander_dir_.x + wander_dir_.y * wander_dir_.y;
        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            wander_dir_.x *= invL;
            wander_dir_.y *= invL;
        } else {
            wander_dir_ = {1.f, 0.f};
        }

        // how long to keep this heading
        std::uniform_real_distribution<float> time_dist(0.8f, 2.0f);
        wander_change_timer_ = time_dist(rng_);
    }

    // constant forward push in wander direction
    return {
        wander_dir_.x * gains_.idle,
        wander_dir_.y * gains_.idle
    };
}

sf::Vector2f Byt::steer_search_food(Seconds dt) {
    // countdown until next direction adjustment
    wander_change_timer_ -= dt;

    if (wander_change_timer_ <= 0.f) {
        // smaller turns than idle = more purposeful movement
        std::uniform_real_distribution<float> angle_dist(-0.35f, 0.35f);
        float angle = angle_dist(rng_);

        float cs = std::cos(angle);
        float sn = std::sin(angle);

        // rotate current heading
        sf::Vector2f d = wander_dir_;
        wander_dir_ = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        // normalise
        float L2 = wander_dir_.x * wander_dir_.x + wander_dir_.y * wander_dir_.y;
        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            wander_dir_.x *= invL;
            wander_dir_.y *= invL;
        } else {
            wander_dir_ = {1.f, 0.f};
        }

        // hold heading longer than idle
        std::uniform_real_distribution<float> time_dist(1.0f, 2.2f);
        wander_change_timer_ = time_dist(rng_);
    }

    // stronger forward push than idle wander
    return {
        wander_dir_.x * gains_.search_food,
        wander_dir_.y * gains_.search_food
    };
}

sf::Vector2f Byt::steer_to_visible_food() const {
    const Seen* nearest_food = nullptr;

    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Food) continue;

        if (!nearest_food || s.distance < nearest_food->distance) {
            nearest_food = &s;
        }
    }

    if (!nearest_food) {
        return {0.f, 0.f};
    }

    return steer_towards(pos_, nearest_food->pos, gains_.visible_food);
}

sf::Vector2f Byt::steer_to_food_memory(Seconds dt) {
    (void)dt;
    const Memory* best_food_memory = nullptr;
    float best_score = -1.f;

    for (const auto& m : memories_) {
        if (m.kind != MemoryKind::Food) continue;

        sf::Vector2f d{ m.pos.x - pos_.x, m.pos.y - pos_.y };
        float dist2 = d.x * d.x + d.y * d.y;

        // prefer confident, nearby memories
        float score = m.confidence / (1.f + std::sqrt(dist2));

        if (!best_food_memory || score > best_score) {
            best_food_memory = &m;
            best_score = score;
        }
    }

    if (!best_food_memory) {
        return {0.f, 0.f};
    }

    sf::Vector2f to{
        best_food_memory->pos.x - pos_.x,
        best_food_memory->pos.y - pos_.y
    };

    float dist2 = to.x * to.x + to.y * to.y;

    // close enough to the remembered location
    const float reached_radius = 12.f;
    if (dist2 <= reached_radius * reached_radius) {
        return {0.f, 0.f};
    }

    return steer_towards(pos_, best_food_memory->pos, gains_.memory_food * 0.8f);
}

sf::Vector2f Byt::steer_to_companion(Seconds dt) {
    (void)dt;

    int count = 0;
    sf::Vector2f centre{0.f, 0.f};

    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;
        ++count;
        centre.x += s.pos.x;
        centre.y += s.pos.y;
    }

    if (count == 0) {
        return {0.f, 0.f};
    }

    centre.x /= static_cast<float>(count);
    centre.y /= static_cast<float>(count);

    return steer_towards(pos_, centre, gains_.companion);
}

sf::Vector2f Byt::steer_towards(sf::Vector2f from, sf::Vector2f to, float strength) const noexcept {    
    sf::Vector2f d{ to.x - from.x, to.y - from.y };
    float L2 = d.x*d.x + d.y*d.y;
    if (L2 < 1e-6f) return {0.f, 0.f};

    float invL = 1.f / std::sqrt(L2);
    return { d.x * invL * strength, d.y * invL * strength };
}

void Byt::integrate(Seconds dt) noexcept {
    const float drag_per_sec = 1.5f;
    float k = std::exp(-drag_per_sec * dt);
    vel_.x *= k; vel_.y *= k;

    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;
}

} // namespace byts