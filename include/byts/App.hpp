#pragma once

#include <SFML/Graphics.hpp>
#include "byts/World.hpp"
#include "byts/render/RenderResources.hpp"

namespace byts {

class App {
public:
    App(float width = 1280.f, float height = 1024.f);
    void run();

private:
    void process_events();
    void update_world(float dt);
    void render();

private:
    float width_;
    float height_;
    bool show_debug_ = true;

    sf::RenderWindow window_;
    sf::Clock clock_;

    World world_;
    render::RenderResources render_;
};

} // namespace byts