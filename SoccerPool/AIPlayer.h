#pragma once

#include "GameState.h"
#include "Constants.h"
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <memory>

namespace SoccerPool {

    struct AIShot {
        int pieceIndex = -1;
        sf::Vector2f velocity{ 0.f, 0.f };
        bool valid = false;
    };

    class AIPlayer {
    public:
        AIPlayer();
        void setState(GameState* state) { state_ = state; }

        // Hàm duy nhất được gọi để lấy nước đi
        AIShot computeShot();

    private:
        GameState* state_ = nullptr;

        // 1. ENGINE GIẢ LẬP VÀ ĐÁNH GIÁ (Simulation & Heuristic)
        float simulateAndEvaluate(const AIShot& shot, Team myTeam);
        float evaluateState(GameState* simState, Team myTeam);

        // 2. HỆ THỐNG RAYCAST & TÌM ĐƯỜNG
        bool isPathClear(sf::Vector2f start, sf::Vector2f end, int ignorePieceIdx, const GameState* state) const;
        std::vector<AIShot> generateCandidateShots(Team myTeam);

        // 3. TÍNH TOÁN CÁC LOẠI CÚ SÚT CHUYÊN SÂU
        void addDirectShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power);
        void addBankShots(std::vector<AIShot>& candidates, int pieceIdx, sf::Vector2f piecePos, sf::Vector2f targetPos, float power);

        float distance(sf::Vector2f a, sf::Vector2f b) const;


        // Tính góc sút vào khung thành
        float calculateShotAngle(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos) const;

        // Kiểm tra xem có thể ghi bàn trực tiếp không
        bool canScoreDirectly(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos, float& requiredPower) const;

        // Tính điểm cho một cú sút dựa trên góc và khoảng cách
        float evaluateShotQuality(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f goalPos, sf::Vector2f shotDir, float power) const;

        // Tìm điểm va chạm tối ưu trên bóng
        sf::Vector2f findOptimalHitPoint(sf::Vector2f piecePos, sf::Vector2f ballPos, sf::Vector2f targetPos) const;

        bool isBetween(sf::Vector2f point, sf::Vector2f a, sf::Vector2f b) const;

    };

} // namespace SoccerPool