#pragma once

#include <SFML/Graphics.hpp>

namespace byts::render {

struct RenderResources {
    sf::CircleShape ring;
    sf::CircleShape byt_dot;
    sf::CircleShape object_shape;
    sf::CircleShape debug_dot;

    sf::Font font;

    sf::Text debug_text;
    sf::Text sight_label;
    sf::Text smell_label;
    sf::Text hearing_label;
    sf::Text idle_label;
};

void init_render_resources(RenderResources& rr);

} // namespace byts::render