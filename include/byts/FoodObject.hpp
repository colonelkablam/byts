#pragma once
#include "WorldObject.hpp"

namespace byts {

class FoodObject : public WorldObject {
public:
    FoodObject(sf::Vector2f pos, std::size_t id, float energy)
        : WorldObject(ObjectKind::Food, pos, SenseMask::Visible, false, 4.f, id),
          energy_(energy) {}

    float energy() const noexcept { return energy_; }

private:
    float energy_;
};

} // namespace byts