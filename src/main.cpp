#include <SFML/Graphics.hpp>
#include "byts/World.hpp"

using namespace byts;

static void draw_ring(sf::RenderWindow& win, sf::Vector2f center, float r, sf::Color outline) {
    sf::CircleShape ring;
    ring.setPointCount(64);
    ring.setRadius(r);
    ring.setOrigin(r, r);
    ring.setPosition(center);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(1.f);
    ring.setOutlineColor(outline);
    win.draw(ring);
}

static void draw_energy_text(sf::RenderWindow& win,
                             sf::Text& text,
                             sf::Vector2f pos,
                             float energy)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.2f", energy);
    text.setString(buf);
    text.setPosition(pos.x + 6.f, pos.y - 10.f);
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

int main() {
    const float W = 960.f, H = 540.f;
    sf::RenderWindow win(sf::VideoMode((unsigned)W,(unsigned)H), "Byts debug");
    win.setFramerateLimit(120);

    sf::Font font;
    if (!font.loadFromFile("assets/DejaVuSans.ttf")) {
        return 1;
    }
    sf::Text energyText;
    energyText.setFont(font);
    energyText.setCharacterSize(10);
    energyText.setFillColor(sf::Color::White);

    World world{W, H};
    world.spawn_byts(5);
    world.spawn_food(12, 0.35f);
    
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

        for (const auto& b : world.byts()) {
            // byt dot
            bytDot.setPosition(b.pos());
            win.draw(bytDot);

            // DEBUG: sight (green) + hearing (blue) radii
            draw_ring(win, b.pos(), b.senses().sight_range,   sf::Color(0,255,0,120));
            draw_ring(win, b.pos(), b.senses().hearing_range, sf::Color(0,128,255,120));
            draw_energy_text(win, energyText, b.pos(), b.energy());
        }

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

        win.display();
    }
}
