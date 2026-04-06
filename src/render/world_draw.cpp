#include "byts/World.hpp"
#include "byts/Byt.hpp"
#include "byts/render/RenderResources.hpp"
#include "byts/render/world_draw.hpp"

#include <cmath>

namespace byts::render {

namespace {

sf::Color to_sf_colour(const Byt::ColourRGB& c) {
    return sf::Color(c.r, c.g, c.b);
}

void draw_object_circle(sf::RenderWindow& win,
                        sf::CircleShape& shape,
                        sf::Vector2f center,
                        float r,
                        sf::Color c)
{
    shape.setRadius(r);
    shape.setOrigin(r, r);
    shape.setPosition(center);
    shape.setFillColor(c);

    win.draw(shape);
}

void draw_byt_heading(sf::RenderWindow& win,
                      const Byt& b,
                      float length,
                      float byt_radius,
                      sf::Color c = sf::Color::White)
{
    sf::Vector2f dir = b.vel();

    float len2 = dir.x * dir.x + dir.y * dir.y;
    if (len2 < 1e-6f) {
        return;
    }

    float inv_len = 1.f / std::sqrt(len2);
    dir.x *= inv_len;
    dir.y *= inv_len;

    sf::Vector2f center = b.pos();

    sf::Vector2f start{
        center.x + dir.x * byt_radius,
        center.y + dir.y * byt_radius
    };

    sf::Vector2f end{
        center.x + dir.x * (byt_radius + length),
        center.y + dir.y * (byt_radius + length)
    };

    sf::Vertex line[] = {
        sf::Vertex(start, c),
        sf::Vertex(end, c)
    };

    win.draw(line, 2, sf::Lines);
}

void draw_byt(sf::RenderWindow& win,
              const Byt& b,
              sf::CircleShape& shape,
              sf::Color heading_colour = sf::Color::Yellow)
{
    float r = b.size();
    draw_object_circle(win, shape, b.pos(), r, to_sf_colour(b.colour()));
    draw_byt_heading(win, b, r, r, heading_colour);
}

} // namespace



void draw_world(sf::RenderWindow& win,
                const World& world,
                RenderResources& rr)
{
    for (const auto& obj : world.objects()) {
        draw_object_circle(
            win,
            rr.object_shape,
            obj->pos(),
            obj->size(),
            obj->color()
        );
    }

    for (const auto& b : world.byts()) {
        draw_byt(win, b, rr.byt_dot);
    }
}

} // namespace byts::render