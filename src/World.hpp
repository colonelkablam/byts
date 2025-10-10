#pragma once
#include <vector>
#include "Byt.hpp"

namespace byts {

class World {
public:
    World(float w, float h) : w_(w), h_(h) {}
    void spawn(std::size_t n);
    void update(float dt); // builds perceptions → byt.step → integrate → boundary

    const std::vector<Byt>& byts() const { return byts_; }
    float width() const { return w_; }
    float height() const { return h_; }

private:
    float w_, h_;
    std::vector<Byt> byts_;
    void apply_boundaries(Byt& b) const noexcept; // bounce/wrap/etc.
    void build_perception(std::size_t i, Byt::Seconds dt, Perception& out) const;
};

} // namespace byts
