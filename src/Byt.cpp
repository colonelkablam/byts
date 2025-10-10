#include "Byt.hpp"
#include <cmath>

namespace byts {

static inline float len(sf::Vector2f v){ return std::sqrt(v.x*v.x+v.y*v.y); }
static inline sf::Vector2f norm(sf::Vector2f v){ float L=len(v); return L>1e-6f? sf::Vector2f{v.x/L, v.y/L}: sf::Vector2f{0,0}; }

void Byt::step(const Perception& s, Seconds dt) noexcept {
    // Simple steering from senses (no world size leaks):
    sf::Vector2f steering{0,0};

    // Avoid walls if close (treat distances as repulsive fields)
    const float wall_thresh = 30.f;
    if (s.dist_left   < wall_thresh) steering.x +=  (wall_thresh - s.dist_left) * 2.f;
    if (s.dist_right  < wall_thresh) steering.x += -(wall_thresh - s.dist_right)* 2.f;
    if (s.dist_top    < wall_thresh) steering.y +=  (wall_thresh - s.dist_top)   * 2.f;
    if (s.dist_bottom < wall_thresh) steering.y += -(wall_thresh - s.dist_bottom)* 2.f;

    // Simple neighbor separation
    for (auto& n : s.neighbors) {
        if (n.distance < 20.f) steering = sf::Vector2f{ steering.x - n.offset.x, steering.y - n.offset.y };
    }

    // Drift a little if nothing sensed
    if (len(steering) < 1e-3f) steering = sf::Vector2f{0.5f, 0.2f};

    // Apply steering
    vel_.x += steering.x * dt * 20.f;
    vel_.y += steering.y * dt * 20.f;

    // Clamp speed
    float L = len(vel_);
    if (L > max_speed_) vel_ = sf::Vector2f{ vel_.x * (max_speed_/L), vel_.y * (max_speed_/L) };
}

void Byt::integrate(Seconds dt) noexcept {
    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;
}

} // namespace byts
