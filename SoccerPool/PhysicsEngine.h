#pragma once

#include "Ball.h"
#include "Piece.h"
#include "Field.h"
#include <vector>
#include <memory>

namespace SoccerPool {

class PhysicsEngine {
public:
    PhysicsEngine(Field& field);

    void setBall(std::shared_ptr<Ball> ball) { ball_ = ball; }
    void setPieces(std::vector<std::shared_ptr<Piece>>* pieces) { pieces_ = pieces; }

    void update(float dt);

    // Va chạm quân-quân, quân-bóng, tường
    void resolveCollisions();

    // Trả về 1 nếu bóng vào lưới Team1 (bên trái), 2 nếu Team2 (bên phải), 0 không
    int checkGoal() const;

    bool playHitSoundFlag = false;
    bool playCollideSoundFlag = false;

private:
    void resolveWallBall();
    void resolveWallPiece(Piece& p);
    void resolveBallPiece(Ball& b, Piece& p);
    void resolvePiecePiece(Piece& a, Piece& b);
    // --->THÊM HÀM NÀY : Xử lý va chạm với Cột dọc < -- -
        void resolveCircleCollision(sf::Vector2f & pos, sf::Vector2f & vel, float radius, sf::Vector2f postPos, float restitution);
    // ----------------------------------------------------
    static bool circleCircle(sf::Vector2f c1, float r1, sf::Vector2f c2, float r2);
    static void elasticCollision2D(sf::Vector2f p1, sf::Vector2f v1, float m1,
                                   sf::Vector2f p2, sf::Vector2f v2, float m2,
                                   sf::Vector2f& outV1, sf::Vector2f& outV2);

    Field& field_;
    std::shared_ptr<Ball> ball_;
    std::vector<std::shared_ptr<Piece>>* pieces_;
};

} // namespace SoccerPool
