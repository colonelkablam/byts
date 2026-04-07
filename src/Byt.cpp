#include "byts/Byt.hpp"
#include "byts/World.hpp"
#include <cmath>
#include <algorithm>
#include <random>

namespace byts {

namespace {
inline float len2(sf::Vector2f v) noexcept { return v.x*v.x + v.y*v.y; }
inline sf::Vector2f clamp_speed(sf::Vector2f v, float max_speed) noexcept {
    const float max2 = max_speed * max_speed;
    float L2 = len2(v);
    if (L2 <= max2) return v;
    float inv = max_speed / std::sqrt(L2);
    return { v.x * inv, v.y * inv };
}
inline sf::Vector2f rotate_vector(sf::Vector2f v, float angle) noexcept {
    float cs = std::cos(angle);
    float sn = std::sin(angle);
    return {
        v.x * cs - v.y * sn,
        v.x * sn + v.y * cs
    };
}
} // namespace

void Byt::set_id(std::size_t id) noexcept {
    id_ = id;
    std::seed_seq seed{ static_cast<unsigned>(0xB17), static_cast<unsigned>(id_) };
    rng_.seed(seed);
    std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);
    float a = angle_dist(rng_);
    heading_ = { std::cos(a), std::sin(a) };

    if (intention_ == Intention::Idle) {
        idle_.anchor = pos_;
        idle_.anchor_set = true;
        idle_.direction_timer = 0.f;
        idle_.jiggle_dir = heading_;
    }
}

void Byt::sense_update(const World& world, Seconds dt) {
    sense_.sight_timer   += dt;
    sense_.hearing_timer += dt;
    sense_.smell_timer   += dt;

    // SIGHT
    if (sense_.sight_timer >= senses_.sight_cadence) {
        sense_.sight_timer = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, senses_.sight_range, SenseMask::Visible, buf);

        seen_.clear();
        seen_.reserve(buf.size());

        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;

            seen_.push_back(Seen{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id, it.visual_strength});

            if (it.kind == ObjectKind::Food) {
                add_or_update_memory(
                    MemoryKind::Food,
                    it.id,
                    {it.pos.x, it.pos.y},
                    static_cast<std::uint8_t>(SenseSource::Sight),
                    0.6f,
                    10.f,
                    20.f
                );
            }
        }
    }

    // HEARING
    if (sense_.hearing_timer >= senses_.hearing_cadence) {
        sense_.hearing_timer = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, senses_.hearing_range, SenseMask::Audible, buf);

        heard_.clear();
        heard_.reserve(buf.size());

        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;

            heard_.push_back(Heard{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id, it.auditory_strength});
        }
    }

    // SMELL
    if (sense_.smell_timer >= senses_.smell_cadence) {
        sense_.smell_timer = 0.f;
        std::vector<Perceived> buf;
        world.query_in_radius(pos_, senses_.smell_range, SenseMask::Smell, buf);

        smelled_.clear();
        smelled_.reserve(buf.size());

        float food_smell_strength = 0.f;

        for (const auto& it : buf) {
            if (it.kind == ObjectKind::Byt && it.id == id_) continue;

            smelled_.push_back(
                Smelled{ it.kind, {it.pos.x, it.pos.y}, it.distance, it.id, it.olfactory_strength }
            );

            if (it.kind == ObjectKind::Food) {
                food_smell_strength += it.olfactory_strength;
            }
        }

        smell_.food_strength = food_smell_strength;
        smell_.sample_updated = true;
    }
}

Byt::Memory* Byt::find_matching_memory(MemoryKind kind,
                                       std::optional<std::size_t> object_id,
                                       sf::Vector2f pos,
                                       float merge_radius)
{
    if (object_id.has_value()) {
        for (auto& m : memories_) {
            if (m.kind == kind && m.object_id == object_id) {
                return &m;
            }
        }
    }

    float best_dist2 = merge_radius * merge_radius;
    Memory* best = nullptr;

    for (auto& m : memories_) {
        if (m.kind != kind) continue;
        if (m.object_id.has_value()) continue;

        sf::Vector2f d{ m.pos.x - pos.x, m.pos.y - pos.y };
        float dist2 = d.x*d.x + d.y*d.y;

        if (dist2 < best_dist2) {
            best_dist2 = dist2;
            best = &m;
        }
    }

    return best;
}

void Byt::add_or_update_memory(MemoryKind kind,
                               std::optional<std::size_t> object_id,
                               sf::Vector2f pos,
                               std::uint8_t source_flag,
                               float confidence_boost,
                               float ttl,
                               float merge_radius)
{
    Memory* m = find_matching_memory(kind, object_id, pos, merge_radius);

    if (m) {
        // --- UPDATE EXISTING ---
        m->age = 0.f;
        m->ttl = std::max(m->ttl, ttl);

        // Confidence accumulates but clamps
        m->confidence = std::min(1.f, m->confidence + confidence_boost);

        // Position update
        if (object_id.has_value()) {
            // precise → overwrite
            m->pos = pos;
        } else {
            // fuzzy → blend
            float alpha = 0.3f;
            m->pos.x = m->pos.x * (1.f - alpha) + pos.x * alpha;
            m->pos.y = m->pos.y * (1.f - alpha) + pos.y * alpha;
        }

        m->sources |= source_flag;
    }
    else {
        // --- CREATE NEW ---
        Memory mem;
        mem.kind = kind;
        mem.nature = object_id.has_value() ? MemoryNature::Object
                                           : MemoryNature::Place;
        mem.object_id = object_id;
        mem.pos = pos;
        mem.confidence = confidence_boost;
        mem.age = 0.f;
        mem.ttl = ttl;
        mem.sources = source_flag;

        memories_.push_back(mem);
    }
}

void Byt::update_memories(float dt) {
    for (auto it = memories_.begin(); it != memories_.end(); ) {
        it->age += dt;

        // decay confidence
        it->confidence -= dt * 0.1f;

        if (it->age > it->ttl || it->confidence <= 0.f) {
            it = memories_.erase(it);
        } else {
            ++it;
        }
    }
}

void Byt::update_internal_needs(Seconds dt) {
    brain_.stored_energy -= brain_.energy_drain_per_sec * dt;
    if (brain_.stored_energy < 0.f) {
        brain_.stored_energy = 0.f;
    }

    brain_.social_need += brain_.social_rise_per_sec * dt;
    if (brain_.social_need > 1.f) {
        brain_.social_need = 1.f;
    }

    brain_.hunger_drive = 1.f - brain_.stored_energy;
    brain_.social_drive = brain_.social_need;
}

void Byt::choose_intention() {
    bool sees_food = false;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Food) {
            sees_food = true;
            break;
        }
    }

    bool has_food_memory = false;
    for (const auto& m : memories_) {
        if (m.kind == MemoryKind::Food) {
            has_food_memory = true;
            break;
        }
    }

    bool sees_byt = false;
    for (const auto& s : seen_) {
        if (s.kind == ObjectKind::Byt) {
            sees_byt = true;
            break;
        }
    }

    bool smells_food = smell_.food_strength > behaviour_.smell_interest_threshold;

    Intention next = Intention::Idle;

    if (brain_.hunger_drive >= behaviour_.hunger_on) {
        if (sees_food) {
            next = Intention::MoveToFoodVisible;
        } else if (has_food_memory) {
            next = Intention::MoveToFoodMemory;
        } else if (smells_food) {
            next = Intention::SeekFoodSmell;
        } else {
            next = Intention::SearchFood;
        }
    } else if (brain_.social_drive >= behaviour_.social_on && sees_byt) {
        next = Intention::SeekCompanion;
    }

    if (next != intention_) {
        transition_to(next);
    }
}

void Byt::exit_intention() {
    switch (intention_) {
        case Intention::Idle:
            idle_.anchor_set = false;
            idle_.pause_timer = 0.f;
            idle_.direction_timer = 0.f;
            idle_.jiggle_dir = {0.f, 0.f};
            break;

        case Intention::SearchFood:
            search_.leg_timer = 0.f;
            search_.dir = {0.f, 0.f};
            break;

        case Intention::SeekFoodSmell:
            smell_.sample_updated = false;
            smell_.probe_timer = 0.f;
            smell_.dir = {0.f, 0.f};
            break;

        default:
            break;
    }
}

void Byt::enter_intention(Intention next) {
    sf::Vector2f dir = current_heading();

    switch (next) {
        case Intention::Idle:
            idle_.anchor = pos_;
            idle_.anchor_set = true;
            idle_.pause_timer = 0.f;
            idle_.direction_timer = 0.f;
            idle_.jiggle_dir = dir;
            break;

        case Intention::SearchFood:
            search_.dir = dir;
            search_.leg_timer = 0.f; // force new leg setup on first update
            break;

        case Intention::SeekFoodSmell:
            smell_.dir = dir;
            smell_.prev_food_strength = smell_.food_strength;
            smell_.sample_updated = false;
            smell_.probe_timer = 0.f;
            break;

        default:
            break;
    }

    intention_ = next;
}

void Byt::transition_to(Intention next) {
    if (next == intention_) return;
    exit_intention();
    enter_intention(next);
}

const char* Byt::intent_to_string(Intention i) {
    static const char* names[] = {
        "Idle",
        "SearchFood",
        "SeekFoodSmell",
        "MoveToFoodVisible",
        "MoveToFoodMemory",
        "SeekCompanion",
        "Count"
    };

    int idx = static_cast<int>(i);
    if (idx < 0 || idx >= static_cast<int>(Intention::Count))
        return "Unknown";

    return names[idx];
}

float Byt::action_strength() const noexcept {
    switch (intention_) {
        case Intention::Idle:               return 45.f;
        case Intention::SearchFood:         return 180.f;
        case Intention::SeekFoodSmell:      return 200.f;
        case Intention::MoveToFoodVisible:  return 400.f;
        case Intention::MoveToFoodMemory:   return 320.f;
        case Intention::SeekCompanion:      return 35.f;
        default:                            return 100.f;
    }
}

std::vector<sf::Vector2f> Byt::food_memory_positions() const {
    std::vector<sf::Vector2f> result;
    for (const auto& m : memories_) {
        if (m.kind == MemoryKind::Food) {
            result.push_back(m.pos);
        }
    }
    return result;
}

void Byt::add_energy(float amount) noexcept
{
    brain_.stored_energy = std::min(1.f, brain_.stored_energy + amount);
}

void Byt::step(Seconds dt, sf::Vector2f world_size) noexcept {
    update_memories(dt);
    update_internal_needs(dt);
    choose_intention();

    sf::Vector2f force{0.f, 0.f};

    // always-on separation
    sf::Vector2f sep = steer_away_from_other_byts();
    force.x += sep.x;
    force.y += sep.y;

    sf::Vector2f edge_force = steer_away_from_edges(world_size);
    force.x += edge_force.x;
    force.y += edge_force.y;


    sf::Vector2f b{0.f, 0.f};

    switch (intention_) {
        case Intention::Idle:
            b = steer_idle(dt);
            break;
        case Intention::SearchFood:
            b = steer_search_food(dt);
            break;
        case Intention::SeekFoodSmell:
            b = steer_follow_food_smell(dt);
            break;
        case Intention::MoveToFoodVisible:
            b = steer_to_visible_food();
            break;
        case Intention::MoveToFoodMemory:
            b = steer_to_food_memory(dt);
            break;
        case Intention::SeekCompanion:
            b = steer_to_companion(dt);
            break;
    }

    force.x += b.x;
    force.y += b.y;

    vel_.x += force.x * dt;
    vel_.y += force.y * dt;
    vel_ = clamp_speed(vel_, max_speed_);
}

sf::Vector2f Byt::steer_away_from_edges(sf::Vector2f world_size) const {
    sf::Vector2f force{0.f, 0.f};

    // left edge
    if (pos_.x < edge_margin) {
        float t = 1.f - (pos_.x / edge_margin);
        force.x += t * steering_.edge_avoid;
    }

    // right edge
    if (pos_.x > world_size.x - edge_margin) {
        float dist = world_size.x - pos_.x;
        float t = 1.f - (dist / edge_margin);
        force.x -= t * steering_.edge_avoid;
    }

    // top edge
    if (pos_.y < edge_margin) {
        float t = 1.f - (pos_.y / edge_margin);
        force.y += t * steering_.edge_avoid;
    }

    // bottom edge
    if (pos_.y > world_size.y - edge_margin) {
        float dist = world_size.y - pos_.y;
        float t = 1.f - (dist / edge_margin);
        force.y -= t * steering_.edge_avoid;
    }

    return force;
}

sf::Vector2f Byt::steer_away_from_other_byts() const {
    sf::Vector2f force{0.f, 0.f};

    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;

        sf::Vector2f away{ pos_.x - s.pos.x, pos_.y - s.pos.y };
        float d2 = away.x * away.x + away.y * away.y;

        const float min_d2 = 16.f;
        if (d2 < min_d2) d2 = min_d2;

        float inv_d = 1.f / std::sqrt(d2);

        away.x *= inv_d * inv_d;
        away.y *= inv_d * inv_d;

        force.x += away.x * steering_.personal_space;
        force.y += away.y * steering_.personal_space;
    }

    return force;
}

sf::Vector2f Byt::current_heading() const noexcept {
    float v2 = vel_.x * vel_.x + vel_.y * vel_.y;

    if (v2 > 1e-6f) {
        float inv = 1.f / std::sqrt(v2);
        return { vel_.x * inv, vel_.y * inv };
    }

    return heading_; // fallback when basically stopped
}

sf::Vector2f Byt::steer_idle(Seconds dt) {
    // occasionally adjust local drift direction
    idle_.direction_timer -= dt;
    if (idle_.direction_timer <= 0.f) {
        std::uniform_real_distribution<float> angle_dist(
            -idle_config_.max_turn_angle,
             idle_config_.max_turn_angle
        );
        float angle = angle_dist(rng_);

        float cs = std::cos(angle);
        float sn = std::sin(angle);

        sf::Vector2f d = idle_.jiggle_dir;
        idle_.jiggle_dir = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        float L2 = idle_.jiggle_dir.x * idle_.jiggle_dir.x +
                   idle_.jiggle_dir.y * idle_.jiggle_dir.y;

        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            idle_.jiggle_dir.x *= invL;
            idle_.jiggle_dir.y *= invL;
        } else {
            idle_.jiggle_dir = {1.f, 0.f};
        }

        heading_ = idle_.jiggle_dir;

        std::uniform_real_distribution<float> time_dist(
            idle_config_.dir_time_min,
            idle_config_.dir_time_max
        );
        idle_.direction_timer = time_dist(rng_);
    }

    float strength = action_strength();

    // gentle local drift
    sf::Vector2f jiggle = {
        idle_.jiggle_dir.x * strength,
        idle_.jiggle_dir.y * strength
    };

    // pull back toward idle anchor so byt inhabits a local area
    sf::Vector2f to_anchor = {
        idle_.anchor.x - pos_.x,
        idle_.anchor.y - pos_.y
    };

    float dist2 = to_anchor.x * to_anchor.x + to_anchor.y * to_anchor.y;
    sf::Vector2f anchor_pull{0.f, 0.f};

    float dead_zone2 = idle_config_.dead_zone_radius * idle_config_.dead_zone_radius;

    if (dist2 > dead_zone2) {
        float dist = std::sqrt(dist2);
        float inv = 1.f / dist;

        float leash = std::min(1.f, dist / idle_config_.leash_radius);

        anchor_pull = {
            to_anchor.x * inv * idle_config_.anchor_pull_strength * leash,
            to_anchor.y * inv * idle_config_.anchor_pull_strength * leash
        };
    }

    return {
        jiggle.x + anchor_pull.x,
        jiggle.y + anchor_pull.y
    };
}

sf::Vector2f Byt::steer_search_food(Seconds dt) {
    search_.leg_timer -= dt;

    // when a search leg ends, set up the next one
    if (search_.leg_timer <= 0.f) {
        std::uniform_real_distribution<float> leg_time_dist(
            search_config_.leg_time_min,
            search_config_.leg_time_max
        );
        search_.leg_timer = leg_time_dist(rng_);

        // sometimes flip sweep direction
        std::uniform_real_distribution<float> flip_roll(0.f, 1.f);
        if (flip_roll(rng_) < search_config_.flip_chance) {
            search_.turn_sign *= -1.f;
        }

        // occasional reorientation between legs
        std::uniform_real_distribution<float> reorient_dist(
            -search_config_.reorient_angle,
             search_config_.reorient_angle
        );
        float angle = reorient_dist(rng_);

        float cs = std::cos(angle);
        float sn = std::sin(angle);

        sf::Vector2f d = search_.dir;
        search_.dir = {
            d.x * cs - d.y * sn,
            d.x * sn + d.y * cs
        };

        float L2 = search_.dir.x * search_.dir.x + search_.dir.y * search_.dir.y;
        if (L2 > 1e-6f) {
            float invL = 1.f / std::sqrt(L2);
            search_.dir.x *= invL;
            search_.dir.y *= invL;
        } else {
            search_.dir = current_heading();
        }
    }

    // while on the leg, keep applying a gentle sweep turn
    std::uniform_real_distribution<float> noise_dist(
        -search_config_.turn_noise,
         search_config_.turn_noise
    );

    float turn_amount =
        (search_config_.turn_bias * search_.turn_sign + noise_dist(rng_)) * dt;

    float cs = std::cos(turn_amount);
    float sn = std::sin(turn_amount);

    sf::Vector2f d = search_.dir;
    search_.dir = {
        d.x * cs - d.y * sn,
        d.x * sn + d.y * cs
    };

    float L2 = search_.dir.x * search_.dir.x + search_.dir.y * search_.dir.y;
    if (L2 > 1e-6f) {
        float invL = 1.f / std::sqrt(L2);
        search_.dir.x *= invL;
        search_.dir.y *= invL;
    } else {
        search_.dir = {1.f, 0.f};
    }

    heading_ = search_.dir;

    float strength = action_strength();

    return {
        search_.dir.x * strength,
        search_.dir.y * strength
    };
}

sf::Vector2f Byt::steer_follow_food_smell(Seconds dt) {
    smell_.probe_timer += dt;

    if (smell_.probe_timer >= smell_.probe_duration && smell_.sample_updated) {
        smell_.sample_updated = false;
        smell_.probe_timer = 0.f;

        float delta = smell_.food_strength - smell_.prev_food_strength;

        if (delta > 0.01f) {
            // smell improved: keep current direction
            smell_.fail_count = 0;
            smell_.turn_angle = std::max(0.15f, smell_.turn_angle * 0.9f);
        } else {
            // smell not better: rotate direction and try again
            smell_.fail_count++;

            if (delta < -0.05f) {
                smell_.turn_angle = std::min(1.2f, smell_.turn_angle + 0.15f);
            }

            smell_.dir = rotate_vector(smell_.dir, smell_.turn_angle * smell_.turn_sign);
            // alternate sides
            smell_.turn_sign *= -1.f;

            // after repeated failures, widen the search
            if (smell_.fail_count >= 4) {
                smell_.dir = rotate_vector(smell_.dir, 1.2f * smell_.turn_sign);
                smell_.fail_count = 0;
                smell_.turn_angle = 0.5f;
            }
        }

        smell_.prev_food_strength = smell_.food_strength;
    }

    heading_ = smell_.dir;
    float strength = action_strength();
    return smell_.dir * strength;
}

sf::Vector2f Byt::steer_to_visible_food() const {
    const Seen* nearest_food = nullptr;

    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Food) continue;

        if (!nearest_food || s.distance < nearest_food->distance) {
            nearest_food = &s;
        }
    }

    if (!nearest_food) {
        return {0.f, 0.f};
    }

    return steer_towards(pos_, nearest_food->pos, action_strength());
}

sf::Vector2f Byt::steer_to_food_memory(Seconds dt) {
    (void)dt;

    const Memory* best_food_memory = nullptr;
    float best_score = -1.f;

    for (const auto& m : memories_) {
        if (m.kind != MemoryKind::Food) continue;

        sf::Vector2f d{ m.pos.x - pos_.x, m.pos.y - pos_.y };
        float dist2 = d.x * d.x + d.y * d.y;

        // prefer confident, nearby memories
        float score = m.confidence / (1.f + std::sqrt(dist2));

        if (!best_food_memory || score > best_score) {
            best_food_memory = &m;
            best_score = score;
        }
    }

    if (!best_food_memory) {
        return {0.f, 0.f};
    }

    sf::Vector2f to{
        best_food_memory->pos.x - pos_.x,
        best_food_memory->pos.y - pos_.y
    };

    float dist2 = to.x * to.x + to.y * to.y;

    const float reached_radius = 12.f;
    if (dist2 <= reached_radius * reached_radius) {
        return {0.f, 0.f};
    }

    return steer_towards(pos_, best_food_memory->pos, action_strength());
}

sf::Vector2f Byt::steer_to_companion(Seconds dt) {
    (void)dt;

    int count = 0;
    sf::Vector2f centre{0.f, 0.f};

    for (const auto& s : seen_) {
        if (s.kind != ObjectKind::Byt) continue;
        ++count;
        centre.x += s.pos.x;
        centre.y += s.pos.y;
    }

    if (count == 0) {
        return {0.f, 0.f};
    }

    centre.x /= static_cast<float>(count);
    centre.y /= static_cast<float>(count);

    return steer_towards(pos_, centre, action_strength());
}

sf::Vector2f Byt::steer_towards(sf::Vector2f from, sf::Vector2f to, float strength) const noexcept {    
    sf::Vector2f d{ to.x - from.x, to.y - from.y };
    float L2 = d.x*d.x + d.y*d.y;
    if (L2 < 1e-6f) return {0.f, 0.f};

    float invL = 1.f / std::sqrt(L2);
    return { d.x * invL * strength, d.y * invL * strength };
}

void Byt::integrate(Seconds dt) noexcept {
    const float drag_per_sec = 1.5f;
    float k = std::exp(-drag_per_sec * dt);
    vel_.x *= k; vel_.y *= k;

    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;
}

} // namespace byts