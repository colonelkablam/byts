#pragma once
#include <cstdint>

namespace byts {

// What an object can be detected by
enum class SenseMask : uint8_t {
    None    = 0,
    Visible = 1 << 0,
    Audible = 1 << 1,
    Smell   = 1 << 2,
    Tactile = 1 << 3,
};

inline SenseMask operator|(SenseMask a, SenseMask b) {
    return static_cast<SenseMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline SenseMask& operator|=(SenseMask& a, SenseMask b) {
    a = a | b; return a;
}
inline bool has(SenseMask mask, SenseMask flag) {
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
}

// Simple classification of objects in the world
enum class ObjectKind : uint8_t { Byt, Food, Wall, Other };

// Lightweight record returned from world queries
struct Perceived {
    ObjectKind   kind;
    float        distance; // precomputed for convenience
    std::size_t  id;       // stable id (unique per object type pool)
    struct { float x, y; } pos; // avoid pulling SFML into headers that don't need it
    float visual_strength = 0.f;
    float auditory_strength = 0.f;
    float olfactory_strength = 0.f;
};

} // namespace byts
