#include "World.hpp"
#include <random>
#include <cmath>

namespace byts {

void World::spawn(std::size_t n) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> xd(5.f, w_-5.f), yd(5.f, h_-5.f);
    byts_.clear(); byts_.reserve(n);
    for (std::size_t i=0;i<n;++i) byts_.emplace_back(sf::Vector2f{xd(rng), yd(rng)});
}

void World::build_perception(std::size_t i, Byt::Seconds, Perception& out) const {
    const auto& me = byts_[i];
    auto p = me.pos();

    // Wall distances as senses (no world size leaked to Byt)
    out.dist_left   = p.x;
    out.dist_right  = w_ - p.x;
    out.dist_top    = p.y;
    out.dist_bottom = h_ - p.y;

    // Very simple neighbor sensing (O(N^2) for now)
    out.neighbors.clear();
    const float radius = 40.f;
    for (std::size_t j=0;j<byts_.size();++j) if (j!=i) {
        sf::Vector2f o{ byts_[j].pos().x - p.x, byts_[j].pos().y - p.y };
        float d = std::sqrt(o.x*o.x + o.y*o.y);
        if (d < radius) out.neighbors.push_back({o, d});
    }
}

void World::apply_boundaries(Byt& b) const noexcept {
    auto p = b.pos();
    auto v = b.vel();

    if (p.x < 0.f) { p.x = 0.f; v.x = -v.x; }
    if (p.y < 0.f) { p.y = 0.f; v.y = -v.y; }
    if (p.x > w_)  { p.x = w_;  v.x = -v.x; }
    if (p.y > h_)  { p.y = h_;  v.y = -v.y; }

    b.set_pos(p);
    b.set_vel(v);
}

void World::update(float dt) {
    Perception sense;
    for (std::size_t i=0;i<byts_.size();++i) {
        sense.neighbors.clear();
        build_perception(i, dt, sense);  // world builds senses
        byts_[i].step(sense, dt);        // byt decides from senses
    }
    for (auto& b : byts_) {
        b.integrate(dt);                 // physics integration
        apply_boundaries(b);             // world rules
    }
}

} // namespace byts
