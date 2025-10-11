#include "byts/World.hpp"
#include <random>
#include <cmath>

namespace byts {

namespace {
inline float len2(sf::Vector2f v) noexcept { return v.x*v.x + v.y*v.y; }
} // namespace

void World::query_in_radius(sf::Vector2f center,
                            float radius,
                            SenseMask sense,
                            std::vector<Perceived>& out) const
{
    out.clear();
    const float r2 = radius * radius;

    auto consider = [&](ObjectKind kind,
                        std::size_t id,
                        sf::Vector2f pos,
                        SenseMask senses)
    {
        if (!has(senses, sense)) return;
        sf::Vector2f d{ pos.x - center.x, pos.y - center.y };
        float d2 = len2(d);
        if (d2 <= r2) {
            out.push_back(Perceived{
                kind,
                std::sqrt(d2),
                id,
                { pos.x, pos.y }
            });
        }
    };

    // Byts (visible + audible)
    for (std::size_t i = 0; i < byts_.size(); ++i) {
        consider(ObjectKind::Byt, byts_[i].id(), byts_[i].pos(),
                 SenseMask::Visible | SenseMask::Audible);
    }

    // Generic world objects
    for (const auto& obj : objects_) {
        consider(obj.kind(), obj.id(), obj.pos(), obj.senses());
    }
}

std::size_t World::add_object(ObjectKind kind, sf::Vector2f pos, SenseMask senses) {
    std::size_t id = next_object_id_++;
    objects_.emplace_back(kind, pos, senses, id);
    return id;
}

void World::spawn(std::size_t n) {
    // Simple deterministic-ish scatter (use your own RNG/seed as needed)
    std::mt19937 rng(12345u);
    std::uniform_real_distribution<float> dx(0.f, w_), dy(0.f, h_);

    const std::size_t start = byts_.size();
    byts_.reserve(start + n);
    for (std::size_t i = 0; i < n; ++i) {
        Byt b({ dx(rng), dy(rng) });
        b.set_id(start + i + 1); // ids start at 1, keep 0 unused if preferred
        byts_.push_back(b);
    }

    // Add a few foods for the demo
    for (int i = 0; i < 8; ++i) {
        add_object(ObjectKind::Food,
                   { dx(rng), dy(rng) },
                   SenseMask::Visible);
    }
}

void World::update(float dt) {
    // 1) Sensing
    for (auto& b : byts_) b.sense_update(*this, dt);

    // 2) Decision/behaviour
    for (auto& b : byts_) b.step(dt);

    // 3) Integrate & boundaries
    for (auto& b : byts_) {
        b.integrate(dt);
        apply_boundaries(b);
    }
}

void World::apply_boundaries(Byt& b) const noexcept {
    auto p = b.pos();
    auto v = b.vel();

    // Simple bounce within [0,w_]x[0,h_]
    if (p.x < 0.f) { p.x = 0.f; v.x = std::abs(v.x); }
    if (p.x > w_)  { p.x = w_;  v.x = -std::abs(v.x); }
    if (p.y < 0.f) { p.y = 0.f; v.y = std::abs(v.y); }
    if (p.y > h_)  { p.y = h_;  v.y = -std::abs(v.y); }

    b.set_pos(p);
    b.set_vel(v);
}

} // namespace byts
