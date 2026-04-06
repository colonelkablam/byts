#pragma once

#include <SFML/Graphics.hpp>
#include <string>

namespace byts {
class Byt;
class World;
}

namespace byts::render {

struct RenderResources;

void draw_debug(sf::RenderWindow& win,
                const World& world,
                RenderResources& rr);

std::string make_byt_debug_text(const Byt& b);

void draw_ring(sf::RenderWindow& win,
               sf::CircleShape& ring,
               sf::Vector2f center,
               float r,
               sf::Text& label,
               sf::Color outline);

void draw_debug_text(sf::RenderWindow& win,
                     sf::Text& text,
                     sf::Vector2f pos,
                     const std::string& label);

void draw_debug_text_centered(sf::RenderWindow& win,
                              sf::Text& text,
                              sf::Vector2f pos,
                              const std::string& label);

void draw_debug_dot(sf::RenderWindow& win,
                    sf::CircleShape& shape,
                    sf::Vector2f pos,
                    float r = 2.f,
                    sf::Color c = sf::Color::Magenta);

void draw_debug_line(sf::RenderWindow& win,
                     sf::Vector2f a,
                     sf::Vector2f b,
                     sf::Color c = sf::Color::Magenta);

} // namespace byts::render