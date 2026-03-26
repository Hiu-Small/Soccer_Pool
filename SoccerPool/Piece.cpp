#include "Piece.h"
#include "Constants.h"
#include <cmath>

namespace SoccerPool {

Piece::Piece(int id, Team team) : id_(id), team_(team), position_(0.f, 0.f), velocity_(0.f, 0.f) {}

void Piece::update(float dt) {
    position_.x += velocity_.x * dt;
    position_.y += velocity_.y * dt;
    applyFriction(dt);
}

void Piece::applyFriction(float dt) {
    velocity_.x *= std::pow(FRICTION, dt * 60.f);
    velocity_.y *= std::pow(FRICTION, dt * 60.f);
    float speedSq = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
    if (speedSq < MIN_VELOCITY_PIECE * MIN_VELOCITY_PIECE) {
        velocity_.x = 0.f;
        velocity_.y = 0.f;
    }
}

bool Piece::isMoving() const {
    return velocity_.x != 0.f || velocity_.y != 0.f;
}

} // namespace SoccerPool
