#include "Ball.h"
#include "Constants.h"
#include <cmath>

namespace SoccerPool {

Ball::Ball() : position_(FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f), velocity_(0.f, 0.f) {}

void Ball::update(float dt) {
    position_.x += velocity_.x * dt;
    position_.y += velocity_.y * dt;
    applyFriction(dt);
}

void Ball::applyFriction(float dt) {
    velocity_.x *= std::pow(FRICTION, dt * 60.f);
    velocity_.y *= std::pow(FRICTION, dt * 60.f);
    float speedSq = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
    if (speedSq < MIN_VELOCITY_BALL * MIN_VELOCITY_BALL) {
        velocity_.x = 0.f;
        velocity_.y = 0.f;
    }
}

bool Ball::isMoving() const {
    return velocity_.x != 0.f || velocity_.y != 0.f;
}

} // namespace SoccerPool
