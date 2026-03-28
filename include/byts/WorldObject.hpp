#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include "SenseMask.hpp"

namespace byts {

class WorldObject {
public:
    WorldObject(ObjectKind kind,
                sf::Vector2f pos,
                SenseMask perceptible_by,
                bool solid,
                float size,
                sf::Color color,
                std::size_t id,
                float visibility = 1.f,
                float smell_strength = 0.f,
                float loudness = 0.f)
                
        : kind_(kind),
          pos_(pos),
          perceptible_by_(perceptible_by),
          solid_(solid),
          size_(size),
          color_(color),
          id_(id),
          visibility_(visibility),
          smell_strength_(smell_strength),
          loudness_(loudness)
    {}

    virtual ~WorldObject() = default;

    ObjectKind kind() const noexcept { return kind_; }
    const sf::Vector2f& pos() const noexcept { return pos_; }
    std::size_t id() const noexcept { return id_; }

    SenseMask perceptible_by() const noexcept { return perceptible_by_; }
    bool solid() const noexcept { return solid_; }
    float size() const noexcept { return size_; }
    sf::Color color() const noexcept { return color_; }

    float visibility() const noexcept { return visibility_; }
    float smell() const noexcept { return smell_strength_; }
    float loudness() const noexcept { return loudness_; }

protected:
    ObjectKind kind_;
    sf::Vector2f pos_;
    SenseMask perceptible_by_;
    bool solid_;
    float size_;
    sf::Color color_;
    std::size_t id_;

    float visibility_;
    float smell_strength_;
    float loudness_;
};

} // namespace byts