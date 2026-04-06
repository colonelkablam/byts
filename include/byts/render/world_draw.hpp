#pragma once

#include <SFML/Graphics.hpp>

namespace byts {
    class World;
}

namespace byts::render {

struct RenderResources;

void draw_world(sf::RenderWindow& win,
                const World& world,
                RenderResources& rr);

} // namespace byts::render