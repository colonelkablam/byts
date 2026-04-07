#include "byts/render/RenderResources.hpp"

#include <stdexcept>
#include <string>

namespace byts::render {

static void set_default_text(sf::Text& text,
                             const sf::Font& font,
                             const std::string& label,
                             unsigned size)
{
    text.setFont(font);
    text.setCharacterSize(size);
    text.setFillColor(sf::Color::White);
    text.setString(label);
}

void init_render_resources(RenderResources& rr)
{
    rr.ring.setPointCount(64);
    rr.ring.setFillColor(sf::Color::Transparent);
    rr.ring.setOutlineThickness(2.f);

    rr.byt_dot = sf::CircleShape(3.f);
    rr.byt_dot.setOrigin(3.f, 3.f);
    rr.byt_dot.setFillColor(sf::Color::White);

    rr.object_shape.setPointCount(32);
    rr.object_shape.setFillColor(sf::Color::White);
    rr.object_shape.setOutlineColor(sf::Color::Black);
    rr.object_shape.setOutlineThickness(2.f);

    rr.debug_dot.setPointCount(16);

    if (!rr.font.loadFromFile("assets/DejaVuSans.ttf")) {
        throw std::runtime_error("Failed to load font: assets/DejaVuSans.ttf");
    }

    set_default_text(rr.debug_text, rr.font, "", 10);
    set_default_text(rr.sight_label, rr.font, "sight", 8);
    set_default_text(rr.smell_label, rr.font, "smell", 8);
    set_default_text(rr.hearing_label, rr.font, "hearing", 8);
    set_default_text(rr.idle_label, rr.font, "idle\nanchor", 8);
}

} // namespace byts::render