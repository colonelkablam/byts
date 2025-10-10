#include <SFML/Graphics.hpp>
#include "World.hpp"

using namespace byts;

int main() {
    const float W = 960.f, H = 540.f;

    sf::RenderWindow win(sf::VideoMode(static_cast<unsigned>(W),
                                       static_cast<unsigned>(H)),
                         "Byts");
    win.setFramerateLimit(120);

    World world{W, H};
    world.spawn(50);

    sf::CircleShape dot(3.f);
    dot.setOrigin(3.f, 3.f);
    dot.setFillColor(sf::Color::White);

    sf::Clock clk;
    while (win.isOpen()) {
        for (sf::Event e; win.pollEvent(e); )
            if (e.type == sf::Event::Closed) win.close();

        float dt = clk.restart().asSeconds();
        world.update(dt);

        win.clear(sf::Color(40,100,20));
        for (const auto& b : world.byts()) {
            dot.setPosition(b.pos());
            win.draw(dot);
        }
        win.display();
    }
}
