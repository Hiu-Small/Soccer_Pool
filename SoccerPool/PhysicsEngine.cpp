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

        // Chuẩn hóa vector pháp tuyến (Normal vector)
        n.x /= len;
        n.y /= len;

        // Tính vận tốc tương đối
        sf::Vector2f rv = v2 - v1;
        float velAlongNormal = rv.x * n.x + rv.y * n.y;

        // Nếu 2 vật đang tách xa nhau thì không va chạm
        if (velAlongNormal > 0) {
            outV1 = v1;
            outV2 = v2;
            return;
        }

        // Hệ số bồi hoàn (Restitution) - Giống Soccer Stars
        // Hệ số bồi hoàn (Restitution) - Lấy từ Constants.h
        float e = COLLISION_RESISTANCE_BALL;

        // Tính toán lực xung (Impulse scalar)
        float j = -(1 + e) * velAlongNormal;
        j /= (1 / m1) + (1 / m2);

        // Áp dụng lực xung lên các vector vận tốc
        sf::Vector2f impulse = sf::Vector2f(j * n.x, j * n.y);

        outV1 = sf::Vector2f(v1.x - (1 / m1) * impulse.x, v1.y - (1 / m1) * impulse.y);
        outV2 = sf::Vector2f(v2.x + (1 / m2) * impulse.x, v2.y + (1 / m2) * impulse.y);
    }

    void PhysicsEngine::resolveWallBall() {
        if (!ball_) return;
        auto& b = *ball_;
        sf::Vector2f p = b.getPosition();
        sf::Vector2f v = b.getVelocity();

        // ---> FIX: THU NHỎ HITBOX KHI BÓNG ĐẬP TƯỜNG <---
        // Dùng chung hệ số bạn đã chốt ở resolveBallPiece (VD: 0.85f)
        const float BALL_HITBOX_SCALE = 0.85f;
        float r = BALL_RADIUS * BALL_HITBOX_SCALE;

        float minX = FIELD_MARGIN_X + r;
        float maxX = FIELD_WIDTH - FIELD_MARGIN_X - r;
        float minY = FIELD_MARGIN_Y + r;
        float maxY = FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM - r;

        bool inGoalY = (p.y >= GOAL_Y_OFFSET && p.y <= GOAL_Y_OFFSET + GOAL_HEIGHT);
        bool hit = false;
        float ballWallRestitution = 0.9f;

        // 1. KIỂM TRA VÀ PHẢN XẠ TRỤC X
        if (p.x < minX) {
            if (!inGoalY) {
                p.x = minX;
                v.x = std::abs(v.x) * ballWallRestitution;
                hit = true;
            }
            else {
                float netBackX = FIELD_MARGIN_X - 45.f;
                if (p.x < netBackX + r) {
                    p.x = netBackX + r;
                    v.x = std::abs(v.x) * ballWallRestitution;
                    hit = true;
                }
            }
        }
        else if (p.x > maxX) {
            if (!inGoalY) {
                p.x = maxX;
                v.x = -std::abs(v.x) * ballWallRestitution;
                hit = true;
            }
            else {
                float netBackX = FIELD_WIDTH - FIELD_MARGIN_X + 45.f;
                if (p.x > netBackX - r) {
                    p.x = netBackX - r;
                    v.x = -std::abs(v.x) * ballWallRestitution;
                    hit = true;
                }
            }
        }

        // 2. KIỂM TRA VÀ PHẢN XẠ TRỤC Y SÂN CỎ
        if (p.y < minY) {
            p.y = minY;
            v.y = std::abs(v.y) * ballWallRestitution;
            hit = true;
        }
        else if (p.y > maxY) {
            p.y = maxY;
            v.y = -std::abs(v.y) * ballWallRestitution;
            hit = true;
        }

        // 3. TƯỜNG NGANG BÊN TRONG GÔN (HÔNG LƯỚI)
        float goalTopY = GOAL_Y_OFFSET + r;
        float goalBotY = GOAL_Y_OFFSET + GOAL_HEIGHT - r;

        // ĐÃ FIX: Chỉ áp dụng nếu bóng đã qua vạch vôi (p.x <= FIELD_MARGIN_X)
        if (p.x <= FIELD_MARGIN_X) {
            if (p.y < goalTopY) {
                p.y = goalTopY;
                v.y = std::abs(v.y) * ballWallRestitution;
                hit = true;
            }
            else if (p.y > goalBotY) {
                p.y = goalBotY;
                v.y = -std::abs(v.y) * ballWallRestitution;
                hit = true;
            }
        }
        else if (p.x >= FIELD_WIDTH - FIELD_MARGIN_X) {
            if (p.y < goalTopY) {
                p.y = goalTopY;
                v.y = std::abs(v.y) * ballWallRestitution;
                hit = true;
            }
            else if (p.y > goalBotY) {
                p.y = goalBotY;
                v.y = -std::abs(v.y) * ballWallRestitution;
                hit = true;
            }
        }

        if (hit) {
            float speed = std::sqrt(v.x * v.x + v.y * v.y);
            if (speed > 25.f) playCollideSoundFlag = true;
            b.setPosition(p);
            b.setVelocity(v);
        }
    }

    void PhysicsEngine::resolveWallPiece(Piece& p_obj) {
        sf::Vector2f p = p_obj.getPosition();
        sf::Vector2f v = p_obj.getVelocity();

        // ---> FIX: THU NHỎ HITBOX KHI CẦU THỦ ĐẬP TƯỜNG <---
        // Dùng chung hệ số 0.80f để cầu thủ lún sát được vào vạch kẻ biên
        const float PIECE_HITBOX_SCALE = 0.80f;
        float r = PIECE_RADIUS * PIECE_HITBOX_SCALE;

        bool hit = false;
        float pieceWallRestitution = 0.6f;

        float minX = FIELD_MARGIN_X + r;
        float maxX = FIELD_WIDTH - FIELD_MARGIN_X - r;
        float minY = FIELD_MARGIN_Y + r;
        float maxY = FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM - r;

        bool inGoalY = (p.y >= GOAL_Y_OFFSET && p.y <= GOAL_Y_OFFSET + GOAL_HEIGHT);

        // 1. KIỂM TRA VÀ PHẢN XẠ TRỤC X
        if (p.x < minX) {
            if (!inGoalY) {
                p.x = minX;
                v.x = std::abs(v.x) * pieceWallRestitution;
                hit = true;
            }
            else {
                float netBackX = FIELD_MARGIN_X - 50.f;
                if (p.x < netBackX + r) {
                    p.x = netBackX + r;
                    v.x = std::abs(v.x) * pieceWallRestitution;
                    hit = true;
                }
            }
        }
        else if (p.x > maxX) {
            if (!inGoalY) {
                p.x = maxX;
                v.x = -std::abs(v.x) * pieceWallRestitution;
                hit = true;
            }
            else {
                float netBackX = FIELD_WIDTH - FIELD_MARGIN_X + 50.f;
                if (p.x > netBackX - r) {
                    p.x = netBackX - r;
                    v.x = -std::abs(v.x) * pieceWallRestitution;
                    hit = true;
                }
            }
        }

        // 2. KIỂM TRA VÀ PHẢN XẠ TRỤC Y SÂN CỎ
        if (p.y < minY) {
            p.y = minY;
            v.y = std::abs(v.y) * pieceWallRestitution;
            hit = true;
        }
        else if (p.y > maxY) {
            p.y = maxY;
            v.y = -std::abs(v.y) * pieceWallRestitution;
            hit = true;
        }

        // 3. TƯỜNG NGANG BÊN TRONG GÔN (HÔNG LƯỚI)
        float goalTopY = GOAL_Y_OFFSET + r;
        float goalBotY = GOAL_Y_OFFSET + GOAL_HEIGHT - r;

        // ĐÃ FIX: Chỉ áp dụng nếu cầu thủ đã qua vạch vôi (p.x <= FIELD_MARGIN_X)
        // Ngăn chặn việc cầu thủ đứng ở vạch vôi bị "giật" và hút vào hông lưới
        if (p.x <= FIELD_MARGIN_X) {
            if (p.y < goalTopY) {
                p.y = goalTopY;
                v.y = std::abs(v.y) * pieceWallRestitution;
                hit = true;
            }
            else if (p.y > goalBotY) {
                p.y = goalBotY;
                v.y = -std::abs(v.y) * pieceWallRestitution;
                hit = true;
            }
        }
        else if (p.x >= FIELD_WIDTH - FIELD_MARGIN_X) {
            if (p.y < goalTopY) {
                p.y = goalTopY;
                v.y = std::abs(v.y) * pieceWallRestitution;
                hit = true;
            }
            else if (p.y > goalBotY) {
                p.y = goalBotY;
                v.y = -std::abs(v.y) * pieceWallRestitution;
                hit = true;
            }
        }

        if (hit) {
            p_obj.setPosition(p);
            p_obj.setVelocity(v);
        }
    }

    void PhysicsEngine::resolveBallPiece(Ball& b, Piece& p) {
        // ---> FIX: THU NHỎ HITBOX CHO CẦU THỦ <---
        const float PIECE_HITBOX_SCALE = 0.80f;
        const float PIECE_COLLISION_RADIUS = PIECE_RADIUS * PIECE_HITBOX_SCALE;

        // Bạn có thể chỉnh sửa số này (ví dụ: 0.85f, 0.9f) để xem mức độ lún nào nhìn đẹp nhất
        const float BALL_HITBOX_SCALE = 0.80f;
        const float BALL_COLLISION_RADIUS = BALL_RADIUS * BALL_HITBOX_SCALE;

        if (!circleCircle(b.getPosition(), BALL_COLLISION_RADIUS, p.getPosition(), PIECE_COLLISION_RADIUS)) return;

        sf::Vector2f p1 = b.getPosition(), p2 = p.getPosition();
        sf::Vector2f v1 = b.getVelocity(), v2 = p.getVelocity();

        sf::Vector2f d = p1 - p2;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        float overlap = BALL_COLLISION_RADIUS + PIECE_COLLISION_RADIUS - len;

        if (len > 1e-6f && overlap > 0.f) {
            d.x /= len;
            d.y /= len;
            float invMass1 = 1.0f / BALL_MASS;
            float invMass2 = 1.0f / PIECE_MASS;
            float totalInvMass = invMass1 + invMass2;

            const float percent = 0.8f;
            sf::Vector2f correction = sf::Vector2f(d.x * (overlap / totalInvMass) * percent,
                d.y * (overlap / totalInvMass) * percent);

            b.setPosition(sf::Vector2f(p1.x + correction.x * invMass1, p1.y + correction.y * invMass1));
            p.setPosition(sf::Vector2f(p2.x - correction.x * invMass2, p2.y - correction.y * invMass2));
        }

        sf::Vector2f outV1, outV2;
        elasticCollision2D(b.getPosition(), v1, BALL_MASS, p.getPosition(), v2, PIECE_MASS, outV1, outV2);
        
        b.setVelocity(outV1);
        p.setVelocity(outV2);

        float ballSpeed = std::sqrt(v1.x * v1.x + v1.y * v1.y);
        if (ballSpeed < 1.0f) playHitSoundFlag = true;
        else playCollideSoundFlag = true;
    }

    void PhysicsEngine::resolvePiecePiece(Piece& a, Piece& b) {
        // TẠO HITBOX THU NHỎ: 0.85f nghĩa là hitbox chỉ bằng 85% bán kính ảnh gốc.
        // Bạn có thể tự chỉnh con số này (vd: 0.8f, 0.9f) đến khi thấy 2 cầu thủ cụng nhau vừa khít.
        const float HITBOX_SCALE = 0.80f;
        const float COLLISION_RADIUS = PIECE_RADIUS * HITBOX_SCALE;

        if (!circleCircle(a.getPosition(), COLLISION_RADIUS, b.getPosition(), COLLISION_RADIUS)) return;

        sf::Vector2f p1 = a.getPosition(), p2 = b.getPosition();
        sf::Vector2f v1 = a.getVelocity(), v2 = b.getVelocity();

        sf::Vector2f d = p1 - p2;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        float overlap = 2.f * COLLISION_RADIUS - len;

        if (len > 1e-6f && overlap > 0.f) {
            d.x /= len;
            d.y /= len;
            float half = overlap * 0.5f;
            a.setPosition(sf::Vector2f(p1.x + d.x * half, p1.y + d.y * half));
            b.setPosition(sf::Vector2f(p2.x - d.x * half, p2.y - d.y * half));
        }

        sf::Vector2f outV1, outV2;
        elasticCollision2D(a.getPosition(), v1, PIECE_MASS, b.getPosition(), v2, PIECE_MASS, outV1, outV2);

        a.setVelocity(outV1);
        b.setVelocity(outV2);

        float vt1 = std::sqrt(v1.x * v1.x + v1.y * v1.y);
        float vt2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
        if (vt1 > 30.f || vt2 > 30.f) playCollideSoundFlag = true;
    }

    // HÀM CHUNG ĐỂ XỬ LÝ VA CHẠM VỚI CỘT DỌC HÌNH TRÒN
    void PhysicsEngine::resolveCircleCollision(sf::Vector2f& pos, sf::Vector2f& vel, float radius, sf::Vector2f postPos, float restitution) {
        float postRadius = 1.0f; // Bán kính cột dọc (rất nhỏ để làm mượt góc vuông)

        if (circleCircle(pos, radius, postPos, postRadius)) {
            sf::Vector2f d = pos - postPos;
            float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len > 1e-6f) {
                d.x /= len;
                d.y /= len;

                float overlap = radius + postRadius - len;
                pos.x += d.x * overlap;
                pos.y += d.y * overlap;

                float dotProduct = vel.x * d.x + vel.y * d.y;
                if (dotProduct < 0.f) {
                    vel.x = (vel.x - 2.f * dotProduct * d.x) * restitution;
                    vel.y = (vel.y - 2.f * dotProduct * d.y) * restitution;
                }
            }
        }
    }

    void PhysicsEngine::resolveCollisions() {
    if (pieces_) {
        for (size_t i = 0; i < pieces_->size(); ++i)
            for (size_t j = i + 1; j < pieces_->size(); ++j)
                resolvePiecePiece(*(*pieces_)[i], *(*pieces_)[j]);

        if (ball_) {
            for (auto& p : *pieces_) resolveBallPiece(*ball_, *p);
        }
    }

    // ƯU TIÊN 1: XỬ LÝ 4 CỘT DỌC GÔN (Tránh kẹt góc vuông)
    sf::Vector2f postL_Top(FIELD_MARGIN_X, GOAL_Y_OFFSET);
    sf::Vector2f postL_Bot(FIELD_MARGIN_X, GOAL_Y_OFFSET + GOAL_HEIGHT);
    sf::Vector2f postR_Top(FIELD_WIDTH - FIELD_MARGIN_X, GOAL_Y_OFFSET);
    sf::Vector2f postR_Bot(FIELD_WIDTH - FIELD_MARGIN_X, GOAL_Y_OFFSET + GOAL_HEIGHT);

    // ---> FIX HITBOX CHO CỘT DỌC <---
    const float BALL_HITBOX_SCALE = 0.80f; // Dùng chung hệ số ở các hàm trước
    const float PIECE_HITBOX_SCALE = 0.80f; 

    if (ball_) {
        sf::Vector2f bPos = ball_->getPosition();
        sf::Vector2f bVel = ball_->getVelocity();
        float bRestitution = 0.85f;
        
        // Truyền bán kính đã thu nhỏ vào
        float ballCollisionRadius = BALL_RADIUS * BALL_HITBOX_SCALE;
        
        resolveCircleCollision(bPos, bVel, ballCollisionRadius, postL_Top, bRestitution);
        resolveCircleCollision(bPos, bVel, ballCollisionRadius, postL_Bot, bRestitution);
        resolveCircleCollision(bPos, bVel, ballCollisionRadius, postR_Top, bRestitution);
        resolveCircleCollision(bPos, bVel, ballCollisionRadius, postR_Bot, bRestitution);
       
        ball_->setPosition(bPos);
        ball_->setVelocity(bVel);
    }

    if (pieces_) {
        float pRestitution = 0.6f;
        float pieceCollisionRadius = PIECE_RADIUS * PIECE_HITBOX_SCALE;
        
        for (auto& p : *pieces_) {
            sf::Vector2f pPos = p->getPosition();
            sf::Vector2f pVel = p->getVelocity();
            
            // Truyền bán kính đã thu nhỏ vào
            resolveCircleCollision(pPos, pVel, pieceCollisionRadius, postL_Top, pRestitution);
            resolveCircleCollision(pPos, pVel, pieceCollisionRadius, postL_Bot, pRestitution);
            resolveCircleCollision(pPos, pVel, pieceCollisionRadius, postR_Top, pRestitution);
            resolveCircleCollision(pPos, pVel, pieceCollisionRadius, postR_Bot, pRestitution);
            
            p->setPosition(pPos);
            p->setVelocity(pVel);
        }
    }

    // ƯU TIÊN 2: XỬ LÝ CÁC VÁCH TƯỜNG (AABB)
    resolveWallBall();
    if (pieces_) {
        for (auto& p : *pieces_) resolveWallPiece(*p);
    }
}

    int PhysicsEngine::checkGoal() const {
        if (!ball_) return 0;
        sf::Vector2f p = ball_->getPosition();
        if (field_.isInGoal1(p)) return 1;
        if (field_.isInGoal2(p)) return 2;
        return 0;
    }

} // namespace SoccerPool