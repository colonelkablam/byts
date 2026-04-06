#include "byts/render/debug_draw.hpp"
#include "byts/World.hpp"
#include "byts/Byt.hpp"
#include "byts/render/RenderResources.hpp"

#include <sstream>
#include <iomanip>

namespace byts::render {

namespace {

void draw_byt_senses(sf::RenderWindow& win,
                     const Byt& b,
                     RenderResources& rr)
{
    draw_ring(win, rr.ring, b.pos(),
              b.senses().sight_range,
              rr.sight_label,
              sf::Color(0, 255, 0, 120));

    draw_ring(win, rr.ring, b.pos(),
              b.senses().hearing_range,
              rr.hearing_label,
              sf::Color(0, 128, 255, 120));

    draw_ring(win, rr.ring, b.pos(),
              b.senses().smell_range,
              rr.smell_label,
              sf::Color(255, 100, 255, 100));
}

void draw_byt_status(sf::RenderWindow& win,
                     const Byt& b,
                     RenderResources& rr)
{
    draw_debug_text(win,
                    rr.debug_text,
                    b.pos(),
                    make_byt_debug_text(b));
}

void draw_byt_idle_anchor(sf::RenderWindow& win,
                          const Byt& b,
                          RenderResources& rr)
{
    if (!b.has_idle_anchor()) {
        return;
    }

    const auto anchor = b.idle_anchor();

    draw_ring(win, rr.ring,
              anchor,
              b.idle_leash_radius(),
              rr.idle_label,
              sf::Color(200, 200, 200, 100));

    draw_debug_text_centered(win,
                             rr.debug_text,
                             anchor,
                             "UID: " + std::to_string(b.id()));
}

void draw_byt_food_memories(sf::RenderWindow& win,
                            const Byt& b,
                            RenderResources& rr)
{
    for (const auto& pos : b.food_memory_positions()) {
        draw_debug_dot(win, rr.debug_dot, pos, 3.f, sf::Color::Magenta);
        draw_debug_line(win, b.pos(), pos, sf::Color(255, 0, 255, 60));
    }
}

} // namespace

void draw_ring(sf::RenderWindow& win,
               sf::CircleShape& ring,
               sf::Vector2f center,
               float r,
               sf::Text& label,
               sf::Color outline)
{
    ring.setRadius(r);
    ring.setOrigin(r, r);
    ring.setPosition(center);
    ring.setOutlineColor(outline);
    win.draw(ring);

    sf::FloatRect bounds = label.getLocalBounds();
    label.setOrigin(bounds.left + bounds.width / 2.f, 0.f);
    label.setPosition(center.x, center.y - r + 2.f);
    win.draw(label);
}

void draw_debug_text(sf::RenderWindow& win,
                     sf::Text& text,
                     sf::Vector2f pos,
                     const std::string& label)
{
    text.setString(label);
    text.setOrigin(0.f, 0.f);
    text.setPosition(pos.x + 6.f, pos.y + 5.f);
    win.draw(text);
}

void draw_debug_text_centered(sf::RenderWindow& win,
                              sf::Text& text,
                              sf::Vector2f pos,
                              const std::string& label)
{
    text.setString(label);

    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width / 2.f,
                   bounds.top + bounds.height / 2.f);

    text.setPosition(pos);
    win.draw(text);
}

void draw_debug_dot(sf::RenderWindow& win,
                    sf::CircleShape& shape,
                    sf::Vector2f pos,
                    float r,
                    sf::Color c)
{
    shape.setRadius(r);
    shape.setOrigin(r, r);
    shape.setPosition(pos);
    shape.setFillColor(c);
    win.draw(shape);
}

void draw_debug_line(sf::RenderWindow& win,
                     sf::Vector2f a,
                     sf::Vector2f b,
                     sf::Color c)
{
    sf::Vertex line[] = {
        sf::Vertex(a, c),
        sf::Vertex(b, c)
    };
    win.draw(line, 2, sf::Lines);
}

std::string make_byt_debug_text(const Byt& b)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << std::boolalpha;

    ss << "UID: " << b.id() << '\n';
    ss << "intent: " << b.intent() << '\n';
    ss << "energy: " << b.energy() << '\n';
    ss << "food smell: " << b.food_smell_strength()
       << " (?> " << b.food_smell_threshold() << ")" << '\n';
    ss << "idle anchor set: " << b.has_idle_anchor();

    return ss.str();
}

void draw_debug(sf::RenderWindow& win,
                const World& world,
                RenderResources& rr)
{
    for (const auto& b : world.byts()) {
        draw_byt_senses(win, b, rr);
        draw_byt_status(win, b, rr);
        draw_byt_idle_anchor(win, b, rr);
        draw_byt_food_memories(win, b, rr);
    }
}

} // namespace byts::render