#include "AIPlayer.h"
#include "Field.h"
#include <cmath>
#include <limits>
#include <iostream>
#include <algorithm>

namespace SoccerPool {

    AIPlayer::AIPlayer() {}

    float AIPlayer::distance(sf::Vector2f a, sf::Vector2f b) const {
        float dx = b.x - a.x, dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // ==========================================
    // TÍNH GÓC SÚT VÀO KHUNG THÀNH
    // ==========================================
    float AIPlayer::calculateShotAngle(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos) const {
        // Vector từ bóng đến gôn
        sf::Vector2f toGoal = goalPos - ballPos;
        float goalLen = distance(ballPos, goalPos);
        if (goalLen < 0.01f) return 0.f;
        toGoal /= goalLen;

        // Vector từ cầu thủ đến bóng
        sf::Vector2f toBall = ballPos - piecePos;
        float ballLen = distance(piecePos, ballPos);
        if (ballLen < 0.01f) return 0.f;
        toBall /= ballLen;

        // Góc giữa hướng sút và hướng về gôn (càng nhỏ càng tốt)
        float dot = toBall.x * toGoal.x + toBall.y * toGoal.y;
        float angle = std::acos(std::max(-1.f, std::min(1.f, dot))) * 180.f / 3.14159f;

        return angle;
    }

    // ==========================================
    // KIỂM TRA KHẢ NĂNG GHI BÀN TRỰC TIẾP
    // ==========================================
    bool AIPlayer::canScoreDirectly(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos, float& requiredPower) const {
        // Khoảng cách từ cầu thủ đến bóng
        float distToBall = distance(piecePos, ballPos);
        if (distToBall > 120.f) return false; // Quá xa không thể sút chính xác

        // Khoảng cách từ bóng đến gôn
        float distBallToGoal = distance(ballPos, goalPos);

        // Tính góc sút
        float angle = calculateShotAngle(piecePos, ballPos, goalPos);

        // Góc quá lớn không thể ghi bàn (góc chết)
        if (angle > 45.f) return false;

        // Kiểm tra vật cản trên đường bóng đến gôn
        if (!isPathClear(ballPos, goalPos, -1, state_)) return false;

        // Tính lực cần thiết (càng xa càng cần lực mạnh)
        requiredPower = std::min(MAX_SHOOT_POWER, distBallToGoal * 0.5f + 50.f);

        return true;
    }

    // ==========================================
    // ĐÁNH GIÁ CHẤT LƯỢNG CÚ SÚT
    // ==========================================
    float AIPlayer::evaluateShotQuality(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos, sf::Vector2f shotDir, float power) const {
        float quality = 0.f;

        // 1. Góc sút (càng thẳng về gôn càng tốt)
        sf::Vector2f toGoal = goalPos - ballPos;
        float goalLen = distance(ballPos, goalPos);
        if (goalLen > 0.01f) {
            toGoal /= goalLen;
            float dot = shotDir.x * toGoal.x + shotDir.y * toGoal.y;
            quality += dot * 100.f; // Hướng tốt nhất = +100 điểm
        }

        // 2. Lực sút (không quá mạnh cũng không quá yếu)
        float idealPower = std::min(MAX_SHOOT_POWER, distance(piecePos, ballPos) * 1.5f);
        float powerDiff = std::abs(power - idealPower) / MAX_SHOOT_POWER;
        quality += (1.f - powerDiff) * 50.f;

        // 3. Khoảng cách (càng gần càng tốt)
        float distToBall = distance(piecePos, ballPos);
        quality += (150.f - std::min(150.f, distToBall)) * 0.5f;

        return quality;
    }

    // ==========================================
    // TÌM ĐIỂM VA CHẠM TỐI ƯU TRÊN BÓNG
    // ==========================================
    sf::Vector2f AIPlayer::findOptimalHitPoint(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f targetPos) const {
        // Vector từ bóng đến mục tiêu
        sf::Vector2f toTarget = targetPos - ballPos;
        float targetLen = distance(ballPos, targetPos);
        if (targetLen < 0.01f) return ballPos;
        toTarget /= targetLen;

        // Điểm va chạm tối ưu là điểm trên bóng sao cho hướng bóng bay về target
        // Công thức: hitPoint = ballPos + toTarget * BALL_RADIUS
        sf::Vector2f hitPoint = ballPos + toTarget * BALL_RADIUS;

        return hitPoint;
    }

    // ==========================================
    // 1. RAYCASTING: Kiểm tra vật cản
    // ==========================================
    bool AIPlayer::isPathClear(sf::Vector2f start, sf::Vector2f end, int ignorePieceIdx, const GameState* state) const {
        sf::Vector2f dir = end - start;
        float len = distance(start, end);
        if (len < 0.001f) return true;
        dir /= len;

        const auto& pieces = state->getPieces();
        for (size_t i = 0; i < pieces.size(); ++i) {
            if (static_cast<int>(i) == ignorePieceIdx) continue;

            sf::Vector2f pPos = pieces[i]->getPosition();
            sf::Vector2f toPiece = pPos - start;

            float dotProduct = toPiece.x * dir.x + toPiece.y * dir.y;
            if (dotProduct > 0 && dotProduct < len) {
                sf::Vector2f projection = start + dir * dotProduct;
                float distToLine = distance(pPos, projection);

                // Giảm ngưỡng để AI dễ sút hơn
                if (distToLine < PIECE_RADIUS * 1.5f) {
                    return false;
                }
            }
        }
        return true;
    }

    // ==========================================
    // 2. TẠO CÁC CÚ SÚT TIỀM NĂNG (CẢI TIẾN)
    // ==========================================
    std::vector<AIShot> AIPlayer::generateCandidateShots(Team myTeam) {
        std::vector<AIShot> candidates;
        const auto& pieces = state_->getPieces();
        sf::Vector2f ballPos = state_->getBall().getPosition();

        // Khung thành đối phương (mục tiêu để ghi bàn)
        sf::Vector2f enemyGoal = (myTeam == Team::Team1)
            ? sf::Vector2f(FIELD_WIDTH - 20.f, FIELD_HEIGHT / 2.f + 35.f)
            : sf::Vector2f(20.f, FIELD_HEIGHT / 2.f + 35.f);

        // Các điểm góc khung thành để tăng khả năng ghi bàn
        std::vector<sf::Vector2f> goalTargets = {
            enemyGoal,
            sf::Vector2f(enemyGoal.x, enemyGoal.y - 30.f), // Góc trên
            sf::Vector2f(enemyGoal.x, enemyGoal.y + 30.f), // Góc dưới
            sf::Vector2f(enemyGoal.x, enemyGoal.y - 15.f), // Góc trên nhẹ
            sf::Vector2f(enemyGoal.x, enemyGoal.y + 15.f)  // Góc dưới nhẹ
        };

        for (size_t i = 0; i < pieces.size(); ++i) {
            if (pieces[i]->getTeam() != myTeam) continue;
            sf::Vector2f piecePos = pieces[i]->getPosition();

            float distToBall = distance(piecePos, ballPos);

            // Bỏ qua cầu thủ quá xa bóng
            if (distToBall > 180.f) continue;

            // === CHIẾN THUẬT 1: SÚT VÀO KHUNG THÀNH (ƯU TIÊN CAO NHẤT) ===
            for (const auto& goalTarget : goalTargets) {
                // Kiểm tra xem có thể ghi bàn trực tiếp không
                float requiredPower;
                if (canScoreDirectly(piecePos, ballPos, goalTarget, requiredPower)) {
                    // Tìm điểm va chạm tối ưu trên bóng
                    sf::Vector2f hitPoint = findOptimalHitPoint(piecePos, ballPos, goalTarget);

                    // Kiểm tra đường đi đến điểm va chạm
                    if (isPathClear(piecePos, hitPoint, i, state_)) {
                        sf::Vector2f toHit = hitPoint - piecePos;
                        float len = distance(piecePos, hitPoint);
                        if (len > 0.01f) {
                            toHit /= len;
                            // Thêm cú sút với lực phù hợp
                            candidates.push_back({ (int)i, toHit * requiredPower, true });

                            // Thêm các biến thể lực để tối ưu
                            candidates.push_back({ (int)i, toHit * (requiredPower * 0.8f), true });
                            candidates.push_back({ (int)i, toHit * (requiredPower * 1.2f), true });
                        }
                    }
                }
            }

            // === CHIẾN THUẬT 2: SÚT TRỰC TIẾP VÀO BÓNG ===
            sf::Vector2f toBall = ballPos - piecePos;
            float len = distance(piecePos, ballPos);
            if (len > 0.01f && isPathClear(piecePos, ballPos, i, state_)) {
                toBall /= len;

                // Đánh giá chất lượng cú sút
                float quality = evaluateShotQuality(piecePos, ballPos, enemyGoal, toBall, MAX_SHOOT_POWER);

                // Chỉ thêm nếu chất lượng đủ tốt
                if (quality > 30.f) {
                    candidates.push_back({ (int)i, toBall * MAX_SHOOT_POWER, true });
                    candidates.push_back({ (int)i, toBall * (MAX_SHOOT_POWER * 0.7f), true });
                    candidates.push_back({ (int)i, toBall * (MAX_SHOOT_POWER * 0.4f), true });
                }
            }

            // === CHIẾN THUẬT 3: SÚT ĐẬP BĂNG (KHI KHÔNG THỂ SÚT THẲNG) ===
            if (distToBall < 150.f) {
                addBankShots(candidates, i, piecePos, ballPos, MAX_SHOOT_POWER);
            }

            // === CHIẾN THUẬT 4: SÚT VỀ PHÍA KHUNG THÀNH (TẠO ÁP LỰC) ===
            if (distToBall < 120.f) {
                sf::Vector2f toGoal = enemyGoal - piecePos;
                float goalLen = distance(piecePos, enemyGoal);
                if (goalLen > 0.01f && isPathClear(piecePos, enemyGoal, i, state_)) {
                    toGoal /= goalLen;
                    candidates.push_back({ (int)i, toGoal * (MAX_SHOOT_POWER * 0.6f), true });
                }
            }
        }

        return candidates;
    }

    void AIPlayer::addDirectShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power) {
        sf::Vector2f dir = targetPos - piecePos;
        float len = distance(piecePos, targetPos);
        if (len > 0 && isPathClear(piecePos, targetPos, pieceIdx, state_)) {
            candidates.push_back({ pieceIdx, (dir / len) * power, true });
        }
    }

    void AIPlayer::addBankShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power) {
        // Đập băng Trên
        sf::Vector2f virtualTargetTop(targetPos.x, FIELD_MARGIN_Y - (targetPos.y - FIELD_MARGIN_Y));
        addDirectShots(candidates, pieceIdx, piecePos, virtualTargetTop, power);

        // Đập băng Dưới
        float bottomWall = FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM;
        sf::Vector2f virtualTargetBottom(targetPos.x, bottomWall + (bottomWall - targetPos.y));
        addDirectShots(candidates, pieceIdx, piecePos, virtualTargetBottom, power);

        // Đập băng Trái (nếu cần)
        if (piecePos.x > FIELD_WIDTH / 2.f) {
            sf::Vector2f virtualTargetLeft(FIELD_MARGIN_X - (targetPos.x - FIELD_MARGIN_X), targetPos.y);
            addDirectShots(candidates, pieceIdx, piecePos, virtualTargetLeft, power * 0.8f);
        }

        // Đập băng Phải (nếu cần)
        if (piecePos.x < FIELD_WIDTH / 2.f) {
            sf::Vector2f virtualTargetRight(FIELD_WIDTH - FIELD_MARGIN_X + (FIELD_WIDTH - FIELD_MARGIN_X - targetPos.x), targetPos.y);
            addDirectShots(candidates, pieceIdx, piecePos, virtualTargetRight, power * 0.8f);
        }
    }

    // ==========================================
    // 3. HEURISTIC: Hàm đánh giá điểm số chiến thuật (CẢI TIẾN)
    // ==========================================
    float AIPlayer::evaluateState(GameState* simState, Team myTeam) {
        float score = 0.f;
        Team enemyTeam = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        sf::Vector2f ballPos = simState->getBall().getPosition();

        // Vị trí gôn
        sf::Vector2f myGoal = (myTeam == Team::Team1) ? sf::Vector2f(0.f, FIELD_HEIGHT / 2.f + 35.f) : sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT / 2.f + 35.f);
        sf::Vector2f enemyGoal = (myTeam == Team::Team1) ? sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT / 2.f + 35.f) : sf::Vector2f(0.f, FIELD_HEIGHT / 2.f + 35.f);

        // 1. TRẠNG THÁI BÀN THẮNG (ƯU TIÊN CAO NHẤT)
        int myScore = (myTeam == Team::Team1) ? simState->getScore1() : simState->getScore2();
        int oldMyScore = (myTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
        if (myScore > oldMyScore) return 1000000.f;

        int enemyScore = (myTeam == Team::Team1) ? simState->getScore2() : simState->getScore1();
        int oldEnemyScore = (myTeam == Team::Team1) ? state_->getScore2() : state_->getScore1();
        if (enemyScore > oldEnemyScore) return -1000000.f;

        // 2. TẤN CÔNG: Bóng càng gần gôn địch và có góc tốt
        float distBallToEnemyGoal = distance(ballPos, enemyGoal);
        float angleToGoal = 0.f;

        // Tính góc bóng so với gôn
        sf::Vector2f goalDir = enemyGoal - ballPos;
        float goalLen = distance(ballPos, enemyGoal);
        if (goalLen > 0.01f) {
            goalDir /= goalLen;
            angleToGoal = std::abs(goalDir.x) * 90.f; // Góc càng thẳng càng tốt
        }

        score += (FIELD_WIDTH - distBallToEnemyGoal) * 15.f; // Càng gần càng tốt
        score += (90.f - angleToGoal) * 2.f; // Góc càng thẳng càng tốt

        // 3. PHÒNG NGỰ: Bóng càng xa gôn mình càng tốt
        float distBallToMyGoal = distance(ballPos, myGoal);
        score += distBallToMyGoal * 8.f;

        // 4. KIỂM TRA CƠ HỘI GHI BÀN (THÊM ĐIỂM THƯỞNG)
        for (const auto& p : simState->getPieces()) {
            if (p->getTeam() == myTeam) {
                float distToBall = distance(p->getPosition(), ballPos);
                if (distToBall < 80.f) {
                    // Cầu thủ đang ở gần bóng
                    score += 50.f;

                    // Kiểm tra góc sút về gôn
                    float angle = calculateShotAngle(p->getPosition(), ballPos, enemyGoal);
                    if (angle < 30.f) {
                        score += 100.f; // Cơ hội ghi bàn tốt
                    }
                }
            }
            else {
                // TRỪ ĐIỂM nếu quân địch ở gần bóng
                float dEnemyToBall = distance(p->getPosition(), ballPos);
                if (dEnemyToBall < 80.f) {
                    score -= 80.f;
                }
                // Đặc biệt trừ nhiều nếu địch ở giữa bóng và gôn nhà
                if (isBetween(p->getPosition(), ballPos, myGoal)) {
                    score -= 150.f;
                }
            }
        }

        // 5. VẬN TỐC BÓNG (bóng đang lăn về gôn địch thì tốt)
        sf::Vector2f ballVel = simState->getBall().getVelocity();
        float velTowardGoal = ballVel.x * goalDir.x + ballVel.y * goalDir.y;
        if (velTowardGoal > 0) {
            score += velTowardGoal * 2.f;
        }

        return score;
    }

    // Hàm kiểm tra điểm có nằm giữa 2 điểm không
    //bool isBetween(sf::Vector2f point, sf::Vector2f a, sf::Vector2f b) const {
    //    float distAB = this->distance(a, b);
    //    float distAP = this->distance(a, point);  
    //    float distPB = this->distance(point, b);
    //    return std::abs(distAB - (distAP + distPB)) < 10.f;
    //}

    // ==========================================
    // 4. SIMULATION: Chạy thử và ra quyết định
    // ==========================================
    float AIPlayer::simulateAndEvaluate(const AIShot& shot, Team myTeam) {
        GameState* simState = state_->clone();

        // Áp dụng lực bắn
        simState->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);

        // Chạy mô phỏng
        float fixedDt = 1.f / 60.f;
        int maxSteps = 200;

        for (int step = 0; step < maxSteps; ++step) {
            simState->update(fixedDt);
            if (simState->isEverythingStopped()) break;
            if (simState->getPhase() == GamePhase::GoalScored) break;
        }

        float score = evaluateState(simState, myTeam);
        delete simState;
        return score;
    }

    // ==========================================
    // HÀM CHÍNH LÀM NHIỆM VỤ TÌM NƯỚC ĐI
    // ==========================================
    AIShot AIPlayer::computeShot() {
        if (!state_) return AIShot();
        Team myTeam = state_->getCurrentTurn();

        // 1. Quét tìm tất cả các phương án sút
        std::vector<AIShot> candidates = generateCandidateShots(myTeam);

        if (candidates.empty()) {
            // Sút bừa từ cầu thủ gần bóng nhất
            const auto& pieces = state_->getPieces();
            sf::Vector2f ballPos = state_->getBall().getPosition();
            int nearestIdx = -1;
            float minDist = std::numeric_limits<float>::max();

            for (size_t i = 0; i < pieces.size(); ++i) {
                if (pieces[i]->getTeam() != myTeam) continue;
                float dist = distance(pieces[i]->getPosition(), ballPos);
                if (dist < minDist) {
                    minDist = dist;
                    nearestIdx = static_cast<int>(i);
                }
            }

            if (nearestIdx >= 0) {
                AIShot desperateShot;
                desperateShot.pieceIndex = nearestIdx;
                sf::Vector2f toBall = ballPos - pieces[nearestIdx]->getPosition();
                float len = distance(pieces[nearestIdx]->getPosition(), ballPos);
                if (len > 0.01f) {
                    toBall /= len;
                    desperateShot.velocity = toBall * MAX_SHOOT_POWER;
                }
                else {
                    desperateShot.velocity = sf::Vector2f(MAX_SHOOT_POWER, 0.f);
                }
                desperateShot.valid = true;
                return desperateShot;
            }
            return AIShot();
        }

        // 2. Chọn cú sút tốt nhất
        AIShot bestShot;
        float bestScore = -std::numeric_limits<float>::infinity();

        for (const AIShot& shot : candidates) {
            float score = simulateAndEvaluate(shot, myTeam);
            if (score > bestScore) {
                bestScore = score;
                bestShot = shot;
            }
        }

        return bestShot;
    }

    // Hàm isBetween cần được khai báo trong class
    bool AIPlayer::isBetween(sf::Vector2f point, sf::Vector2f a, sf::Vector2f b) const {
        float distAB = distance(a, b);
        float distAP = distance(a, point);
        float distPB = distance(point, b);
        return std::abs(distAB - (distAP + distPB)) < 10.f;
    }



} // namespace SoccerPool