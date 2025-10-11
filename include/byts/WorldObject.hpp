#pragma once
#include <SFML/System/Vector2.hpp>
#include "Sense.hpp"

namespace byts {

// Generic world object (non-Byt). Keep it simple/data-oriented for now.
class WorldObject {
public:
    WorldObject(ObjectKind kind, sf::Vector2f pos, SenseMask senses, std::size_t id)
        : kind_(kind), pos_(pos), senses_(senses), id_(id) {}

    ObjectKind kind()   const noexcept { return kind_; }
    sf::Vector2f pos()  const noexcept { return pos_; }
    SenseMask senses()  const noexcept { return senses_; }
    std::size_t id()    const noexcept { return id_; }

    void set_pos(sf::Vector2f p) noexcept { pos_ = p; }
    void set_senses(SenseMask s) noexcept { senses_ = s; }

private:
    ObjectKind   kind_;
    sf::Vector2f pos_;
    SenseMask    senses_;
    std::size_t  id_;
};

} // namespace byts
