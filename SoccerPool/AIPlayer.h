#pragma once
#include "GameState.h"
#include "Constants.h"
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <memory>

namespace SoccerPool {

    struct AIShot {
        int          pieceIndex = -1;
        sf::Vector2f velocity{ 0.f, 0.f };
        bool         valid = false;
        float        heuristicScore = 0.f;
    };

    enum class SituationMode { Attacking, Defending, Balanced, Panic };

    class AIPlayer {
    public:
        AIPlayer();
        void setState(GameState* state) { state_ = state; }
        AIShot computeShot();

    private:
        GameState* state_ = nullptr;

        bool hasRealScoringChance(Team myTeam);
        bool isKickoff() const; // Kiểm tra lượt giao bóng đầu tiên

        // =========================================================
        // LOCKDOWN & PANIC MODE (PHÒNG THỦ KHẨN CẤP)
        // =========================================================
        struct ThreatInfo {
            int   pieceIdx = -1;
            float threatScore = 0.f;
            float distToBall = 0.f;
            float distToMyGoal = 0.f;
            float shotAngleToGoal = 0.f;
            bool  hasLineOfSight = false;
        };
        std::vector<ThreatInfo> analyzeThreatMap(Team myTeam) const;
        void genBlockingShots(std::vector<AIShot>& out, Team myTeam, const std::vector<ThreatInfo>& threats);
        void genGoalCoverShots(std::vector<AIShot>& out, Team myTeam, const std::vector<ThreatInfo>& threats);
        void genNeutralZoneShots(std::vector<AIShot>& out, Team myTeam);
        void genPanicClearance(std::vector<AIShot>& out, Team myTeam); // Phá bóng an toàn
        void genDefensiveShots_all(std::vector<AIShot>& out, Team myTeam);
        std::vector<AIShot> generateLockdownShots(Team myTeam);
        float evaluateLockdownState(const GameState* sim, Team myTeam) const;

        // =========================================================
        // TẤN CÔNG (TÍNH TOÁN QUỸ ĐẠO BẰNG LƯỢNG GIÁC)
        // =========================================================
        std::vector<AIShot> generateAllCandidates(Team myTeam);
        void genDirectGoalShots(std::vector<AIShot>& out, int idx, sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f enemyGoal, Team myTeam);
        void genBankShots(std::vector<AIShot>& out, int idx, sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f enemyGoal, Team myTeam);
        void genChainShots(std::vector<AIShot>& out, int idx, sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f enemyGoal, Team myTeam);
        void genDefensiveShots(std::vector<AIShot>& out, int idx, sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f myGoal, Team myTeam);
        void genPressureShots(std::vector<AIShot>& out, int idx, sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f enemyGoal, Team myTeam);

        // ALGORITHMS (BEST-FIRST SEARCH & MINIMAX ALPHA-BETA)
        float               quickHeuristic(const AIShot& shot, Team myTeam) const;
        std::vector<AIShot> selectTopCandidates(std::vector<AIShot>& all, Team myTeam, int topN = 25);
        float               minimaxAlphaBeta(GameState* sim, int depth, float alpha, float beta, Team maximizingTeam, Team myTeam);
        void                runPhysics(GameState* sim, int maxSteps = 180) const;
        std::vector<AIShot> generateEnemyResponses(GameState* sim, Team enemyTeam);

        // HÀM ĐÁNH GIÁ (HEURISTIC EVALUATION)
        float evaluateState(const GameState* sim, Team myTeam) const;
        float scoreGoalThreat(const GameState* sim, Team myTeam) const;
        float scoreDefenseDanger(const GameState* sim, Team myTeam) const;
        float scoreBallControl(const GameState* sim, Team myTeam) const;
        float scoreFieldPosition(const GameState* sim, Team myTeam) const;
        float scorePieceFormation(const GameState* sim, Team myTeam) const;
        float scoreCornerPenalty(sf::Vector2f pos) const; // Tránh đá vào 4 góc sân
        float scoreGoalExposure(const GameState* sim, Team myTeam) const;   // Phát hiện gôn bị hở sau cú sút
        float scorePostShotDefense(const GameState* sim, Team myTeam) const; // Đánh giá đội hình phòng thủ còn lại


        SituationMode analyzeSituation(Team myTeam) const;
        float         getBallDangerScore(Team myTeam) const;
        float         getAttackOpportunity(Team myTeam) const;

        // TOÁN HỌC & HÌNH HỌC (TRIGONOMETRY & VECTOR MATH)
        float        dist(sf::Vector2f a, sf::Vector2f b) const;
        float        distSq(sf::Vector2f a, sf::Vector2f b) const;
        sf::Vector2f normalize(sf::Vector2f v) const;
        float        dot(sf::Vector2f a, sf::Vector2f b) const;
        bool         isPathClear(sf::Vector2f start, sf::Vector2f end, const GameState* state, int ignorePieceIdx = -1, float checkR = -1.f) const;        float        shotAngle(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos) const;
        sf::Vector2f optimalHitPoint(sf::Vector2f ballPos, sf::Vector2f goalTarget) const;
        float        goalViewAngle(sf::Vector2f ballPos, sf::Vector2f goalCenter, float halfGoalH) const;
        sf::Vector2f getEnemyGoal(Team myTeam) const;
        sf::Vector2f getMyGoal(Team myTeam) const;

        int          getGoalkeeperIdx(const GameState* sim, Team team) const;
        float        scoreGoalkeeperPosition(const GameState* sim, Team myTeam) const;
        float        calculateShotPower(float dPieceToBall, float dBallToTarget, float targetVelocity = 150.f) const;


        struct Weights { float attack = 1.f, defense = 1.f, control = 0.5f; };
        Weights computeWeights(Team myTeam, SituationMode mode) const;
    };

} // namespace SoccerPool