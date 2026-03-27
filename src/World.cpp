#include "byts/World.hpp"
#include <random>
#include <cmath>
#include <memory>


namespace byts {

namespace {
inline float len2(sf::Vector2f v) noexcept { return v.x*v.x + v.y*v.y; }
} // namespace

void World::query_in_radius(sf::Vector2f center,
                            float radius,
                            SenseMask sense,
                            std::vector<Perceived>& out) const
{
    out.clear();
    const float r2 = radius * radius;

    auto falloff = [&](float distance) -> float {
        float f = 1.f - (distance / radius);
        return (f < 0.f) ? 0.f : f;
    };

    auto consider = [&](ObjectKind kind,
                        std::size_t id,
                        sf::Vector2f pos,
                        SenseMask perceptible_by,
                        float visual_emission,
                        float loudness_emission,
                        float smell_emission)
    {
        if (!has(perceptible_by, sense)) return;

        sf::Vector2f d{ pos.x - center.x, pos.y - center.y };
        float d2 = len2(d);
        if (d2 > r2) return;

        float distance = std::sqrt(d2);
        float f = falloff(distance);

        Perceived p{
            kind,
            distance,
            id,
            { pos.x, pos.y },
            0.f, // visual_strength
            0.f, // loudness
            0.f  // smell_strength
        };

        if (sense == SenseMask::Visible) {
            p.visual_strength = visual_emission * f;
        }
        else if (sense == SenseMask::Audible) {
            p.auditory_strength = loudness_emission * f;
        }
        else if (sense == SenseMask::Smell) {
            p.olfactory_strength = smell_emission * f;
        }

        out.push_back(p);
    };

    for (std::size_t i = 0; i < byts_.size(); ++i) {
        consider(ObjectKind::Byt,
                 byts_[i].id(),
                 byts_[i].pos(),
                 SenseMask::Visible | SenseMask::Audible,
                 1.f,   // visual emission
                 0.4f,  // loudness emission for now
                 0.f);  // smell emission for now
    }

    for (const auto& obj : objects_) {
        consider(obj->kind(),
                 obj->id(),
                 obj->pos(),
                 obj->perceptible_by(),
                 obj->visibility(),
                 obj->loudness(),
                 obj->smell());
    }
}

std::size_t World::add_object(ObjectKind kind,
                              sf::Vector2f pos,
                              SenseMask perceptible_by,
                              bool solid,
                              float size,
                              sf::Color color)
{
    std::size_t id = next_object_id_++;
    objects_.push_back(
        std::make_unique<WorldObject>(
            kind,
            pos,
            perceptible_by,
            solid,
            size,
            color,
            id
        )
    );
    return id;
}

std::size_t World::add_food(sf::Vector2f pos, float energy, float scent, float size) {

    std::size_t id = next_object_id_++;

    objects_.push_back(
        std::make_unique<FoodObject>(pos, id, energy, scent, size)
    );

    return id;
}

void World::spawn_byts(std::size_t n) {
    std::mt19937 rng(22345u);
    std::uniform_real_distribution<float> dx(0.f, w_), dy(0.f, h_);

    const std::size_t start = byts_.size();
    byts_.reserve(start + n);

    for (std::size_t i = 0; i < n; ++i) {
        Byt b({ dx(rng), dy(rng) });
        b.set_id(start + i + 1);
        byts_.push_back(b);
    }
}

void World::spawn_food(std::size_t n, float energy, float scent, float size) {
    std::mt19937 rng(54321u);
    std::uniform_real_distribution<float> dx(0.f, w_), dy(0.f, h_);

    for (std::size_t i = 0; i < n; ++i) {
        add_food({ dx(rng), dy(rng) }, energy, scent, size);
    }
}

void World::update(float dt) {
    // 1) Sensing
    for (auto& b : byts_) b.sense_update(*this, dt);

    // 2) Decision/behaviour
    for (auto& b : byts_) b.step(dt, {w_, h_});

    // 3) Integrate & boundaries
    for (auto& b : byts_) {
        b.integrate(dt);
        apply_boundaries(b);
    }

        resolve_food_consumption();

}

void World::apply_boundaries(Byt& b) const noexcept {
    auto p = b.pos();
    auto v = b.vel();

    // Simple bounce within [0,w_]x[0,h_]
    if (p.x < 0.f) { p.x = 0.f; v.x = std::abs(v.x); }
    if (p.x > w_)  { p.x = w_;  v.x = -std::abs(v.x); }
    if (p.y < 0.f) { p.y = 0.f; v.y = std::abs(v.y); }
    if (p.y > h_)  { p.y = h_;  v.y = -std::abs(v.y); }

    b.set_pos(p);
    b.set_vel(v);
}

void World::resolve_food_consumption()
{
    for (auto& byt : byts_) {

        const auto& pos = byt.pos();
        float byt_size = byt.size();

        for (auto it = objects_.begin(); it != objects_.end(); ) {

            WorldObject* obj = it->get();

            if (obj->kind() != ObjectKind::Food) {
                ++it;
                continue;
            }

            auto* food = static_cast<FoodObject*>(obj);

            sf::Vector2f delta{
                food->pos().x - pos.x,
                food->pos().y - pos.y
            };

            float dist2 = delta.x * delta.x + delta.y * delta.y;
            float touch = byt_size + food->size();

            if (dist2 <= touch * touch) {

                byt.add_energy(food->energy());

                it = objects_.erase(it);   // food disappears

                break;                     // byt eats only one food per update
            }

            ++it;
        }
    }
}

} // namespace byts
