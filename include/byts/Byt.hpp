#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <random>
#include <optional>
#include <cstdint>
#include "SenseMask.hpp"

namespace byts {

class World; // forward declaration

class Byt {
public:
    using Seconds = float;

    Byt(sf::Vector2f p = {0.f, 0.f}, float size = 6.f)
        : size_(size), pos_(p) {}

    enum class Mood { Neutral, Hungry, Lonely };
    enum class Behavior { Wander, Forage, SeekCompanion };
    enum class MemoryKind {
        Food,
        Byt,
        FoodArea,   // later
        Danger,
        Sound
    };
    enum class MemoryNature {
        Object,
        Place
    };
    enum class SenseSource : std::uint8_t {
        None    = 0,
        Sight   = 1 << 0,
        Hearing = 1 << 1,
        Smell   = 1 << 2
    };
    enum class Intention {
        Idle,
        SearchFood,
        MoveToFoodVisible,
        MoveToFoodMemory,
        SeekCompanion
    };

    struct Memory {
        MemoryKind kind;
        MemoryNature nature;

        std::optional<std::size_t> object_id; // only for Object

        sf::Vector2f pos{};

        float confidence = 0.f;
        float age = 0.f;
        float ttl = 5.f;

        std::uint8_t sources = 0; // bitmask (Sight, Smell, etc.)
    };

    struct Brain {
        float stored_energy = 1.f;   // 0..1
        float social_need   = 0.f;   // 0..1

        float hunger_drive  = 0.f;   // 0..1
        float social_drive  = 0.f;   // 0..1

        float hunger_on     = 0.6f;
        float critical_on   = 0.2f;
        float social_on     = 0.6f;

        float energy_drain_per_sec = 0.02f;
        float food_energy_gain     = 0.35f;
        float social_rise_per_sec  = 0.03f;
    };

    struct SensesConfig {
        float sight_range     = 120.f;
        float hearing_range   = 80.f;
        float smell_range     = 220.f;

        Seconds sight_cadence   = 0.05f;
        Seconds hearing_cadence = 0.20f;
        Seconds smell_cadence   = 0.15f;
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
    struct Smelled {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
    };

    // tweak gains externally?
    struct SteeringGains {
       float personal_space = 150.f;
       float idle = 100.f;
       float search_food = 180.f;
       float visible_food = 400.f;
       float memory_food = 320.f;
       float companion = 35.f;
    };
    SteeringGains& gains() noexcept { return gains_; }
    const SteeringGains& gains() const noexcept { return gains_; }

    // debug access
    float energy() const noexcept { return brain_.stored_energy; }

    void add_energy(float amount) noexcept;
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
    const std::vector<Smelled>& smelled() const noexcept { return smelled_; }
    SensesConfig& senses() noexcept { return cfg_; }
    const SensesConfig& senses() const noexcept { return cfg_; }
    float size() const noexcept { return size_; }

private:
    float size_ = 6.f;
    std::size_t  id_{0};
    sf::Vector2f pos_{0.f, 0.f};
    sf::Vector2f vel_{0.f, 0.f};
    sf::Vector2f wander_dir_{1.f, 0.f};
    float        max_speed_ = 20.f;
    Seconds wander_change_timer_{0.f};
    std::mt19937 rng_{0xB17}; // seed updated via set_id()

    // steering config
    SteeringGains gains_;

    // —— state ——
    Brain     brain_;
    Intention intention_{Intention::Idle};

    // Sense state/buffers
    SensesConfig     cfg_;
    Seconds          sight_timer_{0.f};
    Seconds          hearing_timer_{0.f};
    Seconds          smell_timer_{0.f};
    std::vector<Seen>  seen_;
    std::vector<Heard> heard_;
    std::vector<Smelled> smelled_;

    std::vector<Memory> memories_;

    // helpers
    void update_memories(float dt);
    Memory* find_matching_memory(MemoryKind kind,
                                  std::optional<std::size_t> object_id,
                                  sf::Vector2f pos,
                                  float merge_radius);

    void add_or_update_memory(MemoryKind kind,
                               std::optional<std::size_t> object_id,
                               sf::Vector2f pos,
                               std::uint8_t source_flag,
                               float confidence_boost,
                               float ttl,
                               float merge_radius);
    void update_internal_needs(Seconds dt);
    void choose_intention();

    sf::Vector2f steer_idle(Seconds dt);
    sf::Vector2f steer_search_food(Seconds dt);
    sf::Vector2f steer_to_visible_food() const;
    sf::Vector2f steer_to_food_memory(Seconds dt);
    sf::Vector2f steer_to_companion(Seconds dt);
    
    sf::Vector2f steer_towards(sf::Vector2f from, sf::Vector2f to, float strength) const noexcept;};

} // namespace byts
