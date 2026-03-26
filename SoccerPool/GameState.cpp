#include "GameState.h"
#include <cmath>

namespace SoccerPool {

GameState::GameState()
    : physics_(field_),
      currentTurn_(Team::Team1),
      score1_(0),
      score2_(0),
      phase_(GamePhase::Menu) {
    ball_ = std::make_shared<Ball>();
    physics_.setBall(ball_);
    physics_.setPieces(&pieces_);
}

void GameState::startNewMatch() {
    score1_ = 0;
    score2_ = 0;
    currentTurn_ = Team::Team1;
    phase_ = GamePhase::Playing;
    pieces_.clear();
    spawnPieces();
    spawnBall();
}

//void GameState::spawnPieces() {
//    int n = config_.pieceCount;
//    if (n < MIN_PIECES) n = MIN_PIECES;
//    if (n > MAX_PIECES) n = MAX_PIECES;
//    Formation form = getFormation(n);
//
//    float margin = PIECE_RADIUS + 15.f;
//    int id = 0;
//    float rowSpacing = 70.f;
//    // Team1: bên trái, các hàng từ gần khung thành ra
//    int rowIndex = 0;
//    for (int rowCount : form) {
//        float team1X = margin + 40.f + rowIndex * rowSpacing;
//        float step = rowCount > 1 ? (FIELD_HEIGHT - 2.f * margin) / (rowCount + 1) : 0.f;
//        for (int i = 0; i < rowCount; ++i) {
//            float y = rowCount > 1 ? margin + step * (i + 1) : FIELD_HEIGHT / 2.f;
//            auto p = std::make_shared<Piece>(id++, Team::Team1);
//            p->setPosition(sf::Vector2f(team1X, y));
//            p->setVelocity(sf::Vector2f(0.f, 0.f));
//            pieces_.push_back(p);
//        }
//        ++rowIndex;
//    }
//
//    // Team2: bên phải
//    rowIndex = 0;
//    for (int rowCount : form) {
//        float team2X = FIELD_WIDTH - margin - 40.f - rowIndex * rowSpacing;
//        float step = rowCount > 1 ? (FIELD_HEIGHT - 2.f * margin) / (rowCount + 1) : 0.f;
//        for (int i = 0; i < rowCount; ++i) {
//            float y = rowCount > 1 ? margin + step * (i + 1) : FIELD_HEIGHT / 2.f;
//            auto p = std::make_shared<Piece>(id++, Team::Team2);
//            p->setPosition(sf::Vector2f(team2X, y));
//            p->setVelocity(sf::Vector2f(0.f, 0.f));
//            pieces_.push_back(p);
//        }
//        ++rowIndex;
//    }
//}


void GameState::spawnPieces() {
    pieces_.clear();
    float margin = PIECE_RADIUS + 15.f;
    float rowSpacing = 70.f;
    int idCounter = 0;

    // --- SINH QUÂN CHO TEAM 1 ---
    Formation form1 = getFormation(team1Formation_);
    int rowIndex1 = 0;
    for (int rowCount : form1) {
        float team1X = margin + 40.f + rowIndex1 * rowSpacing;
        float step = rowCount > 1 ? (FIELD_HEIGHT - 2.f * margin) / (rowCount + 1) : 0.f;
        for (int i = 0; i < rowCount; ++i) {
            float y = rowCount > 1 ? margin + step * (i + 1) : FIELD_HEIGHT / 2.f;
            auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
            p->setPosition(sf::Vector2f(team1X, y));
            p->setVelocity(sf::Vector2f(0.f, 0.f));
            pieces_.push_back(p);
        }
        ++rowIndex1;
    }

    // --- SINH QUÂN CHO TEAM 2 ---
    Formation form2 = getFormation(team2Formation_);
    int rowIndex2 = 0;
    for (int rowCount : form2) {
        // Team 2 sinh từ bên phải sân (FIELD_WIDTH trừ ngược lại)
        float team2X = FIELD_WIDTH - margin - 40.f - rowIndex2 * rowSpacing;
        float step = rowCount > 1 ? (FIELD_HEIGHT - 2.f * margin) / (rowCount + 1) : 0.f;
        for (int i = 0; i < rowCount; ++i) {
            float y = rowCount > 1 ? margin + step * (i + 1) : FIELD_HEIGHT / 2.f;
            auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
            p->setPosition(sf::Vector2f(team2X, y));
            p->setVelocity(sf::Vector2f(0.f, 0.f));
            pieces_.push_back(p);
        }
        ++rowIndex2;
    }
}

void GameState::spawnBall() {
    ball_->setPosition(sf::Vector2f(FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f));
    ball_->setVelocity(sf::Vector2f(0.f, 0.f));
}

void GameState::resetPositionsAfterGoal() {
    pieces_.clear();
    spawnPieces();
    spawnBall();
    switchTurn();
    phase_ = GamePhase::Playing;
}

bool GameState::isEverythingStopped() const {
    if (ball_->isMoving()) return false;
    for (const auto& p : pieces_)
        if (p->isMoving()) return false;
    return true;
}

void GameState::update(float dt) {
    if (phase_ != GamePhase::Playing) return;

    if (isEverythingStopped()) {
        turnTimer_ -= dt;

        // 2. Nếu hết thời gian (về 0)
        if (turnTimer_ <= 0.f) {
            switchTurn(); // Hàm này của bạn sẽ đổi đội và gọi resetTurnTimer()
        }
    }

    physics_.update(dt);


    int goal = physics_.checkGoal();
    if (goal != 0) {
        phase_ = GamePhase::GoalScored;
        if (goal == 1) {
            addGoalTeam2();
            if (onGoal_) onGoal_(Team::Team2);
        } else {
            addGoalTeam1();
            if (onGoal_) onGoal_(Team::Team1);
        }
        int s1 = getScore1(), s2 = getScore2();
        if (s1 >= config_.goalsToWin || s2 >= config_.goalsToWin) {
            phase_ = GamePhase::GameOver;
            if (onGameOver_) onGameOver_(s1 >= config_.goalsToWin ? Team::Team1 : Team::Team2);
        }
    }
}

// Bên trong file GameState.cpp, dán vào cuối file:

GameState* GameState::clone() const {
    // 1. Tạo một trận đấu nháp mới tinh
    GameState* copy = new GameState();

    // 2. Copy các chỉ số cơ bản
    copy->config_ = this->config_;
    copy->currentTurn_ = this->currentTurn_;
    copy->score1_ = this->score1_;
    copy->score2_ = this->score2_;
    copy->phase_ = this->phase_;
    
    // Copy thêm 2 biến formation
    copy->team1Formation_ = this->team1Formation_;
    copy->team2Formation_ = this->team2Formation_;

    // 3. SAO CHÉP SÂU QUẢ BÓNG (Tạo quả bóng mới có cùng vị trí, vận tốc)
    copy->ball_ = std::make_shared<Ball>(*this->ball_);

    // Bắt buộc: Nối lại bóng mới vào Engine vật lý của trận đấu nháp
    copy->physics_.setBall(copy->ball_);

    // 4. SAO CHÉP SÂU TỪNG CẦU THỦ
    copy->pieces_.clear();
    for (const auto& p : this->pieces_) {
        // Tạo cầu thủ mới copy y hệt dữ liệu của cầu thủ cũ
        auto newPiece = std::make_shared<Piece>(*p);
        copy->pieces_.push_back(newPiece);
    }

    // Nối lại danh sách cầu thủ mới vào Engine vật lý
    copy->physics_.setPieces(&copy->pieces_);

    // (Lưu ý: Không copy các callback onGoal hay onGameOver để tránh AI sút thử mà lại hiện thông báo Win game lên màn hình!)

    return copy;
}

} // namespace SoccerPool
