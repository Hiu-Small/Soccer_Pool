#include "AIPlayer.h"
#include "Field.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <queue>
#include <functional>

namespace SoccerPool {

    // ============================================================
    // HẰNG SỐ CHUYÊN SÂU CHO AI
    // ============================================================
    static constexpr float AI_MAX_REACH_DIST = 2000.f;   // Được nới rộng để không bỏ sót cầu thủ nào, tránh đơ game
    static const     float AI_GOAL_HALF_H = GOAL_HEIGHT / 2.f;
    static constexpr float AI_MIN_SHOT_LEN = 3.f;
    static constexpr int   AI_SIM_STEPS = 200;
    static constexpr int   AI_MINIMAX_DEPTH = 2;
    static constexpr int   AI_TOP_CANDIDATES = 28;
    static constexpr int   AI_ENEMY_RESPONSES = 10;  // Tăng để dự đoán địch tốt hơn
    static constexpr float CORNER_ZONE_R = 200.f;   // Bán kính vùng góc sân bị phạt
    static constexpr float CORNER_PENALTY_W = 8.f;     // Hệ số phạt góc sân
    static constexpr float OWN_GOAL_PENALTY = -1e9f;   // Phạt phản lưới nhà
    static constexpr float SCORE_GOAL_REWARD = 1e8f;    // Thưởng ghi bàn
    static constexpr float PI = 3.14159265f;

    // Các tỉ lệ lực sút
    static const float POWER_VARIANTS[] = { 0.45f, 0.70f, 0.90f, 1.00f };

    // ============================================================
    // CONSTRUCTOR
    // ============================================================
    AIPlayer::AIPlayer() {}

    // ============================================================
    // TOÁN HỌC & HÌNH HỌC (TRIGONOMETRY & VECTOR MATH)
    // ============================================================
    float AIPlayer::dist(sf::Vector2f a, sf::Vector2f b) const {
        float dx = b.x - a.x, dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    float AIPlayer::distSq(sf::Vector2f a, sf::Vector2f b) const {
        float dx = b.x - a.x, dy = b.y - a.y;
        return dx * dx + dy * dy;
    }

    sf::Vector2f AIPlayer::normalize(sf::Vector2f v) const {
        float len = std::sqrt(v.x * v.x + v.y * v.y);
        if (len < 1e-6f) return { 0.f, 0.f };
        return { v.x / len, v.y / len };
    }

    float AIPlayer::dot(sf::Vector2f a, sf::Vector2f b) const {
        return a.x * b.x + a.y * b.y;
    }

    // Lấy vị trí khung thành địch
    sf::Vector2f AIPlayer::getEnemyGoal(Team myTeam) const {
        return (myTeam == Team::Team1)
            ? sf::Vector2f(FIELD_WIDTH - FIELD_MARGIN_X, FIELD_HEIGHT / 2.f + 35.f)
            : sf::Vector2f(FIELD_MARGIN_X, FIELD_HEIGHT / 2.f + 35.f);
    }

    // Lấy vị trí khung thành của mình
    sf::Vector2f AIPlayer::getMyGoal(Team myTeam) const {
        return (myTeam == Team::Team1)
            ? sf::Vector2f(FIELD_MARGIN_X, FIELD_HEIGHT / 2.f + 35.f)
            : sf::Vector2f(FIELD_WIDTH - FIELD_MARGIN_X, FIELD_HEIGHT / 2.f + 35.f);
    }

    // Kiểm tra đường đi có bị chặn không (Raycast)
    bool AIPlayer::isPathClear(sf::Vector2f start, sf::Vector2f end,
        const GameState* state, int ignorePieceIdx, float checkR) const
    {
        sf::Vector2f dir = end - start;
        float len = dist(start, end);
        if (len < 1e-4f) return true;
        sf::Vector2f dirN = { dir.x / len, dir.y / len };

        // Nếu không truyền checkR, dùng giá trị mặc định cũ.
        // Nếu có, dùng chính xác bán kính động (ví dụ: Piece-Piece = 50px, Ball-Piece = 37px)
        // ---> FIX ĐỒNG BỘ HITBOX VỚI PHYSICS ENGINE <---
        // Thu nhỏ bán kính kiểm tra đường đạn tương ứng với PhysicsEngine (0.80f và 0.85f)
        float actualCheckR;
        if (checkR < 0.f) {
            actualCheckR = (PIECE_RADIUS * 0.80f) + (BALL_RADIUS * 0.85f);
        }
        else {
            // Nếu có truyền vào checkR cụ thể (vd lúc quét đường cho cầu thủ chạy)
            actualCheckR = checkR * 0.80f;
        }

        for (int i = 0; i < (int)state->getPieces().size(); ++i) {
            if (i == ignorePieceIdx) continue;
            sf::Vector2f pPos = state->getPieces()[i]->getPosition();
            sf::Vector2f toPiece = pPos - start;
            float proj = dot(toPiece, dirN);
            if (proj < 0.f || proj > len) continue;
            sf::Vector2f closest = { start.x + dirN.x * proj, start.y + dirN.y * proj };
            if (distSq(pPos, closest) < actualCheckR * actualCheckR) return false;
        }
        return true;
    }

    float AIPlayer::calculateShotPower(float dPieceToBall, float dBallToTarget, float targetVelocity) const {
        const float k = -60.0f * std::log(FRICTION); // ~ 0.9068
        const float e = 0.85f;
        const float C = (1.0f + e) / (1.0f / BALL_MASS + 1.0f / PIECE_MASS); // 1.734375

        float v_ball_initial = targetVelocity + k * dBallToTarget;
        float v_piece_impact = v_ball_initial / C;
        float v_piece_initial = v_piece_impact + k * dPieceToBall;

        return v_piece_initial;
    }

    // Tính góc giữa hướng sút và hướng về gôn (độ)
    float AIPlayer::shotAngle(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos) const {
        sf::Vector2f toBall = normalize(ballPos - piecePos);
        sf::Vector2f toGoal = normalize(goalPos - ballPos);
        float d = std::max(-1.f, std::min(1.f, dot(toBall, toGoal)));
        return std::acos(d) * 180.f / PI;
    }

    // Điểm va chạm tối ưu: đứng sau bóng theo hướng mục tiêu
    sf::Vector2f AIPlayer::optimalHitPoint(sf::Vector2f ballPos, sf::Vector2f goalTarget) const {
        sf::Vector2f dirToGoal = normalize(goalTarget - ballPos);
        // ---> FIX ĐỒNG BỘ HITBOX VỚI PHYSICS ENGINE <---
        // Khoảng cách từ tâm bóng đến tâm cầu thủ lúc chạm nhau giờ đã nhỏ hơn
        float hitDistance = (BALL_RADIUS * 0.85f) + (PIECE_RADIUS * 0.80f);

        return {
            ballPos.x - dirToGoal.x * hitDistance,
            ballPos.y - dirToGoal.y * hitDistance
        };
    }

    // Góc nhìn gôn từ vị trí bóng (radian) – dùng atan2 chuẩn
    float AIPlayer::goalViewAngle(sf::Vector2f ballPos, sf::Vector2f goalCenter, float halfH) const {
        sf::Vector2f toTop = { goalCenter.x - ballPos.x, goalCenter.y - halfH - ballPos.y };
        sf::Vector2f toBot = { goalCenter.x - ballPos.x, goalCenter.y + halfH - ballPos.y };
        float lenT = std::sqrt(toTop.x * toTop.x + toTop.y * toTop.y);
        float lenB = std::sqrt(toBot.x * toBot.x + toBot.y * toBot.y);
        if (lenT < 1e-4f || lenB < 1e-4f) return 0.f;
        float d = (toTop.x * toBot.x + toTop.y * toBot.y) / (lenT * lenB);
        return std::acos(std::max(-1.f, std::min(1.f, d)));
    }

    // Kiểm tra tình huống giao bóng (kickoff)
    bool AIPlayer::isKickoff() const {
        sf::Vector2f ballPos = state_->getBall().getPosition();
        // Bóng ở vùng trung tâm, bán kính 20px
        sf::Vector2f center = { FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f };
        return distSq(ballPos, center) < 400.f;
    }



    // Kiểm tra điểm có gần góc sân không
    float AIPlayer::scoreCornerPenalty(sf::Vector2f pos) const {
        float penalty = 0.f;
        sf::Vector2f corners[] = {
            {FIELD_MARGIN_X,              FIELD_MARGIN_Y},
            {FIELD_WIDTH - FIELD_MARGIN_X, FIELD_MARGIN_Y},
            {FIELD_MARGIN_X,              FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM},
            {FIELD_WIDTH - FIELD_MARGIN_X, FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM}
        };
        for (auto& c : corners) {
            float d = dist(pos, c);
            if (d < CORNER_ZONE_R)
                penalty += (CORNER_ZONE_R - d) * CORNER_PENALTY_W;
        }
        return penalty;
    }

    // ============================================================
    // PHÂN TÍCH TÌNH HUỐNG & MỐI ĐE DỌA
    // ============================================================

    // Điểm nguy hiểm: bóng càng gần gôn nhà và địch càng sát bóng → nguy hơn
    float AIPlayer::getBallDangerScore(Team myTeam) const {
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = state_->getBall().getPosition();
        float dBallGoal = dist(ballPos, myGoal);

        // Ngưỡng nguy hiểm: 40% chiều rộng sân
        float danger = std::max(0.f, 1.f - dBallGoal / (FIELD_WIDTH * 0.40f));

        // Địch gần bóng → tăng nguy hiểm
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        for (const auto& p : state_->getPieces()) {
            if (p->getTeam() != enemy) continue;
            float dEnemyBall = dist(p->getPosition(), ballPos);
            if (dEnemyBall < PIECE_RADIUS * 5.f)
                danger += 0.25f;
            // Địch nằm giữa bóng và gôn → cực kỳ nguy hiểm
            sf::Vector2f toGoal = normalize(myGoal - ballPos);
            sf::Vector2f toPiece = normalize(p->getPosition() - ballPos);
            if (dot(toGoal, toPiece) > 0.7f && dEnemyBall < dist(ballPos, myGoal))
                danger += 0.20f;
        }
        return std::min(1.f, danger);
    }

    // Cơ hội tấn công: góc nhìn gôn địch từ bóng, và bóng gần gôn địch
    float AIPlayer::getAttackOpportunity(Team myTeam) const {
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f ballPos = state_->getBall().getPosition();
        float angle = goalViewAngle(ballPos, enemyGoal, AI_GOAL_HALF_H);
        float opp = (angle / 1.2f) * std::max(0.f, 1.f - dist(ballPos, enemyGoal) / FIELD_WIDTH);
        return std::min(1.f, opp);
    }

    // Phân loại tình huống
    SituationMode AIPlayer::analyzeSituation(Team myTeam) const {
        float danger = getBallDangerScore(myTeam);
        if (danger > 0.75f) return SituationMode::Panic;
        if (danger > 0.45f) return SituationMode::Defending;
        if (getAttackOpportunity(myTeam) > 0.35f) return SituationMode::Attacking;
        return SituationMode::Balanced;
    }

    // Tính trọng số attack/defense theo tình huống và tỉ số
    AIPlayer::Weights AIPlayer::computeWeights(Team myTeam, SituationMode mode) const {
        Weights w;
        int scoreDiff = (myTeam == Team::Team1)
            ? state_->getScore1() - state_->getScore2()
            : state_->getScore2() - state_->getScore1();

        switch (mode) {
        case SituationMode::Panic:
            w.attack = 0.05f; w.defense = 6.0f; w.control = 0.1f; break;
        case SituationMode::Defending:
            w.attack = 0.4f;  w.defense = 3.5f; w.control = 0.8f; break;
        case SituationMode::Attacking:
            w.attack = 3.0f;  w.defense = 0.4f; w.control = 0.8f; break;
        default:
            w.attack = 1.5f;  w.defense = 1.3f; w.control = 0.6f; break;
        }
        // Điều chỉnh theo tỉ số
        if (scoreDiff < 0) { w.attack *= 1.6f; w.defense *= 0.5f; } // Đang thua → tấn công mạnh hơn
        if (scoreDiff > 0) { w.defense *= 1.6f; w.attack *= 0.6f; } // Đang thắng → bảo thủ
        return w;
    }

    // Phân tích bản đồ mối đe dọa từ cầu thủ đối phương
    std::vector<AIPlayer::ThreatInfo> AIPlayer::analyzeThreatMap(Team myTeam) const {
        std::vector<ThreatInfo> threats;
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        const auto& pieces = state_->getPieces();

        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != enemy) continue;
            sf::Vector2f pos = pieces[i]->getPosition();

            ThreatInfo t;
            t.pieceIdx = i;
            t.distToBall = dist(pos, ballPos);
            t.distToMyGoal = dist(pos, myGoal);
            t.hasLineOfSight = isPathClear(pos, ballPos, state_, i);
            t.shotAngleToGoal = shotAngle(pos, ballPos, myGoal);

            // Tính điểm mối đe dọa
            float distScore = std::max(0.f, 1.f - t.distToBall / 350.f);
            float angleScore = std::max(0.f, 1.f - t.shotAngleToGoal / 90.f);
            float goalScore = std::max(0.f, 1.f - t.distToMyGoal / (FIELD_WIDTH * 0.55f));
            float losBonus = t.hasLineOfSight ? 1.8f : 0.5f;

            // Góc nhìn gôn từ vị trí địch
            float viewAngle = goalViewAngle(pos, myGoal, AI_GOAL_HALF_H);
            t.threatScore = (distScore * 40.f + angleScore * 35.f + goalScore * 25.f
                + viewAngle * 30.f) * losBonus;

            if (t.distToBall < 300.f)
                threats.push_back(t);
        }

        std::sort(threats.begin(), threats.end(),
            [](const ThreatInfo& a, const ThreatInfo& b) {
                return a.threatScore > b.threatScore;
            });
        return threats;
    }

    // ============================================================
    // PANIC: Đẩy bóng ra xa gôn nhà nhất có thể
    // ============================================================
    void AIPlayer::genPanicClearance(std::vector<AIShot>& out, Team myTeam) {
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        const auto& pieces = state_->getPieces();

        // Hướng thoát cơ bản: từ gôn nhà ra ngoài
        sf::Vector2f baseDir = normalize(ballPos - myGoal);

        // Sinh nhiều hướng đẩy bóng (fan out 180 độ về phía sân đối phương)
        // Dùng lượng giác quét góc từ -60° đến +60° quanh hướng thoát
        std::vector<sf::Vector2f> clearDirs;
        clearDirs.reserve(13);

        for (int deg = -60; deg <= 60; deg += 10) {
            float rad = deg * PI / 180.f;
            float cs = std::cos(rad), sn = std::sin(rad);
            sf::Vector2f rotated = {
                baseDir.x * cs - baseDir.y * sn,
                baseDir.x * sn + baseDir.y * cs
            };
            clearDirs.push_back(rotated);
        }

        // Thêm hướng về phía gôn địch (nếu bóng đủ xa gôn nhà)
        if (dist(ballPos, myGoal) > 120.f) {
            clearDirs.push_back(normalize(enemyGoal - ballPos));
        }

        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != myTeam) continue;
            sf::Vector2f pos = pieces[i]->getPosition();
            if (dist(pos, ballPos) > AI_MAX_REACH_DIST) continue;

            float bestProjDist = -1e9f;
            sf::Vector2f bestDir = { 0.f, 0.f };

            for (const auto& clearDir : clearDirs) {
                // Tránh hướng hướng về gôn nhà (không phản lưới)
                sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
                if (dot(clearDir, toMyGoal) > 0.5f) continue;

                // Tránh đá vào 4 góc sân
                sf::Vector2f predictedBall = {
                    ballPos.x + clearDir.x * 300.f,
                    ballPos.y + clearDir.y * 300.f
                };
                float cornerPenalty = scoreCornerPenalty(predictedBall);
                if (cornerPenalty > 400.f) continue;

                sf::Vector2f hitPt = optimalHitPoint(ballPos, ballPos + clearDir * 500.f);
                if (!isPathClear(pos, hitPt, state_, i, PIECE_RADIUS * 2.0f - 2.0f)) continue; // Đường người sút phải trống
                if (!isPathClear(ballPos, predictedBall, state_, i, BALL_RADIUS + PIECE_RADIUS - 2.0f)) continue; // Đường phá bóng phải thông thoáng

                // Điểm dự kiến bóng sẽ ở đâu sau khi đá
                float projDistFromMyGoal = dist(predictedBall, myGoal);
                float score = projDistFromMyGoal - cornerPenalty * 0.1f;

                if (score > bestProjDist) {
                    bestProjDist = score;
                    bestDir = normalize(hitPt - pos);
                }
            }

            if (bestProjDist > -1e9f) {
                AIShot s;
                s.pieceIndex = i;
                s.velocity = bestDir * MAX_SHOOT_POWER;
                s.valid = true;
                s.heuristicScore = 2000.f + bestProjDist * 0.5f;
                out.push_back(s);
            }
        }
    }

    // ============================================================
    // LOCKDOWN: CHẶN ĐƯỜNG CỦA ĐỊCH
    // ============================================================
    void AIPlayer::genBlockingShots(std::vector<AIShot>& out, Team myTeam,
        const std::vector<ThreatInfo>& threats)
    {
        if (threats.empty()) return;
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        const auto& pieces = state_->getPieces();

        int numThreats = std::min((int)threats.size(), 3);
        for (int ti = 0; ti < numThreats; ++ti) {
            const ThreatInfo& threat = threats[ti];
            sf::Vector2f enemyPos = pieces[threat.pieceIdx]->getPosition();

            // Hướng địch→bóng
            sf::Vector2f enemyToBall = normalize(ballPos - enemyPos);
            // Vector vuông góc
            sf::Vector2f perp = { -enemyToBall.y, enemyToBall.x };
            // Hướng ra xa gôn nhà
            sf::Vector2f awayGoal = normalize(ballPos - myGoal);

            // 7 hướng đẩy bóng để phá góc nhìn địch
            sf::Vector2f pushDirs[7] = {
                perp,
                { -perp.x, -perp.y },
                awayGoal,
                normalize({ perp.x * 0.7f + awayGoal.x * 0.7f,  perp.y * 0.7f + awayGoal.y * 0.7f }),
                normalize({ -perp.x * 0.7f + awayGoal.x * 0.7f, -perp.y * 0.7f + awayGoal.y * 0.7f }),
                normalize({ perp.x * 0.5f + awayGoal.x * 1.0f,  perp.y * 0.5f + awayGoal.y * 1.0f }),
                normalize({ -perp.x * 0.5f + awayGoal.x * 1.0f, -perp.y * 0.5f + awayGoal.y * 1.0f }),
            };

            for (const auto& pushDir : pushDirs) {
                // Không đẩy về hướng gôn nhà
                sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
                if (dot(pushDir, toMyGoal) > 0.5f) continue;

                sf::Vector2f targetBall = { ballPos.x + pushDir.x * 100.f, ballPos.y + pushDir.y * 100.f };
                float cPenalty = scoreCornerPenalty(targetBall);
                if (cPenalty > 500.f) continue;

                sf::Vector2f hitPt = optimalHitPoint(ballPos, targetBall);

                for (int i = 0; i < (int)pieces.size(); ++i) {
                    if (pieces[i]->getTeam() != myTeam) continue;
                    sf::Vector2f pos = pieces[i]->getPosition();
                    if (dist(pos, hitPt) > AI_MAX_REACH_DIST) continue;
                    if (!isPathClear(pos, hitPt, state_, i)) continue;

                    sf::Vector2f dir = normalize(hitPt - pos);
                    float power = std::max(MAX_SHOOT_POWER * 0.45f,
                        std::min(MAX_SHOOT_POWER, dist(pos, hitPt) * 2.5f + 120.f));
                    AIShot s;
                    s.pieceIndex = i;
                    s.velocity = dir * power;
                    s.valid = true;
                    s.heuristicScore = 600.f + threat.threatScore - cPenalty * 0.1f;
                    out.push_back(s);
                }
            }
        }
    }

    // ============================================================
    // LOCKDOWN: PHỦ KÍN GÔN NHÀ
    // ============================================================
    void AIPlayer::genGoalCoverShots(std::vector<AIShot>& out, Team myTeam,
        const std::vector<ThreatInfo>& threats)
    {
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = state_->getBall().getPosition();
        const auto& pieces = state_->getPieces();

        // Vùng bảo vệ trước gôn (80-130px trước gôn)
        float guardX = (myTeam == Team::Team1)
            ? FIELD_MARGIN_X + 100.f
            : FIELD_WIDTH - FIELD_MARGIN_X - 100.f;

        float guardYs[] = {
            myGoal.y,
            myGoal.y - AI_GOAL_HALF_H * 0.6f,
            myGoal.y + AI_GOAL_HALF_H * 0.6f,
        };

        for (float gy : guardYs) {
            sf::Vector2f guardPos = { guardX, gy };

            // Tìm quân chưa ở vị trí phủ, và chưa sát bóng
            for (int i = 0; i < (int)pieces.size(); ++i) {
                if (pieces[i]->getTeam() != myTeam) continue;
                sf::Vector2f pos = pieces[i]->getPosition();
                if (dist(pos, ballPos) < PIECE_RADIUS * 3.f) continue; // Đừng rút quân sát bóng
                if (dist(pos, guardPos) < PIECE_RADIUS * 2.f) continue; // Đã ở vị trí rồi

                // Hướng đưa quân về vị trí bảo vệ:
                // Đá bóng vào mặt sau quân theo hướng guardPos
                sf::Vector2f pieceToGuard = normalize(guardPos - pos);
                sf::Vector2f hitOnPiece = { pos.x - pieceToGuard.x * PIECE_RADIUS * 1.1f,
                                               pos.y - pieceToGuard.y * PIECE_RADIUS * 1.1f };
                sf::Vector2f hitPt = optimalHitPoint(ballPos, hitOnPiece);

                // Tìm cầu thủ khác gần bóng để bắn
                for (int j = 0; j < (int)pieces.size(); ++j) {
                    if (j == i || pieces[j]->getTeam() != myTeam) continue;
                    sf::Vector2f jPos = pieces[j]->getPosition();
                    if (dist(jPos, ballPos) > AI_MAX_REACH_DIST) continue;
                    if (!isPathClear(jPos, hitPt, state_, j)) continue;

                    sf::Vector2f dir = normalize(hitPt - jPos);
                    float power = std::min(MAX_SHOOT_POWER * 0.7f, dist(jPos, hitPt) * 2.2f + 100.f);
                    AIShot s;
                    s.pieceIndex = j;
                    s.velocity = dir * power;
                    s.valid = true;
                    s.heuristicScore = 450.f;
                    out.push_back(s);
                }
            }
        }
    }

    // ============================================================
    // LOCKDOWN: ĐẨY BÓNG VỀ VÙNG TRUNG LẬP
    // ============================================================
    void AIPlayer::genNeutralZoneShots(std::vector<AIShot>& out, Team myTeam) {
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        const auto& pieces = state_->getPieces();

        // Vùng trung tâm an toàn (ưu tiên đẩy về phía sân địch)
        float safeX = (myTeam == Team::Team1)
            ? FIELD_WIDTH / 2.f + 60.f
            : FIELD_WIDTH / 2.f - 60.f;

        std::vector<sf::Vector2f> safeTargets = {
            { safeX, FIELD_HEIGHT / 2.f + 35.f },
            { safeX, FIELD_HEIGHT / 2.f + 35.f - 80.f },
            { safeX, FIELD_HEIGHT / 2.f + 35.f + 80.f },
            { FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f },
        };

        for (const auto& target : safeTargets) {
            // Kiểm tra không phải góc sân
            if (scoreCornerPenalty(target) > 300.f) continue;

            sf::Vector2f hitPt = optimalHitPoint(ballPos, target);
            for (int i = 0; i < (int)pieces.size(); ++i) {
                if (pieces[i]->getTeam() != myTeam) continue;
                sf::Vector2f pos = pieces[i]->getPosition();
                if (dist(pos, hitPt) > AI_MAX_REACH_DIST) continue;
                if (!isPathClear(pos, hitPt, state_, i)) continue;

                // Không đẩy về hướng gôn nhà
                sf::Vector2f myGoal = getMyGoal(myTeam);
                sf::Vector2f shotDir = normalize(hitPt - pos);
                sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
                if (dot(shotDir, toMyGoal) > 0.4f) continue;

                sf::Vector2f dir = normalize(hitPt - pos);
                float power = std::min(MAX_SHOOT_POWER * 0.8f, dist(pos, hitPt) * 2.0f + 120.f);
                AIShot s;
                s.pieceIndex = i;
                s.velocity = dir * power;
                s.valid = true;
                s.heuristicScore = 300.f;
                out.push_back(s);
            }
        }
    }

    // Phòng thủ cho tất cả quân
    void AIPlayer::genDefensiveShots_all(std::vector<AIShot>& out, Team myTeam) {
        const auto& pieces = state_->getPieces();
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != myTeam) continue;
            sf::Vector2f pos = pieces[i]->getPosition();
            if (dist(pos, ballPos) > AI_MAX_REACH_DIST) continue;
            genDefensiveShots(out, i, pos, ballPos, myGoal, myTeam);
        }
    }

    // Tổng hợp lockdown shots
    std::vector<AIShot> AIPlayer::generateLockdownShots(Team myTeam) {
        std::vector<AIShot> lockdown;
        lockdown.reserve(120);
        auto threats = analyzeThreatMap(myTeam);
        genBlockingShots(lockdown, myTeam, threats);    // Ưu tiên 1: chặn địch
        genGoalCoverShots(lockdown, myTeam, threats);   // Ưu tiên 2: phủ gôn
        genNeutralZoneShots(lockdown, myTeam);          // Ưu tiên 3: đưa bóng ra
        genDefensiveShots_all(lockdown, myTeam);        // Fallback
        return lockdown;
    }

    // Đánh giá trạng thái phòng thủ (lockdown)
    float AIPlayer::evaluateLockdownState(const GameState* sim, Team myTeam) const {
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;

        // Penalty/Reward cực lớn cho goal
        if (sim->getPhase() == GamePhase::GoalScored) {
            int myOld = (myTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
            int myNew = (myTeam == Team::Team1) ? sim->getScore1() : sim->getScore2();
            int enOld = (myTeam == Team::Team1) ? state_->getScore2() : state_->getScore1();
            int enNew = (myTeam == Team::Team1) ? sim->getScore2() : sim->getScore1();
            if (myNew > myOld) return  5e6f;
            if (enNew > enOld) return -5e6f;
        }

        float score = 0.f;

        // Bóng xa gôn nhà
        score += dist(ballPos, myGoal) * 20.f;

        // Góc nhìn gôn nhà từ bóng càng hẹp càng tốt
        float myGoalAngle = goalViewAngle(ballPos, myGoal, AI_GOAL_HALF_H);
        score -= myGoalAngle * 400.f;

        // Địch xa bóng và bị chặn
        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != enemy) continue;
            float dToBall = dist(p->getPosition(), ballPos);
            score += std::min(dToBall, 350.f) * 1.2f;
            if (!isPathClear(p->getPosition(), ballPos, sim))
                score += 120.f;
            score += dist(p->getPosition(), myGoal) * 0.8f;
        }

        // Quân mình phủ trước gôn
        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != myTeam) continue;
            float dToMyGoal = dist(p->getPosition(), myGoal);
            if (dToMyGoal < FIELD_WIDTH * 0.30f) score += 40.f;
            // Quân đứng giữa bóng và gôn
            sf::Vector2f toBall = normalize(ballPos - myGoal);
            sf::Vector2f toPiece = normalize(p->getPosition() - myGoal);
            if (dot(toBall, toPiece) > 0.7f && dToMyGoal < dist(ballPos, myGoal))
                score += 80.f;
        }

        // Phạt bóng ở vùng nguy hiểm
        float dangerZoneDepth = FIELD_MARGIN_X + 180.f;
        bool inDangerZone = (myTeam == Team::Team1)
            ? ballPos.x < dangerZoneDepth
            : ballPos.x > FIELD_WIDTH - dangerZoneDepth;
        if (inDangerZone) score -= 600.f;

        // Phạt góc sân
        score -= scoreCornerPenalty(ballPos);

        return score;
    }

    // ============================================================
    // TẤN CÔNG: SÚT THẲNG VÀO KHUNG THÀNH (tất cả cầu thủ)
    // Dùng lượng giác để tính góc và điểm va chạm tối ưu
    // ============================================================
    void AIPlayer::genDirectGoalShots(std::vector<AIShot>& out, int idx,
        sf::Vector2f piecePos, sf::Vector2f ballPos,
        sf::Vector2f enemyGoal, Team myTeam)
    {
        // Quy tắc: kickoff → không sút thẳng goal
        if (isKickoff()) return;

        // Kiểm tra không phản lưới nhà: hướng bắn phải ra xa gôn nhà
        sf::Vector2f myGoal = getMyGoal(myTeam);

        // Các điểm đích trong khung thành địch: dùng lượng giác chia khung thành
        // halfH = 52.5px, quét 7 điểm từ góc top đến bottom
        float offsets[] = {
            0.f,
            -AI_GOAL_HALF_H * 0.85f, AI_GOAL_HALF_H * 0.85f,  // Gần góc
            -AI_GOAL_HALF_H * 0.50f, AI_GOAL_HALF_H * 0.50f,  // Giữa
            -AI_GOAL_HALF_H * 0.25f, AI_GOAL_HALF_H * 0.25f,  // Chính giữa nhẹ
        };

        for (float off : offsets) {
            sf::Vector2f target = { enemyGoal.x, enemyGoal.y + off };

            // Tính điểm va chạm tối ưu bằng vector normalize (lượng giác)
            sf::Vector2f hitPt = optimalHitPoint(ballPos, target);
            sf::Vector2f toHit = normalize(hitPt - piecePos);
            sf::Vector2f ballToTarget = normalize(target - ballPos);

            // cos(θ) giữa hướng sút và hướng đến goal – dùng dot product
            float cutAngle = dot(toHit, ballToTarget);
            if (cutAngle < 0.15f) continue; // Góc quá xiên

            // Kiểm tra đường đi (Bán kính né chính xác)
            // Đường cầu thủ lao vào bóng -> né các cầu thủ khác (bán kính 2 * PIECE_RADIUS - 2.0f để lách hẹp)
            if (!isPathClear(piecePos, hitPt, state_, idx, PIECE_RADIUS * 2.0f - 2.0f)) continue;
            // Đường bóng bay tới gôn -> né các cầu thủ khác (bán kính BALL_RADIUS + PIECE_RADIUS - 2.0f)
            if (!isPathClear(ballPos, target, state_, idx, BALL_RADIUS + PIECE_RADIUS - 2.0f)) continue;

            // Kiểm tra không phản lưới nhà
            sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
            if (dot(ballToTarget, toMyGoal) > 0.6f) continue;

            // Không đá vào góc sân
            float cPenalty = scoreCornerPenalty(target);
            if (cPenalty > 600.f) continue;

            float dToHit = dist(piecePos, hitPt);
            float dToGoal = dist(ballPos, target);

            // TÍNH TOÁN LỰC SÚT CHÍNH XÁC (Tránh sút quá mạnh đập ra hoặc quá nhẹ)
            // Lực đến tay môn/góc 80.f là đủ vượt qua vạch vôi gọn gàng
            float power = calculateShotPower(dToHit, dToGoal, 80.f);

            // NẾU CẦN LỰC QUÁ MẠNH SO VỚI THỂ LỰC (MAX_SHOOT_POWER) -> Giới hạn lại thay vì bỏ cuộc (để nó vẫn sút vớt vát)
            power = std::min(power, MAX_SHOOT_POWER * 1.0f);

            AIShot s;
            s.pieceIndex = idx;
            s.velocity = toHit * power;
            s.valid = true;
            // Điểm heuristic: góc thẳng > góc xiên, gần goal > xa
            s.heuristicScore = 1200.f + cutAngle * 300.f
                - std::abs(off) * 0.5f
                - cPenalty * 0.1f
                + goalViewAngle(ballPos, enemyGoal, AI_GOAL_HALF_H) * 150.f;
            out.push_back(s);
        }
    }

    // ============================================================
    // TẤN CÔNG: SÚT ĐẬP TƯỜNG (Bank Shot)
    // Dùng gương phản chiếu: target' = 2*wall - target
    // ============================================================
    void AIPlayer::genBankShots(std::vector<AIShot>& out, int idx,
        sf::Vector2f piecePos, sf::Vector2f ballPos,
        sf::Vector2f enemyGoal, Team myTeam)
    {
        if (isKickoff()) return;
        sf::Vector2f myGoal = getMyGoal(myTeam);

        // 2 tường ngang (trên và dưới)
        float walls[] = { FIELD_MARGIN_Y, FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM };
        float goalOffsets[] = { 0.f, -AI_GOAL_HALF_H * 0.6f, AI_GOAL_HALF_H * 0.6f };

        for (int wi = 0; wi < 2; ++wi) {
            float wall = walls[wi];
            for (float off : goalOffsets) {
                sf::Vector2f target = { enemyGoal.x, enemyGoal.y + off };

                // Mirror formula: phản chiếu mục tiêu qua tường (lượng giác phản chiếu)
                sf::Vector2f mirror = { target.x, 2.f * wall - target.y };
                sf::Vector2f hitPt = optimalHitPoint(ballPos, mirror);
                sf::Vector2f toHit = normalize(hitPt - piecePos);

                if (dist(piecePos, hitPt) < AI_MIN_SHOT_LEN) continue;
                // Né đường của cầu thủ (2 * PIECE_RADIUS - 2.0f)
                if (!isPathClear(piecePos, hitPt, state_, idx, PIECE_RADIUS * 2.0f - 2.0f)) continue;

                // Đảm bảo không phản lưới nhà
                sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
                if (dot(toHit, toMyGoal) > 0.55f) continue;

                // Không đá vào góc sân
                float cPenalty = scoreCornerPenalty(mirror);
                if (cPenalty > 500.f) continue;

                AIShot s;
                s.pieceIndex = idx;
                s.velocity = toHit * MAX_SHOOT_POWER * 0.92f;
                s.valid = true;
                s.heuristicScore = 500.f - std::abs(off) * 0.3f - cPenalty * 0.1f;
                out.push_back(s);
            }
        }

        // 2 tường dọc (trái và phải) – dùng khi cầu thủ ở vị trí đặc biệt
        float vWalls[] = { FIELD_MARGIN_X, FIELD_WIDTH - FIELD_MARGIN_X };
        for (int wi = 0; wi < 2; ++wi) {
            float vWall = vWalls[wi];
            for (float off : goalOffsets) {
                sf::Vector2f target = { enemyGoal.x, enemyGoal.y + off };
                sf::Vector2f mirror = { 2.f * vWall - target.x, target.y };
                sf::Vector2f hitPt = optimalHitPoint(ballPos, mirror);
                sf::Vector2f toHit = normalize(hitPt - piecePos);

                if (dist(piecePos, hitPt) < AI_MIN_SHOT_LEN) continue;
                if (!isPathClear(piecePos, hitPt, state_, idx, PIECE_RADIUS * 2.0f - 2.0f)) continue;

                sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
                if (dot(toHit, toMyGoal) > 0.55f) continue;

                float cPenalty = scoreCornerPenalty(mirror);
                if (cPenalty > 500.f) continue;

                AIShot s;
                s.pieceIndex = idx;
                s.velocity = toHit * MAX_SHOOT_POWER * 0.85f;
                s.valid = true;
                s.heuristicScore = 380.f - cPenalty * 0.1f;
                out.push_back(s);
            }
        }
    }

    // ============================================================
    // TẤN CÔNG: SÚT COMBO (chain shot qua cầu thủ trung gian)
    // ============================================================
    void AIPlayer::genChainShots(std::vector<AIShot>& out, int idx,
        sf::Vector2f piecePos, sf::Vector2f ballPos,
        sf::Vector2f enemyGoal, Team myTeam)
    {
        float ballToGoal = dist(ballPos, enemyGoal);
        sf::Vector2f myGoal = getMyGoal(myTeam);

        for (int j = 0; j < (int)state_->getPieces().size(); ++j) {
            if (j == idx) continue;
            sf::Vector2f otherPos = state_->getPieces()[j]->getPosition();

            // Cầu thủ trung gian phải gần gôn địch hơn bóng
            if (dist(otherPos, enemyGoal) >= ballToGoal * 0.88f) continue;
            // Và phải có góc nhìn gôn tốt
            if (goalViewAngle(otherPos, enemyGoal, AI_GOAL_HALF_H) < 0.12f) continue;

            sf::Vector2f hitPt = optimalHitPoint(ballPos, otherPos);
            sf::Vector2f toHit = normalize(hitPt - piecePos);
            if (dist(piecePos, hitPt) < AI_MIN_SHOT_LEN) continue;
            if (!isPathClear(piecePos, hitPt, state_, idx)) continue;

            // Không phản lưới
            sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
            if (dot(toHit, toMyGoal) > 0.55f) continue;

            float power = std::min(MAX_SHOOT_POWER, dist(piecePos, hitPt) * 2.8f + 150.f);
            // Điểm thưởng: cầu thủ trung gian càng gần gôn địch hơn bóng → càng tốt
            float progressScore = ballToGoal - dist(otherPos, enemyGoal);
            AIShot s;
            s.pieceIndex = idx;
            s.velocity = toHit * power;
            s.valid = true;
            s.heuristicScore = 350.f + progressScore * 0.6f;
            out.push_back(s);
        }
    }

    // ============================================================
    // TẤN CÔNG: SÚT ÁP LỰC (đưa bóng vào vùng nguy hiểm gần gôn địch)
    // ============================================================
    void AIPlayer::genPressureShots(std::vector<AIShot>& out, int idx,
        sf::Vector2f piecePos, sf::Vector2f ballPos,
        sf::Vector2f enemyGoal, Team myTeam)
    {
        float pressX = (myTeam == Team::Team1)
            ? FIELD_WIDTH - FIELD_MARGIN_X - 90.f
            : FIELD_MARGIN_X + 90.f;
        sf::Vector2f myGoal = getMyGoal(myTeam);

        sf::Vector2f pressTargets[] = {
            { pressX, FIELD_HEIGHT / 2.f + 35.f },
            { pressX, FIELD_HEIGHT / 2.f + 35.f - 70.f },
            { pressX, FIELD_HEIGHT / 2.f + 35.f + 70.f },
        };

        for (auto& t : pressTargets) {
            float cPenalty = scoreCornerPenalty(t);
            if (cPenalty > 400.f) continue;

            sf::Vector2f hitPt = optimalHitPoint(ballPos, t);
            sf::Vector2f toHit = normalize(hitPt - piecePos);
            if (dist(piecePos, hitPt) < AI_MIN_SHOT_LEN) continue;
            if (!isPathClear(piecePos, hitPt, state_, idx)) continue;

            sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
            if (dot(toHit, toMyGoal) > 0.45f) continue;

            AIShot s;
            s.pieceIndex = idx;
            s.velocity = toHit * MAX_SHOOT_POWER * 0.78f;
            s.valid = true;
            s.heuristicScore = 200.f - cPenalty * 0.05f;
            out.push_back(s);
        }
    }

    // ============================================================
    // PHÒNG THỦ: SÚT ĐẨY BÓNG VỀ PHÍA ĐỊCH (clear ball)
    // Dùng lượng giác xoay hướng ±angles xung quanh hướng clear
    // ============================================================
    void AIPlayer::genDefensiveShots(std::vector<AIShot>& out, int idx,
        sf::Vector2f piecePos, sf::Vector2f ballPos,
        sf::Vector2f myGoal, Team myTeam)
    {
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f clearDir = normalize(enemyGoal - myGoal);

        // Quét góc ±40° quanh hướng về phía địch
        float angles[] = { 0.f, 15.f, -15.f, 30.f, -30.f, 40.f, -40.f };
        for (float angleDeg : angles) {
            float rad = angleDeg * PI / 180.f;
            float cs = std::cos(rad), sn = std::sin(rad);
            // Ma trận quay 2D
            sf::Vector2f dir = {
                clearDir.x * cs - clearDir.y * sn,
                clearDir.x * sn + clearDir.y * cs
            };

            // Kiểm tra không đá về gôn nhà
            sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
            if (dot(dir, toMyGoal) > 0.4f) continue;

            // Kiểm tra không đá vào góc sân
            sf::Vector2f predictedBall = { ballPos.x + dir.x * 200.f, ballPos.y + dir.y * 200.f };
            float cPenalty = scoreCornerPenalty(predictedBall);
            if (cPenalty > 500.f) continue;

            sf::Vector2f hitPt = optimalHitPoint(ballPos,
                { ballPos.x + dir.x * 100.f, ballPos.y + dir.y * 100.f });
            sf::Vector2f toHit = normalize(hitPt - piecePos);
            if (dist(piecePos, hitPt) < AI_MIN_SHOT_LEN) continue;
            if (!isPathClear(piecePos, hitPt, state_, idx)) continue;

            float power = MAX_SHOOT_POWER * (0.72f + std::abs(angleDeg) * 0.002f);
            AIShot s;
            s.pieceIndex = idx;
            s.velocity = toHit * power;
            s.valid = true;
            s.heuristicScore = 180.f - std::abs(angleDeg) * 2.f - cPenalty * 0.05f;
            out.push_back(s);
        }
    }

    // ============================================================
    // TỔNG HỢP TẤT CẢ ỨNG CỬ VIÊN TẤN CÔNG
    // ============================================================
    std::vector<AIShot> AIPlayer::generateAllCandidates(Team myTeam) {
        std::vector<AIShot> candidates;
        candidates.reserve(300);
        const auto& pieces = state_->getPieces();
        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f myGoal = getMyGoal(myTeam);
        SituationMode mode = analyzeSituation(myTeam);

        // PANIC: chỉ tìm cách phá bóng ra xa
        if (mode == SituationMode::Panic) {
            genPanicClearance(candidates, myTeam);
            return candidates;
        }

        // Xác định idx thủ môn trước khi sinh candidate
        int gkIdx = getGoalkeeperIdx(state_, myTeam);

        // Nếu bóng cực gần gôn địch (trong 20% chiều rộng sân) thỵ môn mới được dâng lên
        float dBallToEnemyGoal = dist(ballPos, enemyGoal);
        bool isGoldenOpportunity = dBallToEnemyGoal < FIELD_WIDTH * 0.20f;

        // Xét tất cả cầu thủ (không giới hạn khoảng cách – mọi vị trí đều có thể ghi bàn)
        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != myTeam) continue;
            sf::Vector2f pos = pieces[i]->getPosition();

            // Giới hạn mềm: quá xa thì bỏ qua (tiết kiệm tính toán)
            if (dist(pos, ballPos) > AI_MAX_REACH_DIST + PIECE_RADIUS) continue;

            bool isGoalkeeper = (i == gkIdx);

            if (isGoalkeeper && !isGoldenOpportunity) {
                // THỦ MÔN KHÔNG ĐƯỢC TẤN CÔNG TRU khi không phải cơ hội vàng
                // Chỉ cho phép phòng thủ và che gôn
                genDefensiveShots(candidates, i, pos, ballPos, myGoal, myTeam);
            }
            else {
                // Các cầu thủ khác (và thủ môn khi có cơ hội vàng): tấn công bình thường
                genDirectGoalShots(candidates, i, pos, ballPos, enemyGoal, myTeam);
                genBankShots(candidates, i, pos, ballPos, enemyGoal, myTeam);
                genChainShots(candidates, i, pos, ballPos, enemyGoal, myTeam);
                genPressureShots(candidates, i, pos, ballPos, enemyGoal, myTeam);

                // Phòng thủ nhẹ khi không tấn công
                if (mode != SituationMode::Attacking) {
                    genDefensiveShots(candidates, i, pos, ballPos, myGoal, myTeam);
                }
            }
        }

        return candidates;
    }

    // ============================================================
    // ĐÁNH GIÁ: Các hàm thành phần heuristic
    // ============================================================

    // Điểm tấn công: bóng gần gôn địch, góc nhìn rộng, quân mình có lợi thế
    float AIPlayer::scoreGoalThreat(const GameState* sim, Team myTeam) const {
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();
        float score = (FIELD_WIDTH - dist(ballPos, enemyGoal)) * 14.f;
        // Góc nhìn gôn địch từ bóng (radian * hệ số)
        score += goalViewAngle(ballPos, enemyGoal, AI_GOAL_HALF_H) * 250.f;
        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != myTeam) continue;
            float dToBall = dist(p->getPosition(), ballPos);
            // Cầu thủ gần bóng và có góc sút tốt → thưởng
            if (dToBall < PIECE_RADIUS * 6.f) {
                score += 50.f;
                float sa = shotAngle(p->getPosition(), ballPos, enemyGoal);
                if (sa < 20.f) score += 180.f;
                else if (sa < 40.f) score += 80.f;
            }
            if (dist(p->getPosition(), enemyGoal) < FIELD_WIDTH * 0.30f) score += 25.f;
        }
        return score;
    }

    // Điểm nguy hiểm phòng thủ: bóng gần gôn nhà, địch có lợi thế
    float AIPlayer::scoreDefenseDanger(const GameState* sim, Team myTeam) const {
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        float danger = std::max(0.f, FIELD_WIDTH * 0.5f - dist(ballPos, myGoal)) * 12.f;
        danger += goalViewAngle(ballPos, myGoal, AI_GOAL_HALF_H) * 220.f;
        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != enemy) continue;
            if (dist(p->getPosition(), ballPos) < PIECE_RADIUS * 6.f) danger += 80.f;
            if (dist(p->getPosition(), myGoal) < FIELD_WIDTH * 0.28f) danger += 35.f;
        }
        return danger;
    }

    // Điểm kiểm soát bóng: quân mình sát bóng hơn địch
    float AIPlayer::scoreBallControl(const GameState* sim, Team myTeam) const {
        sf::Vector2f ballPos = sim->getBall().getPosition();
        float closestMy = 1e9f, closestEn = 1e9f;
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        for (const auto& p : sim->getPieces()) {
            float d = dist(p->getPosition(), ballPos);
            if (p->getTeam() == myTeam) closestMy = std::min(closestMy, d);
            else                        closestEn = std::min(closestEn, d);
        }
        return (closestEn - closestMy) * 2.0f;
    }

    // Điểm vị trí sân: bóng ở nửa sân địch → tốt hơn
    float AIPlayer::scoreFieldPosition(const GameState* sim, Team myTeam) const {
        sf::Vector2f ballPos = sim->getBall().getPosition();
        float midX = FIELD_WIDTH / 2.f;
        return (myTeam == Team::Team1) ? (ballPos.x - midX) * 0.7f : (midX - ballPos.x) * 0.7f;
    }

    // Điểm đội hình: quân mình ở nửa sân địch → hỗ trợ tốt hơn
    float AIPlayer::scorePieceFormation(const GameState* sim, Team myTeam) const {
        float midX = FIELD_WIDTH / 2.f;
        float score = 0.f;
        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != myTeam) continue;
            if (myTeam == Team::Team1 && p->getPosition().x > midX) score += 10.f;
            if (myTeam == Team::Team2 && p->getPosition().x < midX) score += 10.f;
        }
        return score;
    }

    // ============================================================
    // ĐÁNH GIÁ GÔN BỊ HỞ SAU CÚ SÚT (quan trọng nhất cho vấn đề hiện tại)
    // ============================================================
    float AIPlayer::scoreGoalExposure(const GameState* sim, Team myTeam) const {
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();
        float exposure = 0.f;

        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != enemy) continue;
            sf::Vector2f ePos = p->getPosition();
            float dToMyGoal = dist(ePos, myGoal);

            // Địch trong tầm nguy hiểm (60% sân bên mình)
            if (dToMyGoal > FIELD_WIDTH * 0.60f) continue;

            // Tính góc nhìn gôn từ cầu thủ địch
            float viewAngle = goalViewAngle(ePos, myGoal, AI_GOAL_HALF_H);
            if (viewAngle < 0.05f) continue;  // Góc nhìn quá hẹp, không nguy

            // Kiểm tra line-of-sight từ địch đến gôn nhà
            bool hasLOS = isPathClear(ePos, myGoal, sim);

            // Tính mức độ nguy hiểm:
            float proxScore = std::max(0.f, 1.f - dToMyGoal / (FIELD_WIDTH * 0.60f));
            float angleScore = viewAngle * 2.0f;  // radian → điểm
            float losMult = hasLOS ? 2.5f : 0.5f;  // Có đường thẳng vào gôn → rất nguy

            // Kiểm tra có quân mình bảo vệ không
            bool hasDefender = false;
            for (const auto& myP : sim->getPieces()) {
                if (myP->getTeam() != myTeam) continue;
                sf::Vector2f mPos = myP->getPosition();
                // Quân mình đứng giữa địch và gôn (trong cone bảo vệ)
                sf::Vector2f eToGoal = normalize(myGoal - ePos);
                sf::Vector2f eToMy = normalize(mPos - ePos);
                float align = dot(eToGoal, eToMy);
                float dDefToGoal = dist(mPos, myGoal);
                if (align > 0.6f && dDefToGoal < dToMyGoal) {
                    hasDefender = true;
                    break;
                }
            }

            float dangerMult = hasDefender ? 1.0f : 3.0f;
            exposure += (proxScore * 200.f + angleScore * 150.f) * losMult * dangerMult;
        }
        return exposure;
    }

    float AIPlayer::scorePostShotDefense(const GameState* sim, Team myTeam) const {
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();
        float score = 0.f;
        int defenders = 0;

        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != myTeam) continue;
            float dToMyGoal = dist(p->getPosition(), myGoal);
            if (dToMyGoal < FIELD_WIDTH * 0.35f) {
                defenders++;
                score += 40.f;
            }
            sf::Vector2f toBall = normalize(ballPos - myGoal);
            sf::Vector2f toPiece = normalize(p->getPosition() - myGoal);
            float align = dot(toBall, toPiece);
            if (align > 0.65f && dToMyGoal < dist(ballPos, myGoal))
                score += 70.f;
        }
        if (defenders >= 2) score += 100.f;
        if (defenders == 0) score -= 300.f;
        return score;
    }

    float AIPlayer::evaluateState(const GameState* sim, Team myTeam) const {
        if (sim->getPhase() == GamePhase::GoalScored) {
            int myOld = (myTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
            int myNew = (myTeam == Team::Team1) ? sim->getScore1() : sim->getScore2();
            int enOld = (myTeam == Team::Team1) ? state_->getScore2() : state_->getScore1();
            int enNew = (myTeam == Team::Team1) ? sim->getScore2() : sim->getScore1();
            if (myNew > myOld) return  SCORE_GOAL_REWARD;
            if (enNew > enOld) return  OWN_GOAL_PENALTY;
        }

        SituationMode mode = analyzeSituation(myTeam);
        Weights w = computeWeights(myTeam, mode);

        sf::Vector2f ballPos = sim->getBall().getPosition();
        sf::Vector2f ballVel = sim->getBall().getVelocity();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        float score = 0.f;

        float bSpeed = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y);
        if (bSpeed > 8.f) {
            sf::Vector2f ballDir = { ballVel.x / bSpeed, ballVel.y / bSpeed };
            sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
            float dangerAlign = dot(ballDir, toMyGoal);
            if (dangerAlign > 0.85f) score -= 80000.f;
        }

        score += w.attack * scoreGoalThreat(sim, myTeam);
        score -= w.defense * scoreDefenseDanger(sim, myTeam);
        score += w.control * scoreBallControl(sim, myTeam);
        score += scoreFieldPosition(sim, myTeam) * 0.5f;

        float exposurePenalty = scoreGoalExposure(sim, myTeam);
        score -= exposurePenalty * w.defense * 2.0f;

        score += scorePostShotDefense(sim, myTeam) * w.defense * 1.0f;
        score -= scoreCornerPenalty(ballPos) * 1.2f;

        score += scoreGoalkeeperPosition(sim, myTeam);

        for (const auto& p : sim->getPieces()) {
            if (p->getTeam() != myTeam) continue;
            sf::Vector2f pos = p->getPosition();
            if (pos.x < FIELD_MARGIN_X || pos.x > FIELD_WIDTH - FIELD_MARGIN_X) {
                score -= 15000.f;
            }
        }

        return score;
    }

    bool AIPlayer::hasRealScoringChance(Team myTeam) {
        if (isKickoff()) return false;
        auto candidates = generateAllCandidates(myTeam);
        if (candidates.empty()) return false;
        selectTopCandidates(candidates, myTeam, 8);
        for (const auto& shot : candidates) {
            auto sim = std::unique_ptr<GameState>(state_->clone());
            sim->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);
            runPhysics(sim.get(), 110);
            if (sim->getPhase() == GamePhase::GoalScored) {
                int myOld = (myTeam == Team::Team1) ? state_->getScore1() : state_->getScore2();
                int myNew = (myTeam == Team::Team1) ? sim->getScore1() : sim->getScore2();
                if (myNew > myOld) return true;
            }
        }
        return false;
    }

    float AIPlayer::quickHeuristic(const AIShot& shot, Team myTeam) const {
        float score = (shot.heuristicScore > 0.f) ? shot.heuristicScore : 100.f;
        if (shot.pieceIndex < 0 || shot.pieceIndex >= (int)state_->getPieces().size()) return -1e9f;

        sf::Vector2f ballPos = state_->getBall().getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f vel = shot.velocity;
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        if (speed < 1.f) return -1e9f;
        sf::Vector2f dir = { vel.x / speed, vel.y / speed };

        sf::Vector2f toMyGoal = normalize(myGoal - ballPos);
        if (dot(dir, toMyGoal) > 0.55f) return -1e9f;

        sf::Vector2f toEnemyGoal = normalize(enemyGoal - ballPos);
        score += dot(dir, toEnemyGoal) * 300.f;
        score += goalViewAngle(ballPos, enemyGoal, AI_GOAL_HALF_H) * 200.f;
        score += (FIELD_WIDTH - dist(ballPos, enemyGoal)) * 0.6f;

        int gkIdx = getGoalkeeperIdx(state_, myTeam);
        if (shot.pieceIndex == gkIdx) {
            if (speed > MAX_SHOOT_POWER * 0.4f) score -= 2500.f;
            if (isKickoff()) score -= 10000.f;
        }

        return score;
    }

    std::vector<AIShot> AIPlayer::selectTopCandidates(std::vector<AIShot>& all, Team myTeam, int topN) {
        for (auto& s : all) s.heuristicScore = quickHeuristic(s, myTeam);
        all.erase(std::remove_if(all.begin(), all.end(), [](const AIShot& s) { return s.heuristicScore < -1e8f; }), all.end());
        if ((int)all.size() > topN) {
            std::partial_sort(all.begin(), all.begin() + topN, all.end(),
                [](const AIShot& a, const AIShot& b) { return a.heuristicScore > b.heuristicScore; });
            all.resize(topN);
        }
        return all;
    }

    void AIPlayer::runPhysics(GameState* sim, int maxSteps) const {
        const float fixedDt = 1.f / 60.f;
        for (int i = 0; i < maxSteps; ++i) {
            if (sim->isEverythingStopped()) {
                break;
            }
            if (sim->getPhase() == GamePhase::GoalScored) break;
            sim->update(fixedDt);
        }
        if (sim->isEverythingStopped()) {
            sim->resolveGoalCollisions();
        }
    }

    std::vector<AIShot> AIPlayer::generateEnemyResponses(GameState* sim, Team enemyTeam) {
        std::vector<AIShot> responses;
        responses.reserve(64);
        const auto& pieces = sim->getPieces();
        sf::Vector2f ballPos = sim->getBall().getPosition();
        sf::Vector2f enemyGoalPos = getEnemyGoal(enemyTeam);
        sf::Vector2f myGoalPos = getMyGoal(enemyTeam);

        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != enemyTeam) continue;
            sf::Vector2f pos = pieces[i]->getPosition();
            genDirectGoalShots(responses, i, pos, ballPos, enemyGoalPos, enemyTeam);
            genBankShots(responses, i, pos, ballPos, enemyGoalPos, enemyTeam);
            if (dist(pos, ballPos) <= AI_MAX_REACH_DIST)
                genDefensiveShots(responses, i, pos, ballPos, myGoalPos, enemyTeam);
        }

        for (auto& s : responses) s.heuristicScore = quickHeuristic(s, enemyTeam);
        int limit = AI_ENEMY_RESPONSES;
        if ((int)responses.size() > limit) {
            std::partial_sort(responses.begin(), responses.begin() + limit, responses.end(),
                [](const AIShot& a, const AIShot& b) { return a.heuristicScore > b.heuristicScore; });
            responses.resize(limit);
        }
        return responses;
    }

    float AIPlayer::minimaxAlphaBeta(GameState* simState, int depth, float alpha, float beta, Team maximizingTeam, Team myTeam) {
        if (depth == 0 || simState->getPhase() == GamePhase::GoalScored)
            return evaluateState(simState, myTeam);

        Team enemy = (maximizingTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        if (maximizingTeam == myTeam) {
            float maxVal = -1e15f;
            for (const auto& shot : generateEnemyResponses(simState, maximizingTeam)) {
                auto child = std::unique_ptr<GameState>(simState->clone());
                if (shot.pieceIndex >= 0 && shot.pieceIndex < (int)child->getPieces().size()) {
                    child->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);
                    runPhysics(child.get(), 120);
                }
                float val = minimaxAlphaBeta(child.get(), depth - 1, alpha, beta, enemy, myTeam);
                maxVal = std::max(maxVal, val);
                alpha = std::max(alpha, val);
                if (beta <= alpha) break;
            }
            return maxVal;
        }
        else {
            float minVal = 1e15f;
            for (const auto& shot : generateEnemyResponses(simState, maximizingTeam)) {
                auto child = std::unique_ptr<GameState>(simState->clone());
                if (shot.pieceIndex >= 0 && shot.pieceIndex < (int)child->getPieces().size()) {
                    child->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);
                    runPhysics(child.get(), 120);
                }
                float val = minimaxAlphaBeta(child.get(), depth - 1, alpha, beta, myTeam, myTeam);
                minVal = std::min(minVal, val);
                beta = std::min(beta, val);
                if (beta <= alpha) break;
            }
            return minVal;
        }
    }

    AIShot AIPlayer::computeShot() {
        if (!state_) return AIShot{};
        Team myTeam = state_->getCurrentTurn();
        Team enemy = (myTeam == Team::Team1) ? Team::Team2 : Team::Team1;
        SituationMode mode = analyzeSituation(myTeam);
        std::vector<AIShot> candidates;

        if (mode == SituationMode::Panic) genPanicClearance(candidates, myTeam);
        else if (mode == SituationMode::Defending) {
            candidates = generateLockdownShots(myTeam);
            auto attackExtra = generateAllCandidates(myTeam);
            for (auto& s : attackExtra) s.heuristicScore = quickHeuristic(s, myTeam) * 0.4f;
            candidates.insert(candidates.end(), attackExtra.begin(), attackExtra.end());
        }
        else {
            candidates = generateAllCandidates(myTeam);
            if (isKickoff()) {
                sf::Vector2f ballPos = state_->getBall().getPosition();
                sf::Vector2f center = { FIELD_WIDTH / 2.f + (myTeam == Team::Team1 ? 80.f : -80.f), FIELD_HEIGHT / 2.f + 40.f };
                for (int i = 0; i < (int)state_->getPieces().size(); ++i) {
                    if (state_->getPieces()[i]->getTeam() != myTeam) continue;
                    sf::Vector2f hitPt = optimalHitPoint(ballPos, center);
                    if (isPathClear(state_->getPieces()[i]->getPosition(), hitPt, state_, i))
                        candidates.push_back({ i, normalize(hitPt - state_->getPieces()[i]->getPosition()) * (MAX_SHOOT_POWER * 0.7f), true, 600.f });
                }
            }
        }

        // BULLETPROOF FIX: Lọc bỏ ngay lập tức những cú sút có lực quá yếu (dưới 5.f gây đứng im lập tức).
        // Phục vụ cho các trường hợp toán học dính điểm chết (Vector normalize trả về {0,0}).
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [](const AIShot& s) {
            return (s.velocity.x * s.velocity.x + s.velocity.y * s.velocity.y) < 25.f; // Nhỏ hơn MIN_VELOCITY_PIECE (5.0f)
            }), candidates.end());

        // LAMBDA FALLBACK: Hàm đá cứu rỗi khi bế tắc
        auto executeFallback = [&]() -> AIShot {
            AIShot fallback;
            int bestIdx = -1;
            float minDist = 1e9f;

            // Lấy vị trí quả bóng thay vì vị trí gôn!
            sf::Vector2f ballPos = state_->getBall().getPosition();

            // 1. Tìm cầu thủ của đội mình GẦN BÓNG NHẤT
            for (int i = 0; i < (int)state_->getPieces().size(); ++i) {
                if (state_->getPieces()[i]->getTeam() == myTeam) {
                    float d = dist(state_->getPieces()[i]->getPosition(), ballPos);
                    if (d < minDist) {
                        minDist = d;
                        bestIdx = i;
                    }
                }
            }

            if (bestIdx != -1) {
                fallback.pieceIndex = bestIdx;
                sf::Vector2f myPos = state_->getPieces()[bestIdx]->getPosition();

                // 2. Chĩa hướng sút thẳng từ cầu thủ vào QUẢ BÓNG
                sf::Vector2f toBall = ballPos - myPos;

                if (distSq(toBall, { 0.f, 0.f }) < 1.f) {
                    toBall = sf::Vector2f(1.f, 0.f);
                }

                // 3. Sút với lực 40% (khều nhẹ bóng để phát)
                fallback.velocity = normalize(toBall) * (MAX_SHOOT_POWER * 0.40f);
                fallback.valid = true;
                return fallback;
            }
            return AIShot{};
            };

        // Nếu trống từ đầu -> Đá cứu rỗi
        if (candidates.empty()) return executeFallback();

        // Chấm điểm và lọc các cú sút tốt nhất (Quá trình này có thể xóa sạch mảng nếu cú sút quá tệ)
        selectTopCandidates(candidates, myTeam, AI_TOP_CANDIDATES);

        // FIX LỖI CRASH TRONG ẢNH CỦA BẠN CHÍNH LÀ Ở ĐÂY:
        // Cần kiểm tra lại mảng một lần nữa, nếu lọc xong mà mảng bị rỗng -> Đá cứu rỗi
        if (candidates.empty()) return executeFallback();

        AIShot bestShot = candidates.front();
        float bestScore = -1e18f;
        float alpha = -1e18f, beta = 1e18f;

        for (const auto& shot : candidates) {
            auto sim = std::unique_ptr<GameState>(state_->clone());
            sim->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);
            runPhysics(sim.get(), AI_SIM_STEPS);
            float score = minimaxAlphaBeta(sim.get(), AI_MINIMAX_DEPTH - 1, alpha, beta, enemy, myTeam);
            if (std::isnan(score)) score = -1e17f; // Tránh lỗi số học
            if (score > bestScore) {
                bestScore = score;
                bestShot = shot;
                alpha = std::max(alpha, score);
            }
        }
        bestShot.valid = true;
        return bestShot;
    }

    int AIPlayer::getGoalkeeperIdx(const GameState* sim, Team team) const {
        const auto& pieces = sim->getPieces();
        int bestIdx = -1;
        float bestScore = -1e12f;
        sf::Vector2f targetY = { 0.f, FIELD_HEIGHT / 2.f + 35.f }; // Tâm gôn

        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i]->getTeam() != team) continue;
            sf::Vector2f pos = pieces[i]->getPosition();

            // Điểm dựa trên độ sâu (X) - Càng sát gôn nhà càng tốt
            float depthScore = (team == Team::Team1) ? (FIELD_WIDTH - pos.x) : pos.x;

            // Điểm dựa trên độ gần tâm (Y) - Càng gần trung tâm gôn càng tốt
            float centerScore = (FIELD_HEIGHT - std::abs(pos.y - targetY.y));

            float total = depthScore * 3.0f + centerScore * 1.5f;
            if (total > bestScore) {
                bestScore = total;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    float AIPlayer::scoreGoalkeeperPosition(const GameState* sim, Team myTeam) const {
        int gkIdx = getGoalkeeperIdx(sim, myTeam);
        if (gkIdx == -1) return 0.f;

        sf::Vector2f gkPos = sim->getPieces()[gkIdx]->getPosition();
        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f enemyGoal = getEnemyGoal(myTeam);
        sf::Vector2f ballPos = sim->getBall().getPosition();

        float dGkToGoal = dist(gkPos, myGoal);
        float dBallToGoal = dist(ballPos, myGoal);
        float dBallToEnemy = dist(ballPos, enemyGoal);
        float score = 0.f;

        // 1. Phạt nặng tủng bước nếu thủ môn rời xa gôn nhà quá 160px
        if (dGkToGoal > 160.f) {
            score -= (dGkToGoal - 160.f) * 600.f; // 3x phạt cũ
        }

        // 2. Phạt cực nặng nếu thủ môn dâng cao khi bóng chưa gần gôn địch (ấy là bạo liễu)
        // Chỉ miễn phạt khi bóng trong 20% sân cược đĩch (cơ hội vàng thực sự)
        bool isGoldenOpportunity = dBallToEnemy < FIELD_WIDTH * 0.20f;
        if (!isGoldenOpportunity && dGkToGoal > 180.f) {
            score -= 15000.f; // Phạt rất nặng – AI sẽ gần như không bao giờ chọn nước đi này
        }

        // 3. Phạt thêm nếu bóng đang gần gôn nhà mà thủ môn bỏ đi xa
        if (dBallToGoal < FIELD_WIDTH * 0.35f && dGkToGoal > 160.f) {
            score -= 12000.f;
        }

        // 4. Thưởng nếu thủ môn đứng chắn giữa bóng và gôn (góc thủ)
        sf::Vector2f toBall = normalize(ballPos - myGoal);
        sf::Vector2f toGk = normalize(gkPos - myGoal);
        float align = dot(toBall, toGk);
        if (align > 0.90f && dGkToGoal < 150.f) {
            score += 1500.f;
        }

        return score;
    }

} // namespace SoccerPool