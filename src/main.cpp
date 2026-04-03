#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>

#include "byts/World.hpp"


using namespace byts;

static void draw_ring(sf::RenderWindow& win,
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
    label.setOrigin(bounds.width / 2.f, 0.f);
    label.setPosition(center.x, center.y - r + 2.f);
    win.draw(label);
}

static void draw_debug_text(sf::RenderWindow& win,
                            sf::Text& text,
                            sf::Vector2f pos,
                            const std::string& label)
{
    text.setString(label);
    text.setPosition(pos.x + 6.f, pos.y + 5.f);
    win.draw(text);
}

static void draw_object_circle(sf::RenderWindow& win,
                        sf::CircleShape& shape,
                        sf::Vector2f center,
                        float r,
                        sf::Color c = sf::Color(255,180,0))
{
    shape.setRadius(r);
    shape.setOrigin(r, r);
    shape.setPosition(center);
    shape.setFillColor(c);

    win.draw(shape);
}

static void draw_debug_dot(sf::RenderWindow& win,
                           sf::CircleShape& shape,
                           sf::Vector2f pos,
                           float r = 2.f,
                           sf::Color c = sf::Color::Magenta)
{
    shape.setRadius(r);
    shape.setOrigin(r, r);
    shape.setPosition(pos);
    shape.setFillColor(c);

    win.draw(shape);
}

static void draw_debug_line(sf::RenderWindow& win,
                            sf::Vector2f a,
                            sf::Vector2f b,
                            sf::Color c = sf::Color::Magenta)
{
    sf::Vertex line[] = {
        sf::Vertex(a, c),
        sf::Vertex(b, c)
    };
    win.draw(line, 2, sf::Lines);
}

static void set_default_text(sf::Text& text,
                             const sf::Font& font,
                             const std::string& label = "u",
                             unsigned size = 14)
{
    text.setFont(font);
    text.setCharacterSize(size);
    text.setFillColor(sf::Color::White);
    text.setString(label);
}

static std::string make_byt_debug_text(const Byt& b)
{
    std::string t("false");
    if (b.has_idle_anchor()) {
        t = "true";
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "intent: " << b.intent() << '\n';
    ss << "energy: " << b.energy() << '\n';
    ss << "food smell: " << b.food_smell_strength()
       << " (?> " << b.food_smell_threshold() << ")" << '\n';
    ss << "idle anchor set: " << t;

    return ss.str();
}

int main() {
    const float W = 1280.f, H = 1024.f;
    sf::RenderWindow win(sf::VideoMode((unsigned)W,(unsigned)H), "Byts debug");
    win.setFramerateLimit(120);

    sf::CircleShape ring;
    ring.setPointCount(64);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(1.f);
    
    sf::Font font;
    if (!font.loadFromFile("assets/DejaVuSans.ttf")) {
        return 1;
    }

    sf::Text intent_text;
    set_default_text(intent_text, font, "", 10);

    sf::Text sight_label;
    set_default_text(sight_label, font, "sight", 8);

    sf::Text smell_label;
    set_default_text(smell_label, font, "smell", 8);

    sf::Text hearing_label;
    set_default_text(hearing_label, font, "hearing", 8);

    sf::Text idle_label;
    set_default_text(idle_label, font, "hearing", 8);

    sf::CircleShape debugDot;
    debugDot.setPointCount(16);


    World world{W, H, std::random_device{}()};
    world.spawn_byts(1);
    world.spawn_food(1, 0.35f, 0.5f, 6);
    
    sf::CircleShape bytDot(3.f);  bytDot.setOrigin(3.f,3.f);  bytDot.setFillColor(sf::Color::White);
    sf::CircleShape objectShape;
    objectShape.setPointCount(32);   // smoother circle
    objectShape.setOutlineColor(sf::Color::Black);


    sf::Clock clk;
    while (win.isOpen()) {
        for (sf::Event e; win.pollEvent(e); )
            if (e.type == sf::Event::Closed) win.close();

        float dt = clk.restart().asSeconds();
        world.update(dt);

        win.clear(sf::Color(40,100,20));

        // draw food
        for (const auto& obj : world.objects()) {
            draw_object_circle(
                win,
                objectShape,
                obj->pos(),
                obj->size(),
                obj->color()
            );
        }

        for (const auto& b : world.byts()) {
            // byt dot 
            bytDot.setPosition(b.pos());
            win.draw(bytDot);

            // DEBUG: sight (green) + hearing (blue) radii
            draw_ring(win, ring, b.pos(), b.senses().sight_range, sight_label, sf::Color(0,255,0,120));
            draw_ring(win, ring, b.pos(), b.senses().hearing_range, hearing_label, sf::Color(0,128,255,120));
            draw_ring(win, ring, b.pos(), b.senses().smell_range, smell_label, sf::Color(255,100,255,100));
            draw_debug_text(win, intent_text, b.pos(), make_byt_debug_text(b));

            if (b.has_idle_anchor()) {
                draw_ring(win, ring, b.idle_anchor(), b.idle_leash_radius(), idle_label, sf::Color(200,200,200,100));
            }

            for (const auto& pos : b.food_memory_positions()) {
                draw_debug_dot(win, debugDot, pos, 3.f, sf::Color::Magenta);
                draw_debug_line(win, b.pos(), pos, sf::Color(255, 0, 255, 60));
            }

        }

        win.display();
    }
}
