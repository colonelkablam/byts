#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <random>
#include "Sense.hpp"

namespace byts {

class World; // forward declaration

class Byt {
public:
    using Seconds = float;

    enum class Mood { Neutral, Hungry, Lonely };
    enum class Behavior { Wander, Forage, SeekCompanion };

    struct Brain {
        float stored_energy = 1.f;   // 0..1
        float social_need   = 0.f;   // 0..1

        float hunger_on     = 0.6f;
        float critical_on   = 0.2f;
        float social_on     = 0.6f;

        float energy_drain_per_sec = 0.02f;
        float food_energy_gain     = 0.35f;
    };

    struct SensesConfig {
        float sight_range    = 120.f;
        float hearing_range  = 180.f;
        Seconds sight_cadence   = 0.05f; // 20 Hz
        Seconds hearing_cadence = 0.20f; // 5 Hz
    };

    struct Seen {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
    };
    struct Heard {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
    };

    // tweak gains externally?
    struct SteeringGains { float personal_space=150.f, wander=100.f, forage=400.f, companion=35.f; };
    SteeringGains& gains() noexcept { return gains_; }
    const SteeringGains& gains() const noexcept { return gains_; }

    // debug access
    Mood mood() const noexcept { return mood_; }
    Behavior behavior() const noexcept { return behavior_; }
    float energy() const noexcept { return brain_.stored_energy; }

    explicit Byt(sf::Vector2f p) : pos_(p) {}

    void sense_update(const World& world, Seconds dt);
    void step(Seconds dt) noexcept;       // decide/accelerate based on seen_/heard_
    void integrate(Seconds dt) noexcept;  // pos += vel * dt, clamp speed, etc.

    // Accessors
    const sf::Vector2f& pos() const noexcept { return pos_; }
    const sf::Vector2f& vel() const noexcept { return vel_; }
    void set_pos(sf::Vector2f p) noexcept { pos_ = p; }
    void set_vel(sf::Vector2f v) noexcept { vel_ = v; }

    void set_id(std::size_t id) noexcept;
    std::size_t id() const noexcept { return id_; }

    const std::vector<Seen>&  seen()  const noexcept { return seen_; }
    const std::vector<Heard>& heard() const noexcept { return heard_; }
    SensesConfig& senses() noexcept { return cfg_; }
    const SensesConfig& senses() const noexcept { return cfg_; }

private:
    std::size_t  id_{0};
    sf::Vector2f pos_{0.f, 0.f};
    sf::Vector2f vel_{0.f, 0.f};
    float        max_speed_ = 60.f;

    std::mt19937 rng_{0xB17}; // seed updated via set_id()

    // steering config
    SteeringGains gains_;

    // —— state ——
    Mood      mood_{Mood::Neutral};        // hard-coded neutral for now
    Behavior  behavior_{Behavior::Wander}; // derived from mood_
    Brain     brain_;

    // Sense state/buffers
    SensesConfig     cfg_;
    Seconds          sight_timer_{0.f};
    Seconds          hearing_timer_{0.f};
    std::vector<Seen>  seen_;
    std::vector<Heard> heard_;

    // helpers
    void decide_mood();
    void decide_behavior();                 // mood → behavior
    sf::Vector2f do_wander(float dt);
    sf::Vector2f do_forage(float dt);       // needs Food in world to shine
    sf::Vector2f do_seek_companion(float dt);
    bool is_hungry() const noexcept { return brain_.stored_energy <= brain_.hunger_on; }
};

} // namespace byts
