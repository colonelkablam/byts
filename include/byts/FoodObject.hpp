#pragma once
#include "WorldObject.hpp"
#include <SFML/Graphics/Color.hpp>

namespace byts {

class FoodObject : public WorldObject {
public:
    FoodObject(sf::Vector2f pos, std::size_t id, float energy, float size)
        : WorldObject(
            ObjectKind::Food,
            pos,
            SenseMask::Visible | SenseMask::Smell,
            false,                 // not solid
            size,                  // size
            sf::Color(255,180,0),  // colour
            id
          ),
          energy_(energy)
    {}

    float energy() const noexcept { return energy_; }

private:
    float energy_;
};

} // namespace byts