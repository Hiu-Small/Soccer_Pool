 #pragma once

#include "Ball.h"
#include "Piece.h"
#include "Field.h"
#include "PhysicsEngine.h"
#include "Constants.h"
#include <memory>
#include <vector>
#include <functional>

namespace SoccerPool {

    enum class GameMode { PvP, PvAI, AIvsAI };
    enum class AIDifficulty { Easy, Medium, Hard };
    enum class GamePhase { Menu, Setup, Playing, GoalScored, GameOver, PickLineup};

    struct GameConfig {
        int lineUp = 0;
        int goalsToWin = DEFAULT_GOALS_TO_WIN;
        GameMode mode = GameMode::PvP;
        AIDifficulty aiDifficulty = AIDifficulty::Medium;
    };

    class GameState {
    public:
        GameState();

        // --- THÊM DÒNG NÀY ĐỂ AI CÓ THỂ COPY TRẠNG THÁI ---
        GameState* clone() const;

        void setConfig(const GameConfig& config) { config_ = config; }
        const GameConfig& getConfig() const { return config_; }

        void startNewMatch();
        void resetPositionsAfterGoal(); // Đặt lại bóng và quân sau bàn thắng

        // Truy cập
        Ball& getBall() { return *ball_; }
        const Ball& getBall() const { return *ball_; }
        std::vector<std::shared_ptr<Piece>>& getPieces() { return pieces_; }
        const std::vector<std::shared_ptr<Piece>>& getPieces() const { return pieces_; }
        Field& getField() { return field_; }
        PhysicsEngine& getPhysics() { return physics_; }

        Team getCurrentTurn() const { return currentTurn_; }
        void setCurrentTurn(Team t) { currentTurn_ = t; }
        void switchTurn() { currentTurn_ = currentTurn_ == Team::Team1 ? Team::Team2 : Team::Team1; turnTimer_ = TURN_TIME_LIMIT;
        }

        int getScore1() const { return score1_; }
        int getScore2() const { return score2_; }
        void addGoalTeam1() { ++score1_; }
        void addGoalTeam2() { ++score2_; }

        GamePhase getPhase() const { return phase_; }
        void setPhase(GamePhase p) { phase_ = p; }

        bool isEverythingStopped() const;
        void update(float dt);

        // Callback khi có bàn thắng (team ghi bàn)
        void setOnGoal(std::function<void(Team)> f) { onGoal_ = f; }
        void setOnGameOver(std::function<void(Team winner)> f) { onGameOver_ = f; }

        float getTurnTimer() const { return turnTimer_; }
        void resetTurnTimer() { turnTimer_ = TURN_TIME_LIMIT; }

        // Hàm để bắt đầu trận đấu từ Menu
        void selectModeAndStart(GameMode mode) {
            config_.mode = mode;
            startNewMatch();
        }

        void setTeam1Formation(int id) { team1Formation_ = id; }
        void setTeam2Formation(int id) { team2Formation_ = id; }
        //void startNewMatch();

    private:
        void spawnPieces();
        void spawnBall();

        GameConfig config_;
        std::shared_ptr<Ball> ball_;
        std::vector<std::shared_ptr<Piece>> pieces_;
        Field field_;
        PhysicsEngine physics_;

        Team currentTurn_;
        int score1_, score2_;
        GamePhase phase_;

        // --- THÊM 2 BIẾN NÀY ---
        int team1Formation_ = 0; // Mặc định là sơ đồ ID 0 (1-2-2)
        int team2Formation_ = 0;

        std::function<void(Team)> onGoal_;
        std::function<void(Team)> onGameOver_;

        float turnTimer_ = TURN_TIME_LIMIT;
    };

};// namespace SoccerPool
