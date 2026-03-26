#pragma once

#include <SFML/System/Vector2.hpp>

namespace SoccerPool {

enum class Team { None, Team1, Team2 };

class Piece {
public:
    Piece(int id, Team team);

    int getId() const { return id_; }
    Team getTeam() const { return team_; }
    sf::Vector2f getPosition() const { return position_; }
    sf::Vector2f getVelocity() const { return velocity_; }
    void setPosition(sf::Vector2f p) { position_ = p; }
    void setVelocity(sf::Vector2f v) { velocity_ = v; }

    void update(float dt);
    void applyFriction(float dt);
    bool isMoving() const;

private:
    int id_;
    Team team_;
    sf::Vector2f position_;
    sf::Vector2f velocity_;
};

} // namespace SoccerPool
