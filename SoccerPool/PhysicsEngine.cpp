#include "PhysicsEngine.h"
#include "Constants.h"
#include <cmath>
#include <algorithm>

namespace SoccerPool {

PhysicsEngine::PhysicsEngine(Field& field) : field_(field), pieces_(nullptr) {}

void PhysicsEngine::update(float dt) {
    if (ball_) ball_->update(dt);
    if (pieces_)
        for (auto& p : *pieces_) p->update(dt);
    resolveCollisions();
}

bool PhysicsEngine::circleCircle(sf::Vector2f c1, float r1, sf::Vector2f c2, float r2) {
    float dx = c2.x - c1.x, dy = c2.y - c1.y;
    float distSq = dx * dx + dy * dy;
    float sum = r1 + r2;
    return distSq <= sum * sum;
}

void PhysicsEngine::elasticCollision2D(sf::Vector2f p1, sf::Vector2f v1, float m1,
                                       sf::Vector2f p2, sf::Vector2f v2, float m2,
                                       sf::Vector2f& outV1, sf::Vector2f& outV2) {
    sf::Vector2f n = p2 - p1;
    float len = std::sqrt(n.x * n.x + n.y * n.y);
    if (len < 1e-6f) return;
    n.x /= len;
    n.y /= len;
    float v1n = v1.x * n.x + v1.y * n.y;
    float v2n = v2.x * n.x + v2.y * n.y;
    float totalM = m1 + m2;
    float newV1n = (v1n * (m1 - m2) + 2.f * m2 * v2n) / totalM;
    float newV2n = (v2n * (m2 - m1) + 2.f * m1 * v1n) / totalM;
    sf::Vector2f vt1(v1.x - n.x * v1n, v1.y - n.y * v1n);
    sf::Vector2f vt2(v2.x - n.x * v2n, v2.y - n.y * v2n);
    outV1 = sf::Vector2f(vt1.x + n.x * newV1n, vt1.y + n.y * newV1n);
    outV2 = sf::Vector2f(vt2.x + n.x * newV2n, vt2.y + n.y * newV2n);
}

//void PhysicsEngine::resolveWallBall() {
//    if (!ball_) return;
//    auto& b = *ball_;
//    sf::Vector2f p = b.getPosition();
//    sf::Vector2f v = b.getVelocity();
//    float r = BALL_RADIUS;
//
//    if (p.x - r < FIELD_MARGIN_X) {
//        if (field_.isInGoal1(p)) return; // Vào lưới, không nảy
//        b.setPosition(sf::Vector2f(r + FIELD_MARGIN_X, p.y));
//        //b.setVelocity(sf::Vector2f(-v.x * WALL_BOUNCE, v.y * WALL_BOUNCE));
//        b.setVelocity(sf::Vector2f(std::abs(v.x) * WALL_BOUNCE, v.y * WALL_BOUNCE));
//    }
//    if (p.x + r > FIELD_WIDTH - FIELD_MARGIN_X) {
//        if (field_.isInGoal2(p)) return;
//        b.setPosition(sf::Vector2f(FIELD_WIDTH - FIELD_MARGIN_X - r, p.y));
//        //b.setVelocity(sf::Vector2f(-v.x * WALL_BOUNCE, v.y * WALL_BOUNCE));
//        b.setVelocity(sf::Vector2f(-std::abs(v.x) * WALL_BOUNCE, v.y * WALL_BOUNCE));
//    }
//    if (p.y - r < FIELD_MARGIN_Y) {
//        b.setPosition(sf::Vector2f(p.x, FIELD_MARGIN_Y + r));
//        //b.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -v.y * WALL_BOUNCE));
//        b.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, std::abs(v.y) * WALL_BOUNCE));
//    }
//    if (p.y + r > FIELD_HEIGHT - FIELD_MARGIN_Y) {
//        b.setPosition(sf::Vector2f(p.x, FIELD_HEIGHT - FIELD_MARGIN_Y - r));
//        //b.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -v.y * WALL_BOUNCE));
//        b.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -std::abs(v.y) * WALL_BOUNCE));
//    }
//}

void PhysicsEngine::resolveWallBall() {
    if (!ball_) return;
    auto& b = *ball_;
    sf::Vector2f p = b.getPosition();
    sf::Vector2f v = b.getVelocity();
    float r = BALL_RADIUS;
    bool hit = false;

    // Giới hạn biên sân có tính thêm Margin
    float minX = FIELD_MARGIN_X + r;
    float maxX = FIELD_WIDTH - FIELD_MARGIN_X - r;
    float minY = FIELD_MARGIN_Y + r;
    float maxY = FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM - r;

    // Va chạm tường dọc (Trái/Phải)
    if (p.x < minX) {
        if (!field_.isInGoal1(p)) {
            p.x = minX;
            v.x = std::abs(v.x) * WALL_BOUNCE;
            // Nếu vận tốc quá nhỏ, cho nó một cú hích nhẹ để thoát góc
            if (std::abs(v.x) < 20.f) v.x = 30.f;
            hit = true;
        }
    }
    else if (p.x > maxX) {
        if (!field_.isInGoal2(p)) {
            p.x = maxX;
            v.x = -std::abs(v.x) * WALL_BOUNCE;
            if (std::abs(v.x) < 20.f) v.x = -30.f;
            hit = true;
        }
    }

    // Va chạm tường ngang (Trên/Dưới)
    if (p.y < minY) {
        p.y = minY;
        v.y = std::abs(v.y) * WALL_BOUNCE;
        if (std::abs(v.y) < 20.f) v.y = 30.f;
        hit = true;
    }
    else if (p.y > maxY) {
        p.y = maxY;
        v.y = -std::abs(v.y) * WALL_BOUNCE;
        if (std::abs(v.y) < 20.f) v.y = -30.f;
        hit = true;
    }

    if (hit) {
        // Đặc biệt: Nếu bóng ở sát góc (cả 2 trục đều hit)
        // Ta thêm một chút ngẫu nhiên để bóng không bị nảy luẩn quẩn
        b.setPosition(p);
        b.setVelocity(v);
    }
}

//void PhysicsEngine::resolveWallPiece(Piece& p) {
//    sf::Vector2f pos = p.getPosition();
//    sf::Vector2f v = p.getVelocity();
//    float r = PIECE_RADIUS;
//
//    if (pos.x - r < FIELD_MARGIN_X) {
//        p.setPosition(sf::Vector2f(FIELD_MARGIN_X + r, pos.y));
//        //p.setVelocity(sf::Vector2f(-v.x * WALL_BOUNCE, v.y * WALL_BOUNCE));
//        p.setVelocity(sf::Vector2f(std::abs(v.x) * WALL_BOUNCE, v.y * WALL_BOUNCE));
//    }
//    if (pos.x + r > FIELD_WIDTH - FIELD_MARGIN_X) {
//        p.setPosition(sf::Vector2f(FIELD_WIDTH - FIELD_MARGIN_X - r, pos.y));
//        //p.setVelocity(sf::Vector2f(-v.x * WALL_BOUNCE, v.y * WALL_BOUNCE));
//        p.setVelocity(sf::Vector2f(-std::abs(v.x) * WALL_BOUNCE, v.y * WALL_BOUNCE));
//    }
//    if (pos.y - r < FIELD_MARGIN_Y) {
//        p.setPosition(sf::Vector2f(pos.x, FIELD_MARGIN_Y + r));
//        //p.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -v.y * WALL_BOUNCE));
//        p.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, std::abs(v.y) * WALL_BOUNCE));
//    }
//    if (pos.y + r > FIELD_HEIGHT - FIELD_MARGIN_Y) {
//        p.setPosition(sf::Vector2f(pos.x, FIELD_HEIGHT - FIELD_MARGIN_Y - r));
//        //p.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -v.y * WALL_BOUNCE));
//        p.setVelocity(sf::Vector2f(v.x * WALL_BOUNCE, -std::abs(v.y) * WALL_BOUNCE));
//    }
//}

void PhysicsEngine::resolveWallPiece(Piece& p_obj) {
    sf::Vector2f p = p_obj.getPosition();
    sf::Vector2f v = p_obj.getVelocity();
    float r = PIECE_RADIUS;
    bool hit = false;

    if (p.x - r < FIELD_MARGIN_X) {
        p.x = FIELD_MARGIN_X + r;
        v.x = std::abs(v.x) * WALL_BOUNCE;
        hit = true;
    }
    else if (p.x + r > FIELD_WIDTH - FIELD_MARGIN_X) {
        p.x = FIELD_WIDTH - FIELD_MARGIN_X - r;
        v.x = -std::abs(v.x) * WALL_BOUNCE;
        hit = true;
    }

    if (p.y - r < FIELD_MARGIN_Y) {
        p.y = FIELD_MARGIN_Y + r;
        v.y = std::abs(v.y) * WALL_BOUNCE;
        hit = true;
    }
    else if (p.y + r > FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM) {
        p.y = FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM - r;
        v.y = -std::abs(v.y) * WALL_BOUNCE;
        hit = true;
    }

    if (hit) {
        p_obj.setPosition(p);
        p_obj.setVelocity(v);
    }
}

//void PhysicsEngine::resolveBallPiece(Ball& b, Piece& p) {
//    if (!circleCircle(b.getPosition(), BALL_RADIUS, p.getPosition(), PIECE_RADIUS)) return;
//    sf::Vector2f p1 = b.getPosition(), p2 = p.getPosition();
//    sf::Vector2f v1 = b.getVelocity(), v2 = p.getVelocity();
//    sf::Vector2f outV1, outV2;
//    elasticCollision2D(p1, v1, BALL_MASS, p2, v2, PIECE_MASS, outV1, outV2);
//    b.setVelocity(outV1);
//    p.setVelocity(outV2);
//    // Tách ra khỏi overlap
//    sf::Vector2f d = p1 - p2;
//    float len = std::sqrt(d.x * d.x + d.y * d.y);
//    float overlap = BALL_RADIUS + PIECE_RADIUS - len;
//    if (len > 1e-6f && overlap > 0.f) {
//        d.x /= len;
//        d.y /= len;
//        b.setPosition(sf::Vector2f(p1.x + d.x * overlap, p1.y + d.y * overlap));
//    }
//}

void PhysicsEngine::resolveBallPiece(Ball& b, Piece& p) {
    if (!circleCircle(b.getPosition(), BALL_RADIUS, p.getPosition(), PIECE_RADIUS)) return;

    sf::Vector2f p1 = b.getPosition(), p2 = p.getPosition();
    sf::Vector2f v1 = b.getVelocity(), v2 = p.getVelocity();

    // 1. TÁCH VỊ TRÍ TRƯỚC (Penetration Resolution)
    sf::Vector2f d = p1 - p2;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    float overlap = BALL_RADIUS + PIECE_RADIUS - len;

    if (len > 1e-6f && overlap > 0.f) {
        d.x /= len;
        d.y /= len;
        // Đẩy cả 2 dạt ra dựa theo tỷ lệ khối lượng để bóng không bị trôi xuyên qua
        float totalMass = BALL_MASS + PIECE_MASS;
        float moveBall = overlap * (PIECE_MASS / totalMass);
        float movePiece = overlap * (BALL_MASS / totalMass);
        b.setPosition(sf::Vector2f(p1.x + d.x * moveBall, p1.y + d.y * moveBall));
        p.setPosition(sf::Vector2f(p2.x - d.x * movePiece, p2.y - d.y * movePiece));
    }

    // 2. CHỈ TÍNH VẬN TỐC KHI CHÚNG LAO VÀO NHAU (Chống lỗi dính nhau)
    sf::Vector2f n = p2 - p1;
    // Tích vô hướng của vận tốc tương đối và vector pháp tuyến
    float dotProduct = (v2.x - v1.x) * n.x + (v2.y - v1.y) * n.y;

    if (dotProduct < 0.f) { // Trong toán vector, nếu Tích vô hướng < 0, nghĩa là 2 vật đang di chuyển ngược chiều nhau
        sf::Vector2f outV1, outV2;
        elasticCollision2D(p1, v1, BALL_MASS, p2, v2, PIECE_MASS, outV1, outV2);
        b.setVelocity(outV1 * COLLISION_RESISTANCE_BALL);
        p.setVelocity(outV2 * COLLISION_RESISTANCE_BALL);
    }
}

//void PhysicsEngine::resolvePiecePiece(Piece& a, Piece& b) {
//    if (!circleCircle(a.getPosition(), PIECE_RADIUS, b.getPosition(), PIECE_RADIUS)) return;
//    sf::Vector2f p1 = a.getPosition(), p2 = b.getPosition();
//    sf::Vector2f v1 = a.getVelocity(), v2 = b.getVelocity();
//    sf::Vector2f outV1, outV2;
//    elasticCollision2D(p1, v1, PIECE_MASS, p2, v2, PIECE_MASS, outV1, outV2);
//    a.setVelocity(outV1);
//    b.setVelocity(outV2);
//    sf::Vector2f d = p1 - p2;
//    float len = std::sqrt(d.x * d.x + d.y * d.y);
//    float overlap = 2.f * PIECE_RADIUS - len;
//    if (len > 1e-6f && overlap > 0.f) {
//        d.x /= len;
//        d.y /= len;
//        float half = overlap * 0.5f;
//        a.setPosition(sf::Vector2f(p1.x + d.x * half, p1.y + d.y * half));
//        b.setPosition(sf::Vector2f(p2.x - d.x * half, p2.y - d.y * half));
//    }
//}

void PhysicsEngine::resolvePiecePiece(Piece& a, Piece& b) {
    if (!circleCircle(a.getPosition(), PIECE_RADIUS, b.getPosition(), PIECE_RADIUS)) return;

    sf::Vector2f p1 = a.getPosition(), p2 = b.getPosition();
    sf::Vector2f v1 = a.getVelocity(), v2 = b.getVelocity();

    // 1. TÁCH VỊ TRÍ TRƯỚC
    sf::Vector2f d = p1 - p2;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    float overlap = 2.f * PIECE_RADIUS - len;

    if (len > 1e-6f && overlap > 0.f) {
        d.x /= len;
        d.y /= len;
        float half = overlap * 0.5f;
        a.setPosition(sf::Vector2f(p1.x + d.x * half, p1.y + d.y * half));
        b.setPosition(sf::Vector2f(p2.x - d.x * half, p2.y - d.y * half));
    }

    // 2. CHỈ TÍNH VẬN TỐC KHI LAO VÀO NHAU
    sf::Vector2f n = p2 - p1;
    float dotProduct = (v2.x - v1.x) * n.x + (v2.y - v1.y) * n.y;

    if (dotProduct < 0.f) {
        sf::Vector2f outV1, outV2;
        elasticCollision2D(p1, v1, PIECE_MASS, p2, v2, PIECE_MASS, outV1, outV2);
        a.setVelocity(outV1 * COLLISION_RESISTANCE_PIECE);
        b.setVelocity(outV2 * COLLISION_RESISTANCE_PIECE);
    }
}

void PhysicsEngine::resolveCollisions() {
    resolveWallBall();
    if (pieces_) {
        for (auto& p : *pieces_) resolveWallPiece(*p);
        if (ball_) {
            for (auto& p : *pieces_) resolveBallPiece(*ball_, *p);
        }
        for (size_t i = 0; i < pieces_->size(); ++i)
            for (size_t j = i + 1; j < pieces_->size(); ++j)
                resolvePiecePiece(*(*pieces_)[i], *(*pieces_)[j]);
    }
}

int PhysicsEngine::checkGoal() const {
    if (!ball_) return 0;
    sf::Vector2f p = ball_->getPosition();
    if (field_.isInGoal1(p)) return 1;  // Bàn thắng cho Team2 (bóng vào lưới Team1)
    if (field_.isInGoal2(p)) return 2;  // Bàn thắng cho Team1
    return 0;
}

} // namespace SoccerPool
