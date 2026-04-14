#pragma once
#include "GameState.h"
#include "Constants.h"
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <memory>
#include <functional>

namespace SoccerPool {

    // ============================================================
    //  Cấu trúc nước đi AI
    // ============================================================
    struct AIShot {
        int          pieceIndex = -1;
        sf::Vector2f velocity{ 0.f, 0.f };
        bool         valid = false;
        float        heuristicScore = 0.f;
        bool         isDefensive = false;   // Nước đi phòng thủ / phá bóng
        bool         isKeeperShot = false;   // Thủ môn tấn công (đã xác nhận 100%)
    };

    // ============================================================
    //  Mức độ nguy hiểm của bóng trước gôn nhà
    // ============================================================
    enum class DangerLevel {
        Safe,       // Bóng ở nửa sân địch hoặc xa gôn
        Neutral,    // Nửa sân nhà nhưng chưa trong vùng nguy hiểm
        Danger,     // Bóng trước gôn nhà, trong vòng cấm
        Critical    // Bóng cực kỳ gần gôn nhà, có góc bắn thẳng
    };

    // ============================================================
    //  Bối cảnh tình huống
    // ============================================================
    struct SituationContext {
        DangerLevel  danger = DangerLevel::Safe;
        bool         isKickoff = false;
        float        dangerScore = 0.f;
        float        attackScore = 0.f;
        bool         ballOnMySide = false;
        sf::Vector2f ballPos;
        sf::Vector2f myGoalPos;
        sf::Vector2f enemyGoalPos;
    };

    // ============================================================
    //  Lớp AI chính
    // ============================================================
    class AIPlayer {
    public:
        AIPlayer();
        void setState(GameState* state) { state_ = state; }
        AIShot computeShot();

    private:
        GameState* state_ = nullptr;

        // ── Phân tích tình huống ────────────────────────────────
        SituationContext analyzeSituation(const GameState* sim, Team myTeam) const;
        float dangerToOwnGoal(sf::Vector2f ballPos, Team myTeam) const;
        float ballGoalViewAngle(sf::Vector2f ballPos, sf::Vector2f goalCenter) const;

        // ── [MỚI] Phát hiện bóng sau lưng cầu thủ ──────────────
        // thresh: ngưỡng dot product (< thresh → bóng phía sau)
        bool isBallBehindPlayer(sf::Vector2f pPos, sf::Vector2f bPos,
            Team myTeam, float thresh = -0.1f) const;

        // ── [MỚI] Tính lực sút tối ưu theo khoảng cách ─────────
        float calcOptimalPower(float distToGoal, float alignment) const;

        // ── [MỚI] Đánh giá mức độ thuận lợi của cầu thủ ────────
        float scorePlayerAdvantage(int idx, sf::Vector2f pPos,
            sf::Vector2f bPos, sf::Vector2f eGoal,
            const GameState* sim,
            Team myTeam, bool isKeeper) const;

        // ── [MỚI] Chọn và xếp hạng cầu thủ theo lợi thế ────────
        // Trả về danh sách pieceIndex đã sort: idx[0] = tốt nhất
        std::vector<int> selectBestPlayers(const GameState* sim,
            Team myTeam,
            sf::Vector2f bPos,
            sf::Vector2f eGoal,
            bool attackMode) const;

        // ── [MỚI] Kiểm tra vùng nguy hiểm góc cột dọc gôn ───────
        // Vùng giao giua tường sân và không gian mở của gôn
        // - isNearGoalPostJunction: cầu thủ đã đứng trong vùng ngưỡng
        // - isGoalJunctionClear: đường di chuyển pPos→hitPt có qua vùng đó không
        bool isNearGoalPostJunction(sf::Vector2f pPos) const;
        bool isGoalJunctionClear(sf::Vector2f pPos, sf::Vector2f hitPt,
            float checkR = -1.f) const;

        // ── Sinh nước đi ────────────────────────────────────────
        std::vector<AIShot> generateShots(const GameState* sim, Team myTeam,
            const SituationContext& ctx);
        void addDirectGoalShots(const GameState* sim, std::vector<AIShot>& out,
            int idx, sf::Vector2f pPos, sf::Vector2f bPos,
            sf::Vector2f eGoal, Team myTeam);
        void addBankShots(const GameState* sim, std::vector<AIShot>& out,
            int idx, sf::Vector2f pPos, sf::Vector2f bPos,
            sf::Vector2f eGoal, Team myTeam);
        void addDefensiveClears(const GameState* sim, std::vector<AIShot>& out,
            int idx, sf::Vector2f pPos, sf::Vector2f bPos,
            sf::Vector2f myGoal, Team myTeam, bool forceUpfield);
        void addKeeperAttack(const GameState* sim, std::vector<AIShot>& out,
            int idx, sf::Vector2f pPos, sf::Vector2f bPos,
            sf::Vector2f eGoal, Team myTeam);
        void addKickoffShot(const GameState* sim, std::vector<AIShot>& out,
            Team myTeam);

        // ── Bộ lọc an toàn ─────────────────────────────────────
        bool safetyCheck(const AIShot& shot, const GameState* baseState,
            Team myTeam, bool& outScoredGoal) const;

        // ── Đánh giá ────────────────────────────────────────────
        float evaluateEndState(const GameState* sim, Team myTeam) const;
        float minimax(GameState* sim, int depth, float alpha, float beta,
            Team maxTeam, Team myTeam, int simSteps);
        float quickHeuristic(const GameState* sim, const AIShot& shot,
            Team myTeam) const;

        // ── Physics helper ──────────────────────────────────────
        void  runPhysics(GameState* sim, int steps) const;

        // ── Tìm kiếm / Heuristic ────────────────────────────────
        int   bfsObstacleCount(sf::Vector2f from, sf::Vector2f to,
            const GameState* sim, Team myTeam) const;
        float hillClimbBestAngle(sf::Vector2f bPos, sf::Vector2f eGoal,
            const GameState* sim, int pieceIdx,
            float startAngle, float range, int iters) const;

        // ── Tiện ích ────────────────────────────────────────────
        int   getGoalkeeperIdx(const GameState* sim, Team team) const;
        AIShot computeFallbackShot(Team myTeam) const;     // [MỚI] member function
        bool  isPathClear(sf::Vector2f start, sf::Vector2f end,
            const GameState* state, int ignoreIdx = -1,
            float checkR = -1.f) const;
        sf::Vector2f optimalHitPoint(sf::Vector2f ballPos, sf::Vector2f target) const;
        sf::Vector2f getGoalPos(Team team) const;
        sf::Vector2f getMyGoal(Team team)    const;
        sf::Vector2f getEnemyGoal(Team team) const;
        float scoreCornerPenalty(sf::Vector2f pos) const;

        float distSq(sf::Vector2f a, sf::Vector2f b) const {
            float dx = b.x - a.x, dy = b.y - a.y; return dx * dx + dy * dy;
        }
        float dist(sf::Vector2f a, sf::Vector2f b) const {
            return std::sqrt(distSq(a, b));
        }
        sf::Vector2f normalize(sf::Vector2f v) const {
            float len = std::sqrt(v.x * v.x + v.y * v.y);
            return len < 1e-6f ? sf::Vector2f(0.f, 0.f) : v / len;
        }
        float dot(sf::Vector2f a, sf::Vector2f b) const {
            return a.x * b.x + a.y * b.y;
        }
        sf::Vector2f rotateVec(sf::Vector2f v, float theta) const {
            return { v.x * std::cos(theta) - v.y * std::sin(theta),
                     v.x * std::sin(theta) + v.y * std::cos(theta) };
        }
    };

} // namespace SoccerPool