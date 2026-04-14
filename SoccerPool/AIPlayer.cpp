// ====================================================================
//  AIPlayer.cpp  –  AI Công Thủ Toàn Diện v2 (High-Speed Edition)
//
//  Nguyên tắc thiết kế:
//   1. TUYỆT ĐỐI không phản lưới nhà.
//   2. Sau khi bóng dừng, bóng KHÔNG được đối diện nguy hiểm gôn nhà.
//   3. Thủ môn CHỈ tấn công khi simulation xác nhận 100% ghi bàn.
//   4. Bóng nguy hiểm → ưu tiên phá bóng lên phía địch.
//   5. Kickoff → không sút thẳng vào gôn địch.
//   6. [MỚI] Bóng sau lưng cầu thủ → KHÔNG đá về phía gôn nhà.
//   7. [MỚI] Ưu tiên cú sút trực tiếp vào gôn với lực tối đa.
//   8. [MỚI] AI ra quyết định nhanh → nhịp trận đấu cao.
// ====================================================================
#include "AIPlayer.h"
#include "Field.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <queue>

namespace SoccerPool {

    // ──────────────────────────────────────────────────────────────────
    //  HẰNG SỐ NỘI BỘ
    // ──────────────────────────────────────────────────────────────────
    static constexpr float PI = 3.14159265f;
    static constexpr float GOAL_H = GOAL_HEIGHT / 2.0f;
    static constexpr float HITBOX_SCALE = 0.80f;
    static constexpr float PP_RADIUS = PIECE_RADIUS * 1.6f;
    static constexpr float BP_RADIUS = (BALL_RADIUS + PIECE_RADIUS) * HITBOX_SCALE;

    // Vùng nguy hiểm trước gôn nhà
    static constexpr float DANGER_RADIUS = 220.f;
    static constexpr float CRITICAL_RADIUS = 120.f;
    static constexpr float DANGER_ANGLE = 0.40f;   // ~23°

    // Penalty cứng
    static constexpr float OWN_GOAL_PENALTY = -1e10f;
    static constexpr float END_DANGER_PENALTY = -80000.f;
    static constexpr float KEEPER_WANDER_PENALTY = -4000.f;

    // Lực sút AI
    static constexpr float AI_POWER_BOOST = 1.15f;
    static constexpr int   FAST_SIM_STEPS = 150;
    static constexpr int   FULL_SIM_STEPS = 600;
    static constexpr int   MAX_CANDIDATES = 80;

    // ─────────────────────────────────────────────────────────────────
    //  HẰNG SỐ CỘT DỌC GÔN (khớp với PhysicsEngine.cpp)
    //  Cột dọc: bán kính 3px, nằm tại (FIELD_MARGIN_X, GOAL_Y_OFFSET)
    //  và (FIELD_MARGIN_X, GOAL_Y_OFFSET + GOAL_HEIGHT)
    //  Vùng "chết" = postRadius + BALL_RADIUS*0.80 + buffer an toàn
    // ─────────────────────────────────────────────────────────────────
    static constexpr float POST_RADIUS = 3.0f;
    static constexpr float BALL_PHYS_R = BALL_RADIUS * 0.80f;   // 9.6f
    // Margin tối thiểu phải giữ từ tâm bóng đến tâm cột
    // = POST_RADIUS + BALL_PHYS_R + buffer 6px để chắc chắn không chạm
    static constexpr float POST_SAFE_MARGIN = POST_RADIUS + BALL_PHYS_R + 8.0f; // ~20.6px
    // Khoảng cách an toàn tính từ mép gôn (GOAL_Y_OFFSET) vào trong lòng gôn
    // Bóng phải target ở y >= GOAL_Y_OFFSET + POST_SAFE_MARGIN
    //                 và y <= GOAL_Y_OFFSET + GOAL_HEIGHT - POST_SAFE_MARGIN
    static constexpr float GOAL_SAFE_TOP = GOAL_Y_OFFSET + POST_SAFE_MARGIN;
    static constexpr float GOAL_SAFE_BOT = GOAL_Y_OFFSET + GOAL_HEIGHT - POST_SAFE_MARGIN;

    // ──────────────────────────────────────────────────────────────────
    //  CONSTRUCTOR
    // ──────────────────────────────────────────────────────────────────
    AIPlayer::AIPlayer() {}

    // ──────────────────────────────────────────────────────────────────
    //  VỊ TRÍ KHUNG THÀNH
    // ──────────────────────────────────────────────────────────────────
    sf::Vector2f AIPlayer::getGoalPos(Team team) const {
        float x = (team == Team::Team1) ? FIELD_MARGIN_X
            : FIELD_WIDTH - FIELD_MARGIN_X;
        return { x, FIELD_HEIGHT / 2.0f + 35.0f };
    }
    sf::Vector2f AIPlayer::getMyGoal(Team team)    const { return getGoalPos(team); }
    sf::Vector2f AIPlayer::getEnemyGoal(Team team) const {
        return getGoalPos(team == Team::Team1 ? Team::Team2 : Team::Team1);
    }

    // ──────────────────────────────────────────────────────────────────
    //  KIỂM TRA ĐƯỜNG ĐI
    // ──────────────────────────────────────────────────────────────────
    bool AIPlayer::isPathClear(sf::Vector2f start, sf::Vector2f end,
        const GameState* state,
        int ignoreIdx, float checkR) const {
        sf::Vector2f dir = end - start;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len < 1.0f) return true;
        sf::Vector2f unit = dir / len;
        float r = (checkR > 0) ? checkR : PP_RADIUS;

        for (int i = 0; i < (int)state->getPieces().size(); ++i) {
            if (i == ignoreIdx) continue;
            sf::Vector2f p = state->getPieces()[i]->getPosition();
            sf::Vector2f toP = p - start;
            float proj = dot(toP, unit);
            if (proj < 0.f || proj > len) continue;
            sf::Vector2f closest = start + unit * proj;
            if (distSq(p, closest) < r * r) return false;
        }
        return true;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] PHÁT HIỆN BÓNG SAU LƯNG CẦU THỦ
    //
    //  "Bóng sau lưng" = vector từ cầu thủ đến bóng hướng NGƯỢC với
    //  hướng tấn công (về phía gôn địch). Khi đó cầu thủ nếu đá thẳng
    //  vào bóng sẽ tạo ra lực đẩy bóng VỀ PHÍA GÔN MÌNH.
    //
    //  Trả về true nếu bóng ở trong vùng "sau lưng" nguy hiểm.
    //  thresh: ngưỡng cos góc (0 = 90°, âm = >90°).
    // ──────────────────────────────────────────────────────────────────
    bool AIPlayer::isBallBehindPlayer(sf::Vector2f pPos, sf::Vector2f bPos,
        Team myTeam, float thresh) const {
        sf::Vector2f eGoal = getEnemyGoal(myTeam);
        // Hướng tấn công: từ cầu thủ đến gôn địch
        sf::Vector2f attackDir = normalize(eGoal - pPos);
        // Hướng từ cầu thủ đến bóng
        sf::Vector2f toBall = normalize(bPos - pPos);
        // dot < thresh → bóng nằm phía sau lưng cầu thủ
        return dot(toBall, attackDir) < thresh;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] TÍNH HIT-POINT CHÍNH XÁC TUYỆT ĐỐI
    //
    //  Dùng lượng giác để tính chính xác điểm cầu thủ phải chạm vào
    //  bề mặt bóng để bóng bay đúng hướng `target`.
    //  Khớp 100% với PhysicsEngine (HITBOX_SCALE = 0.80f).
    // ──────────────────────────────────────────────────────────────────
    sf::Vector2f AIPlayer::optimalHitPoint(sf::Vector2f ballPos,
        sf::Vector2f target) const {
        sf::Vector2f dir = normalize(target - ballPos);
        // Tổng bán kính va chạm = BALL_RADIUS * scale + PIECE_RADIUS * scale
        float totalR = (BALL_RADIUS + PIECE_RADIUS) * HITBOX_SCALE;
        // Hit-point = tâm bóng - hướng bay * tổng bán kính
        return ballPos - dir * totalR;
    }

    // ──────────────────────────────────────────────────────────────────
    //  GÓC NHÌN KHUNG THÀNH (LƯỢNG GIÁC)
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::ballGoalViewAngle(sf::Vector2f ballPos,
        sf::Vector2f goalCenter) const {
        sf::Vector2f vecTop = { goalCenter.x - ballPos.x,
                                goalCenter.y - GOAL_H - ballPos.y };
        sf::Vector2f vecBot = { goalCenter.x - ballPos.x,
                                goalCenter.y + GOAL_H - ballPos.y };
        float d = dot(normalize(vecTop), normalize(vecBot));
        return std::acos(std::max(-1.f, std::min(1.f, d)));
    }

    // ──────────────────────────────────────────────────────────────────
    //  ĐÁNH GIÁ MỨC NGUY HIỂM BÓNG TRƯỚC GÔN NHÀ (0..1)
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::dangerToOwnGoal(sf::Vector2f ballPos, Team myTeam) const {
        sf::Vector2f mGoal = getMyGoal(myTeam);
        float d = dist(ballPos, mGoal);
        if (d > DANGER_RADIUS * 1.5f) return 0.f;
        float angle = ballGoalViewAngle(ballPos, mGoal);
        float distFactor = 1.f - std::min(d / (DANGER_RADIUS * 1.5f), 1.f);
        float angleFactor = angle / PI;
        return std::min(distFactor * 0.6f + angleFactor * 0.4f, 1.f);
    }

    // ──────────────────────────────────────────────────────────────────
    //  PHÂN TÍCH TÌNH HUỐNG
    // ──────────────────────────────────────────────────────────────────
    SituationContext AIPlayer::analyzeSituation(const GameState* sim,
        Team myTeam) const {
        SituationContext ctx;
        ctx.ballPos = sim->getBall().getPosition();
        ctx.myGoalPos = getMyGoal(myTeam);
        ctx.enemyGoalPos = getEnemyGoal(myTeam);

        ctx.isKickoff = distSq(ctx.ballPos,
            { FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f }) < 400.f;

        float midX = FIELD_WIDTH / 2.f;
        ctx.ballOnMySide = (myTeam == Team::Team1)
            ? ctx.ballPos.x < midX
            : ctx.ballPos.x > midX;

        float dScore = dangerToOwnGoal(ctx.ballPos, myTeam);
        ctx.dangerScore = dScore;

        float d = dist(ctx.ballPos, ctx.myGoalPos);
        if (d <= CRITICAL_RADIUS || dScore > 0.75f)
            ctx.danger = DangerLevel::Critical;
        else if (d <= DANGER_RADIUS || dScore > 0.40f)
            ctx.danger = DangerLevel::Danger;
        else if (ctx.ballOnMySide)
            ctx.danger = DangerLevel::Neutral;
        else
            ctx.danger = DangerLevel::Safe;

        float eAngle = ballGoalViewAngle(ctx.ballPos, ctx.enemyGoalPos);
        float eDist = dist(ctx.ballPos, ctx.enemyGoalPos);
        ctx.attackScore = eAngle / PI * 0.5f
            + (1.f - std::min(eDist / FIELD_WIDTH, 1.f)) * 0.5f;

        return ctx;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] KIỂM TRA VÙNG NGU Y HIỂM – GÓC CỘT DỌC GÔN
    //
    //  Bốn góc khung thành (junction) là điểm bất động nơi tường sân
    //  gặp mép gôn. Cầu thủ đứng gần đây khi di chuyển sang ngang rất
    //  dễ bị kẹt hoặc bị bật ngược bởi collision resolution của wall.
    //
    //  Tọa độ 4 junction (nhìn vào PhysicsEngine):
    //   - Gôn trái:  (FIELD_MARGIN_X, GOAL_Y_OFFSET)   ← cột trên
    //                (FIELD_MARGIN_X, GOAL_Y_OFFSET + GOAL_HEIGHT) ← cột dưới
    //   - Gôn phải:  (FW - FIELD_MARGIN_X, GOAL_Y_OFFSET)
    //                (FW - FIELD_MARGIN_X, GOAL_Y_OFFSET + GOAL_HEIGHT)
    //
    //  isNearGoalPostJunction  → cầu thủ có "đang trong vùng nguy hiểm"?
    //  isGoalJunctionClear     → đường pPos→hitPt có đi qua vùng nguy hiểm?
    // ──────────────────────────────────────────────────────────────────

    // Danh sách 4 junction point (tái sử dụng ở cả 2 hàm bên dưới)
    static inline void fillJunctions(sf::Vector2f(&posts)[4]) {
        posts[0] = { FIELD_MARGIN_X,                GOAL_Y_OFFSET };
        posts[1] = { FIELD_MARGIN_X,                GOAL_Y_OFFSET + GOAL_HEIGHT };
        posts[2] = { FIELD_WIDTH - FIELD_MARGIN_X,  GOAL_Y_OFFSET };
        posts[3] = { FIELD_WIDTH - FIELD_MARGIN_X,  GOAL_Y_OFFSET + GOAL_HEIGHT };
    }

    bool AIPlayer::isNearGoalPostJunction(sf::Vector2f pPos) const {
        // Bán kính vùng nguy hiểm ~ 2.8 × PIECE_RADIUS
        static constexpr float ZONE_R = 70.f;
        sf::Vector2f posts[4]; fillJunctions(posts);
        for (auto& p : posts)
            if (dist(pPos, p) < ZONE_R) return true;
        return false;
    }

    // Trả về true nếu đường pPos→hitPt KHÔNG đi quá gần junction
    // (tức là an toàn để di chuyển)
    bool AIPlayer::isGoalJunctionClear(sf::Vector2f pPos, sf::Vector2f hitPt,
        float checkR) const {
        // Bán kính kiểm tra = PIECE_PHYS_R + POST_RADIUS + buffer
        float r = (checkR > 0.f)
            ? checkR
            : (PIECE_RADIUS * HITBOX_SCALE + POST_RADIUS + 6.f); // ~28px

        sf::Vector2f dir = hitPt - pPos;
        float len = dist(pPos, hitPt);
        if (len < 1.f) return true;
        sf::Vector2f unit = dir / len;

        sf::Vector2f posts[4]; fillJunctions(posts);
        for (auto& post : posts) {
            sf::Vector2f toPost = post - pPos;
            float proj = dot(toPost, unit);
            if (proj < 0.f || proj > len) continue;   // Junction không nằm trên đoạn
            sf::Vector2f closest = pPos + unit * proj;
            if (dist(closest, post) < r) return false; // Đường đi quá gần junction
        }
        return true;
    }

    // ──────────────────────────────────────────────────────────────────
    //  BFS – ĐẾM CHƯỚNG NGẠI VẬT GIỮA from VÀ to
    // ──────────────────────────────────────────────────────────────────
    int AIPlayer::bfsObstacleCount(sf::Vector2f from, sf::Vector2f to,
        const GameState* sim, Team myTeam) const {
        sf::Vector2f dir = to - from;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len < 1.f) return 0;
        sf::Vector2f unit = dir / len;
        int count = 0;
        for (auto& p : sim->getPieces()) {
            if (p->getTeam() == myTeam) continue;
            sf::Vector2f tp = p->getPosition() - from;
            float proj = dot(tp, unit);
            if (proj < 0 || proj > len) continue;
            sf::Vector2f closest = from + unit * proj;
            if (distSq(p->getPosition(), closest) < BP_RADIUS * BP_RADIUS)
                ++count;
        }
        return count;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] ĐÁNH GIÁ MỨC ĐỘ THUẬN LỢI CỦA CẦU THỦ ĐỂ SÚT BÓNG
    //
    //  Tiêu chí (score cao = thuận lợi hơn):
    //   1. Khoảng cách cầu thủ → bóng (gần = tốt)
    //   2. Đường đến bóng có thông không (không bị chặn = tốt)
    //   3. Góc nhìn gôn địch từ bóng (rộng = tốt)
    //   4. Đường bóng → gôn địch có thông không (ít chặn = tốt)
    //   5. Hướng cầu thủ → bóng → gôn (thẳng hàng = tốt)
    //   6. Cầu thủ không phải thủ môn được ưu tiên khi tấn công
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::scorePlayerAdvantage(int idx, sf::Vector2f pPos,
        sf::Vector2f bPos,
        sf::Vector2f eGoal,
        const GameState* sim,
        Team myTeam, bool isKeeper) const {
        float score = 0.f;

        // 1. Khoảng cách cầu thủ → bóng: càng gần càng tốt
        float dToBall = dist(pPos, bPos);
        score += 4000.f / (dToBall + 1.f);

        // 2. Đường đến bóng thông không?
        sf::Vector2f hitPtDirect = optimalHitPoint(bPos, eGoal);
        bool pathToBallClear = isPathClear(pPos, hitPtDirect, sim, idx, PP_RADIUS);
        score += pathToBallClear ? 2000.f : 0.f;

        // 3. Góc nhìn gôn địch từ bóng
        float viewAngle = ballGoalViewAngle(bPos, eGoal);
        score += viewAngle * 800.f;

        // 4. Số chướng ngại vật trên đường bóng → gôn
        int obs = bfsObstacleCount(bPos, eGoal, sim, myTeam);
        score -= obs * 1200.f;

        // 5. Hướng thẳng hàng: (cầu thủ → bóng) dot (bóng → gôn)
        sf::Vector2f dirToBall = normalize(bPos - pPos);
        sf::Vector2f dirToGoal = normalize(eGoal - bPos);
        float alignment = dot(dirToBall, dirToGoal);
        score += alignment * 3000.f;

        // 6. Penalty cho thủ môn khi đang tấn công
        if (isKeeper) score -= 1500.f;

        // 7. Bóng sau lưng thì phạt nặng
        if (isBallBehindPlayer(pPos, bPos, myTeam, 0.0f)) score -= 2500.f;

        // 8. [MỚI] Cầu thủ đứng gần góc cột dọc gôn → phạt nặng
        //    vỬ trí này rất dễ bị kẹt khi di chuyển đi xử lý bóng
        if (isNearGoalPostJunction(pPos)) score -= 4000.f;

        return score;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] CHỌN CẦU THỦ TỐT NHẤT CHO TỪNG TÌNH HUỐNG
    //  Trả về danh sách idx đã được sắp xếp từ tốt → kém
    // ──────────────────────────────────────────────────────────────────
    std::vector<int> AIPlayer::selectBestPlayers(const GameState* sim,
        Team myTeam,
        sf::Vector2f bPos,
        sf::Vector2f eGoal,
        bool attackMode) const {
        int gkIdx = getGoalkeeperIdx(sim, myTeam);
        std::vector<std::pair<float, int>> ranked;

        for (int i = 0; i < (int)sim->getPieces().size(); ++i) {
            if (sim->getPieces()[i]->getTeam() != myTeam) continue;
            sf::Vector2f pPos = sim->getPieces()[i]->getPosition();
            bool isKeeper = (i == gkIdx);

            float adv = scorePlayerAdvantage(i, pPos, bPos, eGoal,
                sim, myTeam, isKeeper);
            // Khi tấn công: thủ môn ít được ưu tiên hơn (trừ thêm)
            if (attackMode && isKeeper) adv -= 3000.f;

            ranked.push_back({ adv, i });
        }

        // Sắp xếp giảm dần theo điểm thuận lợi
        std::sort(ranked.begin(), ranked.end(),
            [](auto& a, auto& b) { return a.first > b.first; });

        std::vector<int> result;
        result.reserve(ranked.size());
        for (auto& r : ranked) result.push_back(r.second);
        return result;
    }

    // ──────────────────────────────────────────────────────────────────
    //  HILL CLIMBING – TÌM GÓC SÚT TỐT NHẤT
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::hillClimbBestAngle(sf::Vector2f bPos, sf::Vector2f eGoal,
        const GameState* sim, int /*pieceIdx*/,
        float startAngle, float range,
        int iters) const {
        float bestAngle = startAngle;
        float bestScore = -1e30f;
        float step = range / (float)iters;
        sf::Vector2f directDir = normalize(eGoal - bPos);

        for (int i = -iters / 2; i <= iters / 2; ++i) {
            float angle = startAngle + step * (float)i;
            sf::Vector2f dir = { std::cos(angle), std::sin(angle) };
            float viewAngle = ballGoalViewAngle(bPos + dir * 50.f, eGoal);
            int obs = bfsObstacleCount(bPos, eGoal, sim, Team::None);
            float alignment = dot(dir, directDir);
            float score = viewAngle * 1200.f
                - (float)obs * 300.f
                + alignment * 400.f;
            if (score > bestScore) { bestScore = score; bestAngle = angle; }
        }
        return bestAngle;
    }

    // ──────────────────────────────────────────────────────────────────
    //  [MỚI] TÍNH LỰC SÚT TỐI ƯU
    //  Tính lực dựa vào khoảng cách đến gôn: xa hơn → mạnh hơn.
    //  Luôn đảm bảo bóng đủ lực để đến gôn với tốc độ cao.
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::calcOptimalPower(float distToGoal, float alignment) const {
        // Lực cơ sở: đủ mạnh để bóng đến gôn ngay cả khi có ma sát
        // Công thức: lực tỷ lệ với sqrt(khoảng cách) để bù ma sát
        float basePower = std::min(MAX_SHOOT_POWER * AI_POWER_BOOST,
            MAX_SHOOT_POWER * 0.85f
            + distToGoal * 0.35f);
        // Alignment tốt → cú sút chính xác hơn, không cần lực quá lớn
        // Alignment kém → cú sút xiên, cần lực lớn hơn để bù
        float alignFactor = (alignment > 0.7f) ? 1.0f : 1.15f;
        return std::min(basePower * alignFactor,
            MAX_SHOOT_POWER * AI_POWER_BOOST);
    }

    // ──────────────────────────────────────────────────────────────────
    //  SINH NƯỚC ĐI: SÚT THẲNG VÀO GÔN ĐỊCH
    //
    //  CẢI TIẾN CHỐNG CHẠM CỘT DỌC:
    //   - Target y chỉ được nằm trong [GOAL_SAFE_TOP, GOAL_SAFE_BOT]
    //   - Thêm kiểm tra bóng có đi QUYÉT QUA cột dọc không (trig)
    //   - Quét mỗi 2.5px để phát hiện khe hở tốt hơn
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::addDirectGoalShots(const GameState* sim,
        std::vector<AIShot>& out,
        int idx, sf::Vector2f pPos,
        sf::Vector2f bPos, sf::Vector2f eGoal,
        Team myTeam) {
        if (distSq(bPos, { FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f }) < 400.f)
            return;

        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f toMyGoal = normalize(myGoal - bPos);
        float distBallToEGoal = dist(bPos, eGoal);
        bool  ballIsRear = isBallBehindPlayer(pPos, bPos, myTeam, -0.1f);

        // Vị trí cột dọc gôn địch (2 cột: trên và dưới)
        // Team1 tấn công gôn phải, Team2 tấn công gôn trái
        float postX = eGoal.x;   // x của vạch gôn địch
        sf::Vector2f postTop = { postX, GOAL_Y_OFFSET };
        sf::Vector2f postBot = { postX, GOAL_Y_OFFSET + GOAL_HEIGHT };

        // Quét mỗi 2.5px – siêu chi tiết
        for (float targetY = GOAL_SAFE_TOP; targetY <= GOAL_SAFE_BOT; targetY += 2.5f) {
            sf::Vector2f target = { eGoal.x, targetY };
            sf::Vector2f hitPt = optimalHitPoint(bPos, target);

            if (!isPathClear(pPos, hitPt, sim, idx, PP_RADIUS)) continue;
            if (!isPathClear(bPos, target, sim, idx, BP_RADIUS)) continue;

            sf::Vector2f dirToHit = normalize(hitPt - pPos);
            sf::Vector2f dirToGoal = normalize(target - bPos);

            float alignment = dot(dirToHit, dirToGoal);
            // [FIX ĐỘ CHÍNH XÁC]
            // Alignment = dot(đh cầu thủ đi, đh bóng bay).
            // < 0.60 thì góc tiếp cận > 53° – cú đánh sẽ chệch hướng nhiều.
            // Bóng sau lưng: cần alignment 0.72+ để đảm bảo tiếp cận đúng góc.
            float minAlign = ballIsRear ? 0.72f : 0.60f;
            if (alignment < minAlign) continue;

            // Không đẩy bóng về gôn nhà
            if (dot(dirToGoal, toMyGoal) > 0.08f) continue;
            if (dot(dirToHit, toMyGoal) > 0.28f) continue;
            if (ballIsRear && dot(dirToGoal, toMyGoal) > -0.05f) continue;

            // ─────────────────────────────────────────────────────────
            // [CHỐNG CHẠM CỘT] Kiểm tra đường bóng có đi quá gần cột?
            // Tính khoảng cách từ tâm mỗi cột đến đường thẳng bóng bay
            // ─────────────────────────────────────────────────────────
            auto linePointDist = [&](sf::Vector2f lineStart, sf::Vector2f lineDir,
                sf::Vector2f point) -> float {
                    sf::Vector2f toPoint = point - lineStart;
                    float proj = dot(toPoint, lineDir);
                    sf::Vector2f closest = lineStart + lineDir * proj;
                    return dist(closest, point);
                };

            sf::Vector2f ballDir = normalize(target - bPos);
            float distToPostTop = linePointDist(bPos, ballDir, postTop);
            float distToPostBot = linePointDist(bPos, ballDir, postBot);
            float minPostClear = BALL_PHYS_R + POST_RADIUS + 4.0f; // buffer 4px

            // Bóng bay qua gần cột → bỏ qua target này
            if (distToPostTop < minPostClear) continue;
            if (distToPostBot < minPostClear) continue;

            // Chỉ kiếm tra cột nếu cột nằm GIỮA bóng và target (proj > 0)
            // Nếu cột ở sau target thì không quan trọng
            auto checkPostInPath = [&](sf::Vector2f post) -> bool {
                sf::Vector2f toPost = post - bPos;
                float proj = dot(toPost, ballDir);
                if (proj < 0.f) return false;  // Cột ở phía sau bóng
                float totalLen = dist(bPos, target);
                if (proj > totalLen + BALL_PHYS_R) return false;  // Cột sau mục tiêu
                return linePointDist(bPos, ballDir, post) < minPostClear;
                };

            if (checkPostInPath(postTop)) continue;
            if (checkPostInPath(postBot)) continue;

            // [MỚI] Kiếm tra đường đi của cầu thủ có qua góc cột-địa không
            if (!isGoalJunctionClear(pPos, hitPt)) continue;
            // ─────────────────────────────────────────────────────────

            float viewAngle = ballGoalViewAngle(bPos, eGoal);
            float distToBall = dist(pPos, bPos);
            float proxBonus = 5500.f / (distToBall + 1.f);
            int   obs = bfsObstacleCount(bPos, eGoal, sim, myTeam);
            float obsPenalty = obs * 2200.f;

            float baseAngle = std::atan2(dirToGoal.y, dirToGoal.x);
            float bestAngle = hillClimbBestAngle(bPos, eGoal, sim, idx,
                baseAngle, 0.22f, 14);
            float hillBonus = (1.f - std::abs(bestAngle - baseAngle) / 0.22f) * 700.f;

            float power = calcOptimalPower(distBallToEGoal, alignment);

            // Thưởng nhiều hơn cho target ở GIỮA gôn (tránh xa cột)
            float centerY = GOAL_Y_OFFSET + GOAL_HEIGHT / 2.f;
            float centerBonus = (1.f - std::abs(targetY - centerY) / (GOAL_HEIGHT / 2.f)) * 800.f;
            float viewBonus = viewAngle * 600.f;
            float closeBonus = (distBallToEGoal < 250.f) ? 3500.f : 0.f;

            float score = 2500.f
                + alignment * 6500.f
                + proxBonus
                + viewBonus
                + hillBonus
                + centerBonus
                + closeBonus
                - obsPenalty;

            out.push_back({ idx, dirToHit * power, true, score, false, false });
        }
    }

    // ──────────────────────────────────────────────────────────────────
    //  SINH NƯỚC ĐI: ĐẬP TƯỜNG (BANK SHOT) – cải tiến
    //  Bổ sung kiểm tra bóng sau lưng
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::addBankShots(const GameState* sim, std::vector<AIShot>& out,
        int idx, sf::Vector2f pPos,
        sf::Vector2f bPos, sf::Vector2f eGoal,
        Team myTeam) {
        if (distSq(bPos, { FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f }) < 400.f)
            return;

        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f toMyGoal = normalize(myGoal - bPos);
        float distBallToEGoal = dist(bPos, eGoal);

        // Cột dọc gôn địch
        float postX = eGoal.x;
        sf::Vector2f postTop = { postX, GOAL_Y_OFFSET };
        sf::Vector2f postBot = { postX, GOAL_Y_OFFSET + GOAL_HEIGHT };
        float minPostClear = BALL_PHYS_R + POST_RADIUS + 4.0f;

        float walls[] = { FIELD_MARGIN_Y,
                          FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM };

        for (float wall : walls) {
            // Bank shot target chỉ trong vùng an toàn (tránh cột)
            for (float targetY = GOAL_SAFE_TOP; targetY <= GOAL_SAFE_BOT; targetY += 8.f) {
                sf::Vector2f targetGoal = { eGoal.x, targetY };
                sf::Vector2f mirror = { targetGoal.x, 2.f * wall - targetGoal.y };

                float denom = mirror.y - bPos.y;
                if (std::abs(denom) < 0.1f) continue;
                float t = (wall - bPos.y) / denom;
                if (t < 0.f || t > 1.f) continue;
                sf::Vector2f wallPt = bPos + (mirror - bPos) * t;

                if (wallPt.x < FIELD_MARGIN_X || wallPt.x > FIELD_WIDTH - FIELD_MARGIN_X)
                    continue;

                sf::Vector2f hitPt = optimalHitPoint(bPos, wallPt);

                if (!isPathClear(pPos, hitPt, sim, idx, PP_RADIUS)) continue;
                if (!isPathClear(bPos, wallPt, sim, idx, BP_RADIUS)) continue;
                if (!isPathClear(wallPt, targetGoal, sim, idx, BP_RADIUS)) continue;

                sf::Vector2f dirToHit = normalize(hitPt - pPos);
                sf::Vector2f dirToWall = normalize(wallPt - bPos);

                float alignment = dot(dirToHit, dirToWall);
                if (alignment < 0.15f) continue;

                if (dot(dirToHit, toMyGoal) > 0.25f) continue;
                if (dot(dirToWall, toMyGoal) > 0.15f) continue;
                if (isBallBehindPlayer(pPos, bPos, myTeam, 0.0f)) continue;

                // Kiểm tra đoạn wallPt → targetGoal có quét qua cột không
                sf::Vector2f seg2Dir = normalize(targetGoal - wallPt);
                auto linePointDist2 = [&](sf::Vector2f ls, sf::Vector2f ld,
                    sf::Vector2f pt) -> float {
                        sf::Vector2f tp = pt - ls;
                        float p2 = dot(tp, ld);
                        return dist(ls + ld * p2, pt);
                    };
                if (linePointDist2(wallPt, seg2Dir, postTop) < minPostClear) continue;
                if (linePointDist2(wallPt, seg2Dir, postBot) < minPostClear) continue;

                // [MỚI] Đường cầu thủ đi và góc cột địa
                if (!isGoalJunctionClear(pPos, hitPt)) continue;

                float distToBall = dist(pPos, bPos);
                float proxBonus = 3000.f / (distToBall + 1.f);
                float viewAngle = ballGoalViewAngle(wallPt, eGoal);

                float incidentAngle = std::atan2(wallPt.y - bPos.y, wallPt.x - bPos.x);
                float reflectAngle = std::atan2(targetGoal.y - wallPt.y, targetGoal.x - wallPt.x);
                float symmetryScore = 600.f - std::abs(incidentAngle + reflectAngle) * 120.f;

                // Thưởng target gần giữa gôn
                float centerY = GOAL_Y_OFFSET + GOAL_HEIGHT / 2.f;
                float centerBonus = (1.f - std::abs(targetY - centerY) / (GOAL_HEIGHT / 2.f)) * 500.f;

                float power = calcOptimalPower(distBallToEGoal, alignment);

                float score = 1000.f
                    + alignment * 3000.f
                    + proxBonus
                    + viewAngle * 300.f
                    + symmetryScore
                    + centerBonus;

                out.push_back({ idx, dirToHit * power, true, score, false, false });
            }
        }
    }

    // ──────────────────────────────────────────────────────────────────
    //  SINH NƯỚC ĐI: PHÁ BÓNG PHÒNG NGỰ (cải tiến)
    //  Bổ sung kiểm tra bóng sau lưng cầu thủ
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::addDefensiveClears(const GameState* sim,
        std::vector<AIShot>& out,
        int idx, sf::Vector2f pPos,
        sf::Vector2f bPos, sf::Vector2f myGoal,
        Team myTeam, bool forceUpfield) {
        sf::Vector2f eGoal = getEnemyGoal(myTeam);
        sf::Vector2f awayDir = normalize(bPos - myGoal);
        sf::Vector2f upfieldDir = normalize(eGoal - bPos);

        float wAway = forceUpfield ? 0.25f : 0.5f;
        float wUp = forceUpfield ? 0.75f : 0.5f;
        sf::Vector2f baseDir = normalize(awayDir * wAway + upfieldDir * wUp);
        float baseAngle = std::atan2(baseDir.y, baseDir.x);

        // [MỚI] Quét nhiều góc hơn để có nhiều lựa chọn phá bóng
        float angleOffsets[] = { 0.f, 12.f, -12.f, 25.f, -25.f,
                                 40.f, -40.f, 55.f, -55.f, 70.f, -70.f };
        float baseScore = forceUpfield ? 600.f : 350.f;

        for (float aDeg : angleOffsets) {
            float aRad = baseAngle + aDeg * PI / 180.f;
            sf::Vector2f dir = { std::cos(aRad), std::sin(aRad) };

            // KHÔNG phá bóng về gôn nhà
            if (dot(dir, normalize(myGoal - bPos)) > 0.15f) continue;

            // [MỚI] Bóng sau lưng: chỉ cho phép phá theo hướng lên sân địch
            if (isBallBehindPlayer(pPos, bPos, myTeam, 0.0f)) {
                // Bóng sau lưng: hướng phá phải hướng về sân địch (dot > 0.3)
                if (dot(dir, upfieldDir) < 0.3f) continue;
            }

            sf::Vector2f hitPt = optimalHitPoint(bPos, bPos + dir * 130.f);
            if (!isPathClear(pPos, hitPt, sim, idx, PP_RADIUS)) continue;

            // [MỚI] Đường phá bóng có qua góc cột-địa không
            if (!isGoalJunctionClear(pPos, hitPt)) continue;

            sf::Vector2f sDir = normalize(hitPt - pPos);
            float distToBall = dist(pPos, bPos);
            float proxBonus = 2500.f / (distToBall + 1.f);
            float attackBonus = dot(dir, upfieldDir) * 1000.f;
            float anglePenalty = std::abs(aDeg) * 2.5f;

            float score = baseScore + proxBonus + attackBonus - anglePenalty;
            float power = forceUpfield ? MAX_SHOOT_POWER * AI_POWER_BOOST
                : MAX_SHOOT_POWER;

            out.push_back({ idx, sDir * power, true, score, true, false });
        }
    }

    // ──────────────────────────────────────────────────────────────────
    //  THỦ MÔN TẤN CÔNG (chỉ khi 100% ghi bàn)
    //
    //  [FIX QUAN TRỌNG] Clone không copy shotsFired_ nên mọi goal
    //  trong sim đều bị xử lý như Foul (shotsFired_<=1 branch) →
    //  addGoalTeam1/2 không được gọi → score không đổi.
    //  Fix: sau khi clone, đặt shotsFired_ = 999 để bỏ qua nánh foul.
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::addKeeperAttack(const GameState* sim,
        std::vector<AIShot>& out,
        int idx, sf::Vector2f pPos,
        sf::Vector2f bPos, sf::Vector2f eGoal,
        Team myTeam) {
        // ─────────────────────────────────────────────────────────────
        // GIỚI HẠN KÍ: Thủ môn chỉ tấn công khi bóng gần gôn địch
        // và nằm trong tầm vươn tới của thủ môn (không băng ra quá xa)
        // ─────────────────────────────────────────────────────────────
        // Kẻ hoảng cách tối đa thủ môn được phép chạy tới để sút
        // Rút từ 280 xuống 170px: thủ môn chỉ phi ra khi bóng ở rất gần
        static constexpr float KEEPER_MAX_RUSH = 170.f;

        // Thủ môn đến vị trí sút (hit-point): phải trong tầm
        sf::Vector2f hitPtApprox = optimalHitPoint(bPos, eGoal);
        if (dist(pPos, hitPtApprox) > KEEPER_MAX_RUSH) return;  // Quá xa, không băng lên

        if (!isPathClear(bPos, eGoal, sim, idx)) return;

        float viewAngle = ballGoalViewAngle(bPos, eGoal);
        // Góc nhìn phải rộng ≥ 0.30 rad (~17°) – chặt hơn 0.22 cũ
        if (viewAngle < 0.30f) return;

        // Bóng sau lưng thủ môn → không tấn công (quá nguy hiểm)
        if (isBallBehindPlayer(pPos, bPos, myTeam, 0.0f)) return;

        sf::Vector2f myGoal = getMyGoal(myTeam);
        sf::Vector2f toMyGoal = normalize(myGoal - bPos);

        for (float off = -GOAL_H * 0.70f; off <= GOAL_H * 0.70f; off += 6.f) {
            sf::Vector2f target = { eGoal.x, eGoal.y + off };

            // Chỉ target trong vùng an toàn (tránh cột)
            float targetY = target.y;
            if (targetY < GOAL_SAFE_TOP || targetY > GOAL_SAFE_BOT) continue;

            sf::Vector2f hitPt = optimalHitPoint(bPos, target);

            if (!isPathClear(pPos, hitPt, sim, idx, PP_RADIUS)) continue;
            if (!isPathClear(bPos, target, sim, idx, BP_RADIUS)) continue;

            sf::Vector2f dirToHit = normalize(hitPt - pPos);
            sf::Vector2f dirToGoal = normalize(target - bPos);
            float alignment = dot(dirToHit, dirToGoal);
            // Thủ môn cần alignment rất cao (0.70) vì không có cơ hội sửa sai
            if (alignment < 0.70f) continue;
            if (dot(dirToHit, toMyGoal) > 0.20f) continue;
            if (dot(dirToGoal, toMyGoal) > 0.08f) continue;

            float power = calcOptimalPower(dist(bPos, eGoal), alignment);

            // Clone sim: confirm 100% ghi bàn
            // [FIX] Đặt shotsFired_ = 999 để clone không bị kích hoạt nánh foul
            auto testSim = std::unique_ptr<GameState>(sim->clone());
            testSim->recordShot();  testSim->recordShot();  // đảm bảo shotsFired_ >= 2
            // Làm cho shotsFired_ đủ lớn
            for (int bump = 0; bump < 50; ++bump) testSim->recordShot();

            testSim->getPieces()[idx]->setVelocity(dirToHit * power);
            runPhysics(testSim.get(), 3000);  // 3000 steps là chắc chắn

            bool scored = false;
            bool ownGoal = false;
            if (testSim->getPhase() == GamePhase::GoalScored) {
                bool s1 = (myTeam == Team::Team1 &&
                    testSim->getScore1() > sim->getScore1());
                bool s2 = (myTeam == Team::Team2 &&
                    testSim->getScore2() > sim->getScore2());
                scored = s1 || s2;
                ownGoal = !scored;
            }

            // Chỉ thêm shot nếu simulation xác nhận 100% ghi bàn
            if (scored && !ownGoal) {
                float score = 7000.f + alignment * 3500.f + viewAngle * 800.f;
                out.push_back({ idx, dirToHit * power,
                                true, score, false, true });
            }
        }
    }

    // ──────────────────────────────────────────────────────────────────
    //  SINH NƯỚC ĐI: KICKOFF
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::addKickoffShot(const GameState* sim,
        std::vector<AIShot>& out,
        Team myTeam) {
        sf::Vector2f bPos = sim->getBall().getPosition();

        int bestIdx = -1;
        float minD = 1e9f;
        for (int i = 0; i < (int)sim->getPieces().size(); ++i) {
            if (sim->getPieces()[i]->getTeam() != myTeam) continue;
            float d = dist(sim->getPieces()[i]->getPosition(), bPos);
            if (d < minD) { minD = d; bestIdx = i; }
        }
        if (bestIdx == -1) return;

        sf::Vector2f pPos = sim->getPieces()[bestIdx]->getPosition();
        sf::Vector2f eGoal = getEnemyGoal(myTeam);
        float sign = (myTeam == Team::Team1) ? 1.f : -1.f;
        float cy = FIELD_HEIGHT / 2.f + 35.f;

        sf::Vector2f targets[] = {
            { FIELD_WIDTH / 2.f + sign * 90.f,  cy + 130.f },
            { FIELD_WIDTH / 2.f + sign * 90.f,  cy - 130.f },
            { FIELD_WIDTH / 2.f + sign * 160.f, cy },
            { FIELD_WIDTH / 2.f + sign * 60.f,  cy + 80.f  },
            { FIELD_WIDTH / 2.f + sign * 60.f,  cy - 80.f  },
        };

        for (auto& target : targets) {
            sf::Vector2f hitPt = optimalHitPoint(bPos, target);
            sf::Vector2f dBall = normalize(target - bPos);
            sf::Vector2f dEGoal = normalize(eGoal - bPos);
            // Không sút thẳng vào gôn địch (alignment > 0.88 = angle < ~28°)
            if (dot(dBall, dEGoal) > 0.88f) continue;
            if (!isPathClear(pPos, hitPt, sim, bestIdx, PP_RADIUS)) continue;

            sf::Vector2f dir = normalize(hitPt - pPos);
            float score = 700.f - dist(target, eGoal) * 0.01f;
            out.push_back({ bestIdx, dir * (MAX_SHOOT_POWER * 0.72f),
                            true, score, false, false });
        }

        if (out.empty()) {
            sf::Vector2f target = { FIELD_WIDTH / 2.f + sign * 70.f, cy };
            sf::Vector2f hitPt = optimalHitPoint(bPos, target);
            sf::Vector2f dir = normalize(hitPt - pPos);
            out.push_back({ bestIdx, dir * (MAX_SHOOT_POWER * 0.60f),
                            true, 150.f, false, false });
        }
    }

    // ──────────────────────────────────────────────────────────────────
    //  SINH TẤT CẢ NƯỚC ĐI (cải tiến: dùng selectBestPlayers)
    //
    //  Thay vì duyệt tuần tự từng cầu thủ, giờ xếp hạng trước và sinh
    //  shots theo thứ tự ưu tiên: cầu thủ tốt nhất → sinh nhiều hơn,
    //  cầu thủ kém hơn → giới hạn số lượng shots để tiết kiệm thời gian.
    // ──────────────────────────────────────────────────────────────────
    std::vector<AIShot> AIPlayer::generateShots(const GameState* sim,
        Team myTeam,
        const SituationContext& ctx) {
        std::vector<AIShot> shots;

        if (ctx.isKickoff) {
            addKickoffShot(sim, shots, myTeam);
            return shots;
        }

        sf::Vector2f bPos = ctx.ballPos;
        sf::Vector2f eGoal = ctx.enemyGoalPos;
        sf::Vector2f mGoal = ctx.myGoalPos;
        int gkIdx = getGoalkeeperIdx(sim, myTeam);

        bool mustDefend = (ctx.danger == DangerLevel::Critical ||
            ctx.danger == DangerLevel::Danger);
        bool attackMode = !mustDefend;

        // Xếp hạng cầu thủ: tốt nhất lên đầu
        auto ranked = selectBestPlayers(sim, myTeam, bPos, eGoal, attackMode);

        for (int rank = 0; rank < (int)ranked.size(); ++rank) {
            int i = ranked[rank];
            sf::Vector2f pPos = sim->getPieces()[i]->getPosition();
            bool isKeeper = (i == gkIdx);

            // ══════════════════════════════════════════════════════
            // [MỚI] KIỂM TRA VÙNG CỘT DỌC GÔN
            //
            // Nếu cầu thủ đang đứng gần góc không gian giữa
            // tường sân và không gian mở của gôn (cột dọc), rất dễ
            // bị kẹt khi di chuyển sang nỡi khác.
            // → Chạy clone vịt để xác nhận có thể di chuyển tự do.
            // → Nếu bị kẹt → bỏ qua cầu thủ này, chọn người khác.
            // ══════════════════════════════════════════════════════
            if (isNearGoalPostJunction(pPos)) {
                // Clone nhanh: thử di chuyển về hướng bóng
                auto testSim = std::unique_ptr<GameState>(sim->clone());
                sf::Vector2f intendedDir = normalize(bPos - pPos);
                // Dung vận tốc vừa phải (không cần record shot bờ vì
                // không có shot nào được thêm trong 30 bước này)
                testSim->getPieces()[i]->setVelocity(intendedDir * 280.f);
                runPhysics(testSim.get(), 30);

                sf::Vector2f endPos = testSim->getPieces()[i]->getPosition();
                sf::Vector2f actualMove = endPos - pPos;
                float movedDist = dist(pPos, endPos);

                // Nếu cầu thủ bị kẹt (dọi chuyển < 15px) HOẶC
                // bị bật ngược ngà hoàn toàn (alignment âm)
                bool stuck = movedDist < 15.f;
                bool deflected = (movedDist > 5.f &&
                    dot(normalize(actualMove), intendedDir) < -0.2f);

                if (stuck || deflected) {
                    // Cầu thủ này bị kẹt → bỏ qua, dùng cầu thủ khác
                    continue;
                }
            }

            // Số lượng shots tối đa mỗi cầu thủ:
            // Cầu thủ tốt nhất (rank 0) → không giới hạn
            // Cầu thủ kém hơn → giảm dần (nhưng vẫn cho mọi cầu thủ có cơ hội)
            // Điều này giúp tập trung tính toán vào cầu thủ có lợi thế nhất
            size_t shotsBefore = shots.size();

            if (isKeeper) {
                // ══════════════════════════════════════════════════════
                // QUY TẮc THỦ MÔN - chỉ 2 trường hợp được di chuyển:
                //
                //  A. PHÒNG THỦ: chỉ khi mustDefend = true
                //     (ball trong vùng Danger: ≤ DANGER_RADIUS*1.5 = 330px hoặc Critical)
                //     Khi này phi lên là HOÀN TOÀN HỢP LÝ vì bóng đang đe dọa gôn nhà
                //
                //  B. TẤN CÔNG: chỉ khi clone xác nhận 100% góc ghi bàn
                //     và bóng đang ở phía sân địch
                //
                //  Mọi trường hợp khác: THỦ MÔN ĐỨNG YÊN!
                // ══════════════════════════════════════════════════════

                // A. PHÒNG THỦ: chỉ khi tình huống thực sự nguy cấp
                if (mustDefend) {
                    addDefensiveClears(sim, shots, i, pPos, bPos, mGoal,
                        myTeam, true /* force upfield */);
                }

                // B. TẤN CÔNG: không cần phòng thủ + bóng gần sân địch hơn sân nhà
                // (inside addKeeperAttack: yêu cầu clone 100%, KEEPER_MAX_RUSH 170px)
                if (!mustDefend) {
                    float distToEGoal = dist(bPos, eGoal);
                    float distToMGoal = dist(bPos, mGoal);
                    // Chỉ tấn công khi bóng gần sân địch hơn sân nhà
                    if (distToEGoal < distToMGoal) {
                        addKeeperAttack(sim, shots, i, pPos, bPos, eGoal, myTeam);
                    }
                }
            }
            else {
                if (mustDefend) {
                    addDefensiveClears(sim, shots, i, pPos, bPos, mGoal, myTeam, true);
                    // Vẫn cho phép sút thẳng ngay cả khi phòng thủ (nếu có cơ hội vàng)
                    addDirectGoalShots(sim, shots, i, pPos, bPos, eGoal, myTeam);
                }
                else {
                    addDirectGoalShots(sim, shots, i, pPos, bPos, eGoal, myTeam);
                    addBankShots(sim, shots, i, pPos, bPos, eGoal, myTeam);
                    addDefensiveClears(sim, shots, i, pPos, bPos, mGoal, myTeam, false);
                }
            }

            // Boost heuristicScore của cầu thủ tốt hơn để chắc chắn lên đầu
            // sau khi sort Best-First ở computeShot()
            float rankBonus = (float)(ranked.size() - rank) * 300.f;
            for (size_t s = shotsBefore; s < shots.size(); ++s)
                shots[s].heuristicScore += rankBonus;
        }
        return shots;
    }

    // ──────────────────────────────────────────────────────────────────
    //  VẬT LÝ: CHẠY MÔ PHỎNG (tăng tốc với bước lớn hơn)
    // ──────────────────────────────────────────────────────────────────
    void AIPlayer::runPhysics(GameState* sim, int steps) const {
        // [MỚI] Sử dụng bước thời gian lớn hơn (1/30s thay vì 1/60s)
        // để simulation chạy nhanh gấp đôi, đủ dùng cho AI
        constexpr float DT = 1.f / 60.f;
        for (int i = 0; i < steps; ++i) {
            if (sim->isEverythingStopped()) break;
            sim->update(DT);
            if (sim->getPhase() == GamePhase::GoalScored) break;
        }
        if (sim->isEverythingStopped())
            sim->resolveGoalCollisions();
    }

    // ──────────────────────────────────────────────────────────────────
    //  ĐÁNH GIÁ TRẠNG THÁI CUỐI PHA BÓNG
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::evaluateEndState(const GameState* sim, Team myTeam) const {
        if (sim->getPhase() == GamePhase::GoalScored) {
            bool iScored = (myTeam == Team::Team1 && sim->getScore1() > state_->getScore1())
                || (myTeam == Team::Team2 && sim->getScore2() > state_->getScore2());
            return iScored ? 1e8f : OWN_GOAL_PENALTY;
        }

        sf::Vector2f bPos = sim->getBall().getPosition();
        sf::Vector2f mGoal = getMyGoal(myTeam);
        sf::Vector2f eGoal = getEnemyGoal(myTeam);

        float score = 0.f;

        // Tiến gần gôn địch
        float distToEGoal = dist(bPos, eGoal);
        score += (FIELD_WIDTH - distToEGoal) * 80.f;

        // Góc nhìn gôn địch
        float eViewAngle = ballGoalViewAngle(bPos, eGoal);
        score += eViewAngle * 500.f;

        // [MỚI] Thưởng thêm khi bóng rất gần gôn địch
        if (distToEGoal < 200.f)
            score += (200.f - distToEGoal) * 100.f;

        // RULE 2: Phạt bóng nguy hiểm trước gôn nhà
        float mViewAngle = ballGoalViewAngle(bPos, mGoal);
        float dToMGoal = dist(bPos, mGoal);
        float dangerScore = dangerToOwnGoal(bPos, myTeam);

        if (dToMGoal < DANGER_RADIUS && mViewAngle > DANGER_ANGLE) {
            float proximity = 1.f - dToMGoal / DANGER_RADIUS;
            float angleFactor = mViewAngle / PI;
            score += END_DANGER_PENALTY * proximity * angleFactor;
        }
        if (dangerScore > 0.45f)
            score -= dangerScore * 40000.f;

        // RULE 3: Thủ môn không rời xa gôn
        int gkIdx = getGoalkeeperIdx(sim, myTeam);
        if (gkIdx != -1) {
            float gkDist = dist(sim->getPieces()[gkIdx]->getPosition(), mGoal);
            if (gkDist > 130.f)
                score += (gkDist - 130.f) * KEEPER_WANDER_PENALTY;
        }

        score -= scoreCornerPenalty(bPos);
        return score;
    }

    // ──────────────────────────────────────────────────────────────────
    //  QUICK HEURISTIC
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::quickHeuristic(const GameState* sim, const AIShot& shot,
        Team myTeam) const {
        if (!shot.valid) return -1e12f;
        sf::Vector2f bPos = sim->getBall().getPosition();
        sf::Vector2f mGoal = getMyGoal(myTeam);
        // Loại nhanh nếu velocity hướng về gôn nhà
        if (dot(normalize(shot.velocity), normalize(mGoal - bPos)) > 0.45f)
            return -1e9f;
        return shot.heuristicScore;
    }

    // ──────────────────────────────────────────────────────────────────
    //  BỘ LỌC AN TOÀN (SAFETY CHECK)
    //
    //  [FIX BUG CỐT LÕI] shotsFired_ trong clone = 0, nên mọi goal
    //  đều bị tính là Foul (nánh shotsFired_<=1) → score không thay đổi
    //  → AI không biết đó là own-goal.
    //  Fix: gọi recordShot() 100 lần trước khi chạy sim để bỏ qua nánh foul.
    // ──────────────────────────────────────────────────────────────────
    bool AIPlayer::safetyCheck(const AIShot& shot,
        const GameState* baseState,
        Team myTeam,
        bool& outScoredGoal) const {
        outScoredGoal = false;
        auto sim = std::unique_ptr<GameState>(baseState->clone());

        // [FIX QUAN TRỌNG] Đặt shotsFired_ đủ lớn để clone
        // không bị kích hoạt nánh foul (shotsFired_<=1)
        for (int i = 0; i < 100; ++i) sim->recordShot();

        sim->getPieces()[shot.pieceIndex]->setVelocity(shot.velocity);
        runPhysics(sim.get(), 2000);  // 2000 steps (~33s) – chạy đủ để phát hiện mọi chain

        if (sim->getPhase() == GamePhase::GoalScored) {
            bool s1 = (myTeam == Team::Team1 &&
                sim->getScore1() > baseState->getScore1());
            bool s2 = (myTeam == Team::Team2 &&
                sim->getScore2() > baseState->getScore2());
            if (s1 || s2) { outScoredGoal = true; return true; }
            return false;  // Own-goal → loại bỏ
        }

        // RULE 2: Vị trí bóng cuối không nguy hiểm
        sf::Vector2f finalPos = sim->getBall().getPosition();
        sf::Vector2f mGoal = getMyGoal(myTeam);
        float dToMGoal = dist(finalPos, mGoal);
        float mViewAngle = ballGoalViewAngle(finalPos, mGoal);

        // [THẮT CHẶT] phạt cả tình huống bóng gần gôn nhà hơn
        if (dToMGoal < DANGER_RADIUS * 0.70f && mViewAngle > DANGER_ANGLE * 0.80f)
            return false;

        return true;
    }

    // ──────────────────────────────────────────────────────────────────
    //  ALPHA-BETA MINIMAX (depth 2 thay vì 3 để nhanh hơn)
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::minimax(GameState* sim, int depth, float alpha, float beta,
        Team maxTeam, Team myTeam, int simSteps) {
        if (depth == 0 || sim->getPhase() == GamePhase::GoalScored)
            return evaluateEndState(sim, myTeam);

        SituationContext ctx = analyzeSituation(sim, maxTeam);
        auto shots = generateShots(sim, maxTeam, ctx);

        std::sort(shots.begin(), shots.end(), [&](const AIShot& a, const AIShot& b) {
            return quickHeuristic(sim, a, maxTeam) > quickHeuristic(sim, b, maxTeam);
            });

        // [MỚI] Giới hạn nhánh chặt hơn để AI nhanh hơn
        size_t maxBranch = (depth >= 2) ? 4 : 8;
        if (shots.size() > maxBranch) shots.resize(maxBranch);

        float best = (maxTeam == myTeam) ? -1e15f : 1e15f;
        Team enemy = (maxTeam == Team::Team1) ? Team::Team2 : Team::Team1;

        for (auto& s : shots) {
            auto child = std::unique_ptr<GameState>(sim->clone());
            child->getPieces()[s.pieceIndex]->setVelocity(s.velocity);
            runPhysics(child.get(), simSteps);
            float v = minimax(child.get(), depth - 1, alpha, beta, enemy, myTeam, simSteps);
            if (maxTeam == myTeam) { best = std::max(best, v); alpha = std::max(alpha, v); }
            else { best = std::min(best, v); beta = std::min(beta, v); }
            if (beta <= alpha) break;
        }
        return best;
    }

    // ──────────────────────────────────────────────────────────────────
    //  PHẠT GÓC SÂN
    // ──────────────────────────────────────────────────────────────────
    float AIPlayer::scoreCornerPenalty(sf::Vector2f pos) const {
        float p = 0;
        sf::Vector2f corners[] = {
            { FIELD_MARGIN_X,               FIELD_MARGIN_Y },
            { FIELD_WIDTH - FIELD_MARGIN_X, FIELD_MARGIN_Y },
            { FIELD_MARGIN_X,               FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM },
            { FIELD_WIDTH - FIELD_MARGIN_X, FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM }
        };
        for (auto& c : corners) {
            float d = dist(pos, c);
            if (d < 140.f) p += (140.f - d) * 5.f;
        }
        return p;
    }

    // ──────────────────────────────────────────────────────────────────
    //  XÁC ĐỊNH CHỈ SỐ THỦ MÔN
    // ──────────────────────────────────────────────────────────────────
    int AIPlayer::getGoalkeeperIdx(const GameState* sim, Team team) const {
        int best = -1;
        float maxD = -1e9f;
        for (int i = 0; i < (int)sim->getPieces().size(); ++i) {
            if (sim->getPieces()[i]->getTeam() != team) continue;
            float d = (team == Team::Team1)
                ? (FIELD_WIDTH - sim->getPieces()[i]->getPosition().x)
                : sim->getPieces()[i]->getPosition().x;
            if (d > maxD) { maxD = d; best = i; }
        }
        return best;
    }

    // ──────────────────────────────────────────────────────────────────
    //  FALLBACK AN TOÀN
    // ──────────────────────────────────────────────────────────────────
    AIShot AIPlayer::computeFallbackShot(Team myTeam) const {
        sf::Vector2f bPos = state_->getBall().getPosition();
        sf::Vector2f eGoal = getEnemyGoal(myTeam);

        AIShot best;
        best.valid = false;
        float bestS = -1e18f;

        for (int i = 0; i < (int)state_->getPieces().size(); ++i) {
            if (state_->getPieces()[i]->getTeam() != myTeam) continue;
            sf::Vector2f pPos = state_->getPieces()[i]->getPosition();

            // [MỚI] Fallback: đẩy bóng về phía sân địch, không về gôn mình
            sf::Vector2f upDir = normalize(eGoal - bPos);
            sf::Vector2f hitPt = optimalHitPoint(bPos, bPos + upDir * 120.f);
            sf::Vector2f dir = normalize(hitPt - pPos);

            // Đảm bảo hướng đi không quay về gôn mình
            sf::Vector2f toMG = normalize(getMyGoal(myTeam) - bPos);
            if (dot(dir, toMG) > 0.4f) {
                // Xoay 90° sang bên để tránh
                dir = { -dir.y, dir.x };
            }

            float d = dist(pPos, bPos);
            float s = 1200.f / (d + 1.f);
            if (s > bestS) {
                bestS = s;
                best = { i, dir * MAX_SHOOT_POWER * AI_POWER_BOOST,
                          true, s, true, false };
            }
        }

        if (!best.valid) {
            for (int i = 0; i < (int)state_->getPieces().size(); ++i) {
                if (state_->getPieces()[i]->getTeam() != myTeam) continue;
                sf::Vector2f pPos = state_->getPieces()[i]->getPosition();
                sf::Vector2f dir = normalize(bPos - pPos);
                best = { i, dir * MAX_SHOOT_POWER, true, -9999.f, true, false };
                break;
            }
        }
        return best;
    }

    // ──────────────────────────────────────────────────────────────────
    //  HÀM CHÍNH: TÍNH NƯỚC ĐI (cải tiến tốc độ + chính xác)
    // ──────────────────────────────────────────────────────────────────
    AIShot AIPlayer::computeShot() {
        Team myTeam = state_->getCurrentTurn();
        SituationContext ctx = analyzeSituation(state_, myTeam);

        // 1. Sinh nước đi
        auto candidates = generateShots(state_, myTeam, ctx);

        if (candidates.empty())
            return computeFallbackShot(myTeam);

        // 2. Best-First: sort theo heuristic
        std::sort(candidates.begin(), candidates.end(),
            [&](const AIShot& a, const AIShot& b) {
                return a.heuristicScore > b.heuristicScore;
            });

        // 3. [MỚI] Giới hạn candidates để AI phản hồi nhanh hơn
        int maxSims = std::min((int)candidates.size(), MAX_CANDIDATES);

        AIShot bestShot;
        bestShot.valid = false;
        float bestVal = -1e18f;
        bool foundValid = false;

        // [MỚI] Pass 1: Quét nhanh tìm cú sút ghi bàn ngay
        //  Chỉ dùng safety-check (không minimax) – nếu ghi bàn thì return luôn
        for (int i = 0; i < maxSims; ++i) {
            bool scored = false;
            if (!safetyCheck(candidates[i], state_, myTeam, scored)) continue;
            if (scored) return candidates[i];  // Cú sút ghi bàn → thực hiện ngay!
        }

        // [MỚI] Pass 2: Minimax depth 2 (nhanh hơn depth 3 cũ)
        //  chỉ dùng cho TOP-40 candidates còn lại
        int maxMinimax = std::min(maxSims, 40);
        for (int i = 0; i < maxMinimax; ++i) {
            auto& s = candidates[i];

            // Safety check lần 2 (đã được check pass 1 rồi nhưng cần lấy scored)
            bool scored = false;
            if (!safetyCheck(s, state_, myTeam, scored)) continue;
            // scored đã được bắt ở pass 1, ở đây chỉ lọc own-goal & end-danger

            // Tạo clone sau nước đi
            auto sim = std::unique_ptr<GameState>(state_->clone());
            sim->getPieces()[s.pieceIndex]->setVelocity(s.velocity);
            runPhysics(sim.get(), FAST_SIM_STEPS);

            // [MỚI] Minimax depth 2 thay vì 3 – nhanh hơn x3
            float val = minimax(sim.get(), 2,
                -1e18f, 1e18f,
                (myTeam == Team::Team1) ? Team::Team2 : Team::Team1,
                myTeam,
                FAST_SIM_STEPS);

            val += s.heuristicScore * 50.f;   // Best-First bonus tăng lên 50

            if (s.isKeeperShot) val += 1e7f;

            // [MỚI] Boost mạnh hơn cho defensive clear khi nguy hiểm
            if (s.isDefensive && ctx.danger >= DangerLevel::Danger)
                val += 8000.f;

            // [MỚI] Boost cho cú sút thẳng vào gôn (không phải defensive)
            if (!s.isDefensive && !s.isKeeperShot)
                val += ctx.attackScore * 5000.f;

            if (val > bestVal || !foundValid) {
                bestVal = val;
                bestShot = s;
                foundValid = true;
            }
        }

        if (!foundValid)
            return computeFallbackShot(myTeam);

        return bestShot;
    }

} // namespace SoccerPool