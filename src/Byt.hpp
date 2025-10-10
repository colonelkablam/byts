#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace byts {

struct Neighbor {
    sf::Vector2f offset; // neighbor_pos - my_pos (local frame)
    float        distance;
};

struct Perception {
    // Wall proximity as *senses*, not absolute world size:
    float dist_left, dist_right, dist_top, dist_bottom; // metres/pixels until wall
    std::vector<Neighbor> neighbors;                    // within some radius
    // You can add: local resource gradient, light level, noise, etc.
};

class Byt {
public:
    using Seconds = float;
    Byt(sf::Vector2f p) : pos_(p) {}

    void step(const Perception& sense, Seconds dt) noexcept; // decide+accelerate
    void integrate(Seconds dt) noexcept;                     // pos += vel*dt

    const sf::Vector2f& pos() const noexcept { return pos_; }
    const sf::Vector2f& vel() const noexcept { return vel_; }
    void set_pos(sf::Vector2f p) noexcept { pos_ = p; }   // add this
    void set_vel(sf::Vector2f v) noexcept { vel_ = v; }

private:
    sf::Vector2f pos_{0.f,0.f};
    sf::Vector2f vel_{0.f,0.f};
    float max_speed_ = 60.f;
};

} // namespace byts
