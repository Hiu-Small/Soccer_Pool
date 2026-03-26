#pragma once

#include <SFML/System/Vector2.hpp>

namespace SoccerPool {

class Ball {
public:
    Ball();

    sf::Vector2f getPosition() const { return position_; }
    sf::Vector2f getVelocity() const { return velocity_; }
    void setPosition(sf::Vector2f p) { position_ = p; }
    void setVelocity(sf::Vector2f v) { velocity_ = v; }

    void update(float dt);
    void applyFriction(float dt);
    bool isMoving() const;

private:
    sf::Vector2f position_;
    sf::Vector2f velocity_;
};

} // namespace SoccerPool
