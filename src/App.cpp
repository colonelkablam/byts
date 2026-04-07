#include "byts/App.hpp"

#include <random>
#include <stdexcept>

#include "byts/render/debug_draw.hpp"
#include "byts/render/world_draw.hpp"
#include "byts/render/RenderResources.hpp"

namespace byts {

App::App(float width, float height)
    : width_(width),
      height_(height),
      window_(sf::VideoMode(static_cast<unsigned>(width),
                            static_cast<unsigned>(height)),
              "Byts debug"),
      world_(width, height, std::random_device{}())
{
    window_.setFramerateLimit(120);

    render::init_render_resources(render_);

    world_.spawn_byts(1);
    world_.spawn_food(8, 0.35f, 0.5f, 6.f);
}

void App::run()
{
    while (window_.isOpen()) {
        process_events();

        float dt = clock_.restart().asSeconds();
        update_world(dt);
        render();
    }
}

void App::process_events()
{
    for (sf::Event e; window_.pollEvent(e); ) {
        if (e.type == sf::Event::Closed) {
            window_.close();
        }

        if (e.type == sf::Event::KeyPressed) {
            if (e.key.code == sf::Keyboard::D) {
                show_debug_ = !show_debug_;
            }
        }
    }
}

void App::update_world(float dt)
{
    world_.update(dt);
}

void App::render()
{
    window_.clear(sf::Color(40, 100, 20));

    render::draw_world(window_, world_, render_);

    if (show_debug_) {
        render::draw_debug(window_, world_, render_);
    }

    window_.display();
}


} // namespace byts