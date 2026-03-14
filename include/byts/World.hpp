#pragma once
#include <vector>
#include <memory>
#include "Byt.hpp"
#include "WorldObject.hpp"
#include "FoodObject.hpp"

namespace byts {

class World {
public:
    World(float w, float h) : w_(w), h_(h) {}

    void spawn_byts(std::size_t n);
    void spawn_food(std::size_t n, float energy = 0.35f);
    
    // sense → step → integrate → boundaries
    void update(float dt);

    // Read-only spatial query used by Byts to sense the environment
    void query_in_radius(sf::Vector2f center,
                         float radius,
                         SenseMask sense,
                         std::vector<Perceived>& out) const;

    // Convenience to add a generic world object
    std::size_t add_object(ObjectKind kind,
                           sf::Vector2f pos,
                           SenseMask perceptible_by,
                           bool solid,
                           float size,
                           sf::Color color = sf::Color::White);

    std::size_t add_food(sf::Vector2f pos, float energy);
    
    // Access
    const std::vector<Byt>& byts() const noexcept { return byts_; }
    std::vector<Byt>&       byts()       noexcept { return byts_; }

    float width()  const noexcept { return w_; }
    float height() const noexcept { return h_; }

    const std::vector<std::unique_ptr<WorldObject>>& objects() const noexcept {
        return objects_;
    }

private:
    float w_, h_;
    std::vector<Byt>         byts_;
    std::vector<std::unique_ptr<WorldObject>> objects_;
    std::size_t              next_object_id_{1}; // 0 reserved if needed

    void apply_boundaries(Byt& b) const noexcept; // simple bounce/wrap
};

} // namespace byts
