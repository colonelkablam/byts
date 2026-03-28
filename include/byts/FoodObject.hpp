#pragma once
#include "WorldObject.hpp"
#include <SFML/Graphics/Color.hpp>

namespace byts {

class FoodObject : public WorldObject {
public:
    FoodObject(sf::Vector2f pos, std::size_t id, float energy, float smell_strength, float size)
        : WorldObject(
            ObjectKind::Food,
            pos,
            SenseMask::Visible | SenseMask::Smell,
            false,                 // not solid
            size,                  // size
            sf::Color(255,180,0),  // colour
            id,
            1.f, // currently 1 visibility food harcoded
            smell_strength,
            0.f // currently 0 volume on food harcoded
          ),
          energy_(energy)
    {}

    float energy() const noexcept { return energy_; }

private:
    float energy_;
};

} // namespace byts