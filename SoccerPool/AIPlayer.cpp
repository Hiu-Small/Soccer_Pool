#include "AIPlayer.h"
#include "Field.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace SoccerPool {

    AIPlayer::AIPlayer() {}

    float AIPlayer::distance(sf::Vector2f a, sf::Vector2f b) const {
        float dx = b.x - a.x, dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
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

            // Tính khoảng cách từ tâm quân cờ tới đường đạn (Toán học chiếu vector)
            float dotProduct = toPiece.x * dir.x + toPiece.y * dir.y;
            if (dotProduct > 0 && dotProduct < len) {
                sf::Vector2f projection = start + dir * dotProduct;
                float distToLine = distance(pPos, projection);

                // Nếu khoảng cách nhỏ hơn 2 lần bán kính -> Bị vướng
                if (distToLine < PIECE_RADIUS * 2.f) {
                    return false;
                }
            }
        }
        return true;
    }

    // ==========================================
    // 2. TẠO CÁC CÚ SÚT TIỀM NĂNG (Bao gồm đập băng)
    // ==========================================
    std::vector<AIShot> AIPlayer::generateCandidateShots(Team myTeam) {
        std::vector<AIShot> candidates;
        const auto& pieces = state_->getPieces();
        sf::Vector2f ballPos = state_->getBall().getPosition();

        for (size_t i = 0; i < pieces.size(); ++i) {
            if (pieces[i]->getTeam() != myTeam) continue;
            sf::Vector2f pPos = pieces[i]->getPosition();

            // A. Thử sút thẳng vào bóng
            if (isPathClear(pPos, ballPos, i, state_)) {
                addDirectShots(candidates, i, pPos, ballPos, MAX_SHOOT_POWER);
                addDirectShots(candidates, i, pPos, ballPos, MAX_SHOOT_POWER * 0.6f); // Thử lực nhẹ
            }

            // B. Thử sút đập băng (Bank shots) - Bí quyết của AI cao thủ
            addBankShots(candidates, i, pPos, ballPos, MAX_SHOOT_POWER);
        }
        return candidates;
    }

    void AIPlayer::addDirectShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power) {
        sf::Vector2f dir = targetPos - piecePos;
        float len = distance(piecePos, targetPos);
        if (len > 0) {
            candidates.push_back({ pieceIdx, (dir / len) * power, true });
        }
    }

    void AIPlayer::addBankShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power) {
        // Kỹ thuật lấy đối xứng gương: Để đập băng trên, ta giả vờ mục tiêu nằm đối xứng qua băng trên.

        // Đập băng Trên
        sf::Vector2f virtualTargetTop(targetPos.x, FIELD_MARGIN_Y - (targetPos.y - FIELD_MARGIN_Y));
        addDirectShots(candidates, pieceIdx, piecePos, virtualTargetTop, power);

        // Đập băng Dưới
        float bottomWall = FIELD_HEIGHT - FIELD_MARGIN_Y;
        sf::Vector2f virtualTargetBottom(targetPos.x, bottomWall + (bottomWall - targetPos.y));
        addDirectShots(candidates, pieceIdx, piecePos, virtualTargetBottom, power);

        // Bạn có thể làm tương tự cho băng trái/phải
    }

    // ==========================================
    // 3. HEURISTIC: Hàm đánh giá điểm số chiến thuật
    // ==========================================
    float AIPlayer::evaluateState(GameState* simState, Team myTeam) {
        float score = 0.f;
        Team enemyTeam = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;

        // 1. Kiểm tra Bàn Thắng (Ưu tiên Tuyệt đối)
        int myGoals = (myTeam == Team::Team1) ? simState->getScore1() : simState->getScore2();
        int oldGoals = (myTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
        if (myGoals > oldGoals) score += 10000.f; // Ghi bàn

        int enemyGoals = (enemyTeam == Team::Team1) ? simState->getScore1() : simState->getScore2();
        int oldEnemyGoals = (enemyTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
        if (enemyGoals > oldEnemyGoals) score -= 10000.f; // Phản lưới nhà

        // 2. Vị trí bóng so với khung thành đối phương
        sf::Vector2f enemyGoalCenter = (myTeam == Team::Team1)
            ? sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT / 2.f)
            : sf::Vector2f(0.f, FIELD_HEIGHT / 2.f);

        sf::Vector2f ballPos = simState->getBall().getPosition();
        float distToEnemyGoal = distance(ballPos, enemyGoalCenter);
        score -= distToEnemyGoal * 2.0f; // Bóng càng gần gôn địch càng tốt

        // 3. Cấu trúc đội hình (Gọng kìm / Phòng thủ)
        // Tặng điểm nếu quân cờ của mình nằm chắn giữa bóng và gôn nhà
        sf::Vector2f myGoalCenter = (myTeam == Team::Team1)
            ? sf::Vector2f(0.f, FIELD_HEIGHT / 2.f)
            : sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT / 2.f);

        for (const auto& p : simState->getPieces()) {
            if (p->getTeam() == myTeam) {
                float distToMyGoal = distance(p->getPosition(), myGoalCenter);
                if (distToMyGoal < 150.f) score += 50.f; // Hậu vệ đứng bảo vệ gôn
            }
        }

        return score;
    }

    // ==========================================
    // 4. SIMULATION: Chạy thử và ra quyết định
    // ==========================================
    float AIPlayer::simulateAndEvaluate(const AIShot& shot, Team myTeam) {
        // BẮT BUỘC: Bạn phải có hàm clone() để copy toàn bộ dữ liệu trận đấu hiện tại ra một bản nháp
        GameState* simState = state_->clone();

        // Áp dụng lực bắn vào bản nháp
        simState->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);

        // Chạy Engine Vật lý ngầm (tua nhanh thời gian) cho đến khi mọi thứ dừng lại
        // Tránh vòng lặp vô hạn bằng cách giới hạn tối đa 500 step
        int maxSteps = 500;
        while (!simState->isEverythingStopped() && maxSteps > 0) {
            simState->update(1.f / 60.f); // Hàm update của bạn
            maxSteps--;
        }

        // Chấm điểm kết quả
        float finalScore = evaluateState(simState, myTeam);

        delete simState; // Xóa bản nháp để chống tràn RAM
        return finalScore;
    }

    // ==========================================
    // HÀM CHÍNH LÀM NHIỆM VỤ TÌM NƯỚC ĐI
    // ==========================================
    AIShot AIPlayer::computeShot() {
        if (!state_) return AIShot();
        Team myTeam = state_->getCurrentTurn();

        // 1. Quét tìm tất cả các phương án sút có thể
        std::vector<AIShot> candidates = generateCandidateShots(myTeam);

        if (candidates.empty()) {
            // Nếu không có đường nào, sút bừa giải vây
            AIShot desperateShot;
            desperateShot.pieceIndex = 0; // Tạm lấy quân đầu tiên
            desperateShot.velocity = sf::Vector2f(MAX_SHOOT_POWER, 0.f);
            desperateShot.valid = true;
            return desperateShot;
        }

        // 2. Giả lập từng phương án và tìm điểm cao nhất
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

} // namespace SoccerPool