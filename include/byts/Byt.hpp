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

    Byt(sf::Vector2f p = {0.f, 0.f}, float size = 10.f)
        : size_(size), pos_(p), rng_(std::random_device{}()){}

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
        SeekFoodSmell,
        MoveToFoodVisible,
        MoveToFoodMemory,
        SeekCompanion,
        Count
    };

    struct ColourRGB {
        uint8_t r, g, b;
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
        // internal state
        float stored_energy = 1.f;   // 0..1
        float social_need   = 0.f;   // 0..1

        float hunger_drive  = 0.f;   // 0..1
        float social_drive  = 0.f;   // 0..1

        // internal dynamics
        float energy_drain_per_sec = 0.02f;
        float social_rise_per_sec  = 0.03f;
    };

    struct BehaviourConfig {
        float hunger_on = 0.2f;
        float critical_on = 0.2f;
        float social_on = 0.6f;

        float smell_interest_threshold = 0.15f;
        float memory_confidence_threshold = 0.2f;
        float danger_flee_threshold = 0.8f;
    };
    struct SensesConfig {
        float sight_range     = 120.f;
        float hearing_range   = 80.f;
        float smell_range     = 400.f;

        Seconds sight_cadence   = 0.05f;
        Seconds hearing_cadence = 0.20f;
        Seconds smell_cadence   = 0.15f;
    };
    struct SteeringConfig {
        float personal_space = 150.f;
        float edge_avoid = 220.f;
    };
    struct IdleConfig {
        float anchor_pull_strength = 35.f;

        float max_turn_angle = 0.9f;
        Seconds dir_time_min = 0.4f;
        Seconds dir_time_max = 1.0f;

        float dead_zone_radius = 6.f;
        float leash_radius = 30.f;
    };
    struct SearchConfig {
        Seconds leg_time_min = 1.2f;
        Seconds leg_time_max = 3.0f;

        float turn_bias = 0.7f;
        float turn_noise = 0.15f;
        float flip_chance = 0.30f;
        float reorient_angle = 0.5f;
    };

    struct SenseState {
        Seconds sight_timer = 0.f;
        Seconds hearing_timer = 0.f;
        Seconds smell_timer = 0.f;
    };
    struct SmellState {
        sf::Vector2f dir{1.f, 0.f};

        float food_strength = 0.f;        // current sensed value (from world)
        float prev_food_strength = 0.f;   // last sampled value

        float probe_timer = 0.f;
        float probe_duration = 1.f;

        float turn_angle = 0.4f;
        float turn_sign = 1.f;

        int fail_count = 0;

        bool sample_updated = false;
    };
    
    struct IdleState {
        sf::Vector2f anchor{0.f, 0.f};
        sf::Vector2f jiggle_dir{1.f, 0.f};

        bool anchor_set = false;

        Seconds direction_timer = 0.f;
        Seconds pause_timer = 0.f;
    };
    struct SearchState {
        sf::Vector2f dir{1.f, 0.f};

        Seconds leg_timer = 0.f;
        float turn_sign = 1.f;
    };

    struct Seen {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
        float visual_strength;
    };
    struct Heard {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
        float loudness;
    
    };
    struct Smelled {
        ObjectKind   kind;
        sf::Vector2f pos;
        float        distance;
        std::size_t  id;
        float smell_strength;
    };
    
    SteeringConfig& steering() noexcept { return steering_; }
    const SteeringConfig& steering() const noexcept { return steering_; }

    IdleConfig& idle_config() noexcept { return idle_config_; }
    const IdleConfig& idle_config() const noexcept { return idle_config_; }

    SearchConfig& search_config() noexcept { return search_config_; }
    const SearchConfig& search_config() const noexcept { return search_config_; }

    // debug access
    float energy() const noexcept { return brain_.stored_energy; }
    const char* intent() const noexcept {
        return intent_to_string(intention_);
    }
    float food_smell_strength() const noexcept { return smell_.food_strength; }
    float food_smell_threshold() const noexcept { return behaviour_.smell_interest_threshold; }
    std::vector<sf::Vector2f> food_memory_positions() const;
    sf::Vector2f idle_anchor() const noexcept { return idle_.anchor; }
    bool has_idle_anchor() const noexcept { return idle_.anchor_set; }
    float idle_leash_radius() const noexcept { return idle_config_.leash_radius; }

    void add_energy(float amount) noexcept;
    void sense_update(const World& world, Seconds dt);
    void step(Seconds dt, sf::Vector2f world_size) noexcept;
    void integrate(Seconds dt) noexcept; // clamp speed, etc.

    // Accessors
    const sf::Vector2f& pos() const noexcept { return pos_; }
    const sf::Vector2f& vel() const noexcept { return vel_; }
    void set_pos(sf::Vector2f p) noexcept { pos_ = p; }
    void set_vel(sf::Vector2f v) noexcept { vel_ = v; }

    void set_id(std::size_t id) noexcept;
    std::size_t id() const noexcept { return id_; }

    const ColourRGB& colour() const noexcept { return colour_; }

    const std::vector<Seen>&  seen()  const noexcept { return seen_; }
    const std::vector<Heard>& heard() const noexcept { return heard_; }
    const std::vector<Smelled>& smelled() const noexcept { return smelled_; }
    SensesConfig& senses() noexcept { return senses_; }
    const SensesConfig& senses() const noexcept { return senses_; }
    float size() const noexcept { return size_; }

private:
    float size_{10.f};
    std::size_t  id_{0};
    sf::Vector2f pos_{0.f, 0.f};
    sf::Vector2f vel_{0.f, 0.f};
    sf::Vector2f heading_{0.f, 0.f}; // memory of direction

    float        max_speed_ = 20.f;
    const float edge_margin = 40.f;

    std::mt19937 rng_;

    // config
    BehaviourConfig behaviour_;
    SensesConfig    senses_;
    SteeringConfig steering_;
    IdleConfig idle_config_;
    SearchConfig search_config_;

    // state
    Brain     brain_;
    Intention intention_{Intention::Idle};
    SmellState smell_;
    SenseState sense_;
    ColourRGB colour_ {100, 20, 20};
    
    IdleState  idle_;
    SearchState search_;

    // Sense memory/buffers
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
    void exit_intention();
    void enter_intention(Intention next);
    void transition_to(Intention next);
    static const char* intent_to_string(Intention i);
    float action_strength() const noexcept;

    sf::Vector2f steer_away_from_edges(sf::Vector2f world_size) const;
    sf::Vector2f steer_away_from_other_byts() const;
    sf::Vector2f current_heading() const noexcept;

    sf::Vector2f steer_idle(Seconds dt);
    sf::Vector2f steer_search_food(Seconds dt);
    sf::Vector2f steer_follow_food_smell(Seconds dt);
    sf::Vector2f steer_to_visible_food() const;
    sf::Vector2f steer_to_food_memory(Seconds dt);
    sf::Vector2f steer_to_companion(Seconds dt);
    
    sf::Vector2f steer_towards(sf::Vector2f from, sf::Vector2f to, float strength) const noexcept;};

} // namespace byts
