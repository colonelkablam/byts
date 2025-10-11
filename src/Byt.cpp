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
}

void Byt::sense_update(const World& world, Seconds dt) {
    sight_timer_   += dt;
    hearing_timer_ += dt;

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
}

void Byt::decide_behavior() {
    // For now: hard-coded mapping; later use brain_ to switch moods.
    switch (mood_) {
        case Mood::Neutral:  behavior_ = Behavior::Wander;        break;
        case Mood::Hungry:   behavior_ = Behavior::Forage;        break;
        case Mood::Friendly: behavior_ = Behavior::SeekCompanion; break;
    }
}

sf::Vector2f Byt::do_wander(float /*dt*/) {
    std::normal_distribution<float> N(0.f, 1.f);
    return { N(rng_) * gains_.wander, N(rng_) * gains_.wander };
}

sf::Vector2f Byt::do_forage(float /*dt*/) {
    const Seen* nearest = nullptr;
    for (const auto& s : seen_) if (s.kind == ObjectKind::Food)
        if (!nearest || s.distance < nearest->distance) nearest = &s;

    if (!nearest) return do_wander(0.f);

    sf::Vector2f to{ nearest->pos.x - pos_.x, nearest->pos.y - pos_.y };
    float L2 = to.x*to.x + to.y*to.y;
    if (L2 < 1e-6f) return {0.f, 0.f};
    float invL = 1.0f / std::sqrt(L2);
    return { to.x * invL * gains_.forage, to.y * invL * gains_.forage };
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
    decide_behavior();

    sf::Vector2f force{0.f, 0.f};

    // ---- Always-on: gentle personal space (emergent “separation”) ----
    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;
        sf::Vector2f away{ pos_.x - s.pos.x, pos_.y - s.pos.y };
        float d2 = away.x*away.x + away.y*away.y;
        const float min_d2 = 16.f; if (d2 < min_d2) d2 = min_d2;
        float inv_d = 1.0f / std::sqrt(d2);     // 1/d
        away.x *= inv_d * inv_d;                // ~1/d^2
        away.y *= inv_d * inv_d;
        force.x += away.x * gains_.personal_space;
        force.y += away.y * gains_.personal_space;
    }

    // ---- Behavior-specific steering ----
    sf::Vector2f b = {};
    switch (behavior_) {
        case Behavior::Wander:        b = do_wander(dt);        break;
        case Behavior::Forage:        b = do_forage(dt);        break;
        case Behavior::SeekCompanion: b = do_seek_companion(dt);break;
    }
    force.x += b.x; force.y += b.y;

    // integrate force → velocity
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
