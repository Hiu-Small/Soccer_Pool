#include "GameState.h"
#include <cmath>
#include <string>

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
    resetTurnTimer();
    pieces_.clear();
    spawnPieces();
    spawnBall();
}

void GameState::spawnPieces() {
    pieces_.clear();
    int idCounter = 0;

    // Các thông số căn chỉnh (Có thể đưa vào Constants.h)
    float START_X = 180.f;     // Vị trí hàng thủ môn (cách biên trái)
    float ROW_SPACING = 90.f;  // Khoảng cách giữa các hàng
    float PLAYABLE_HEIGHT = 380.f; // Chiều cao vùng cỏ thực tế (tránh dính biên)
    float CENTER_Y = FIELD_HEIGHT / 2.f + 35.f; // Tâm dọc sân (khớp với bóng)

    // --- SINH QUÂN CHO TEAM 1 ---
    Formation form1 = getFormation(team1Formation_); // Ví dụ: {1, 2, 2}
    std::string lineUp = "";
	for (int i : form1) lineUp += std::to_string(i) + "-";
    int rowIndex = 0;
    if (lineUp == "1-2-2-") {
        // CHUYỂN SANG VÒNG FOR CÓ CHỈ SỐ j (0 -> n)
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j]; 

            float currentRS = ROW_SPACING;
			if (j == 2) currentRS = 120.f;
            float x = START_X + j * currentRS;

            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y; // Thủ môn ở giữa
                }
                else if (rowCount == 2) {
                    float spread = (j == 1) ? 220.f : 100.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                //else {
                //    // Trường hợp dự phòng cho hàng > 2 người
                //    float spread = 120.f;
                //    float startY = CENTER_Y - ((rowCount - 1) * spread) / 2.f;
                //    y = startY + i * spread;
                //}

                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if(lineUp == "1-1-3-") {
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j];
            float currentRS = ROW_SPACING;
            currentRS = 115.f;
            float x = START_X + j * currentRS;
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 3) {
                    float spread = 100.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
	}
    if (lineUp == "1-2-1-1-") {
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j];
            float currentRS = ROW_SPACING;
            if(j == 2) currentRS = 65.f;
            else if (j == 3) currentRS = 80.f;
            float x = START_X + j * currentRS;
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if(rowCount == 2) {
                    float spread = 220.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
				}
                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp == "1-3-1-") {
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j];
            float currentRS = ROW_SPACING;
            if (j == 1) currentRS = 90.f;
            else if (j == 2) currentRS = 115.f;
            float x = START_X + j * currentRS;
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if(rowCount == 3){
                    float spread = 105.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
		}
    }
    if(lineUp == "0-3-2-" || lineUp == "0-2-3-") {
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j];
            float currentRS = ROW_SPACING;
            if (j == 1) currentRS = 75.f;
            else if (j == 2) currentRS = 110.f;
            float x = START_X + j * currentRS;
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 3) {
                    float spread = 105.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                else if(rowCount == 2){
                    float spread = 90.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
	}
    if (lineUp == "1-1-1-2-") {
        for (int j = 0; j < form1.size(); ++j) {
            int rowCount = form1[j];
            float currentRS = ROW_SPACING;
            if (j == 3) currentRS = 85.f;
            float x = START_X + j * currentRS;
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 2) {
                    float spread = 145.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    //for (int rowCount : form1) {
    //    float x = START_X + rowIndex * ROW_SPACING;

    //    for (int i = 0; i < rowCount; ++i) {
    //        float y;
    //        if (rowCount == 1) {
    //            // Nếu hàng chỉ có 1 người (Thủ môn) -> Cho vào chính giữa
    //            y = CENTER_Y;
    //        }
    //        else {
    //            // Nếu hàng có nhiều người (2 người) -> Chia đều quanh tâm
    //            // Khoảng cách giữa các cầu thủ trong hàng (ví dụ 150px)
    //            float spread = 150.f;
    //            float totalSpread = (rowCount - 1) * spread;
    //            float startY = CENTER_Y - (totalSpread / 2.f);
    //            y = startY + i * spread;
    //        }

    //        auto p = std::make_shared<Piece>(idCounter++, Team::Team1);
    //        p->setPosition({ x, y });
    //        p->setVelocity({ 0.f, 0.f });
    //        pieces_.push_back(p);
    //    }
    //    rowIndex++;
    //}

    // --- SINH QUÂN CHO TEAM 2 (ĐỐI XỨNG) ---
    Formation form2 = getFormation(team2Formation_);
    std::string lineUp2 = "";
    for (int i : form2) lineUp2 += std::to_string(i) + "-";
    rowIndex = 0;
    if (lineUp2 == "1-2-2-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];

            float currentRS = ROW_SPACING;
            if (j == 2) currentRS = 120.f;

            float x = FIELD_WIDTH - (START_X + j * currentRS);

            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 2) {
                    float spread = (j == 1) ? 220.f : 100.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }

                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp2 == "1-1-3-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];
            float currentRS = ROW_SPACING;
            currentRS = 115.f;
            float x = FIELD_WIDTH - (START_X + j * currentRS);
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 3) {
                    float spread = 100.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp2 == "1-2-1-1-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];
            float currentRS = ROW_SPACING;
            if (j == 2) currentRS = 65.f;
            else if (j == 3) currentRS = 80.f;
            float x = FIELD_WIDTH - (START_X + j * currentRS);
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 2) {
                    float spread = 220.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp2 == "1-3-1-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];
            float currentRS = ROW_SPACING;
            if (j == 1) currentRS = 90.f;
            else if (j == 2) currentRS = 115.f;
            float x = FIELD_WIDTH - (START_X + j * currentRS);
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 3) {
                    float spread = 105.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp2 == "0-3-2-" || lineUp2 == "0-2-3-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];
            float currentRS = ROW_SPACING;
            if (j == 1) currentRS = 75.f;
            else if (j == 2) currentRS = 110.f;
            float x = FIELD_WIDTH - (START_X + j * currentRS);
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 3) {
                    float spread = 105.f;
                    y = CENTER_Y + (i - 1.f) * spread; // i=0 -> -100, i=1 -> 0, i=2 -> +100
                }
                else if (rowCount == 2) {
                    float spread = 90.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    if (lineUp2 == "1-1-1-2-") {
        for (int j = 0; j < form2.size(); ++j) {
            int rowCount = form2[j];
            float currentRS = ROW_SPACING;
            if (j == 3) currentRS = 85.f;
            float x = FIELD_WIDTH - (START_X + j * currentRS);
            for (int i = 0; i < rowCount; ++i) {
                float y;
                if (rowCount == 1) {
                    y = CENTER_Y;
                }
                else if (rowCount == 2) {
                    float spread = 145.f;
                    y = CENTER_Y + (i - 0.5f) * spread;
                }
                auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
                p->setPosition({ x, y });
                p->setVelocity({ 0.f, 0.f });
                pieces_.push_back(p);
            }
        }
    }
    //for (int rowCount : form2) {
    //    float x = FIELD_WIDTH - START_X - rowIndex * ROW_SPACING; // Đối xứng từ phải sang

    //    for (int i = 0; i < rowCount; ++i) {
    //        float y;
    //        if (rowCount == 1) y = CENTER_Y;
    //        else {
    //            float spread = 150.f;
    //            float totalSpread = (rowCount - 1) * spread;
    //            float startY = CENTER_Y - (totalSpread / 2.f);
    //            y = startY + i * spread;
    //        }

    //        auto p = std::make_shared<Piece>(idCounter++, Team::Team2);
    //        p->setPosition({ x, y });
    //        p->setVelocity({ 0.f, 0.f });
    //        pieces_.push_back(p);
    //    }
    //    rowIndex++;
    //}
}

void GameState::spawnBall() {
    ball_->setPosition(sf::Vector2f(FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 35.f));
    ball_->setVelocity(sf::Vector2f(0.f, 0.f));
}

void GameState::resetPositionsAfterGoal() {
    pieces_.clear();
    spawnPieces();
    spawnBall();
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
            switchTurn(Team::Team1);
            if (onGoal_) onGoal_(Team::Team2);
        } else {
            addGoalTeam1();
            switchTurn(Team::Team2);
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

    copy->turnTimer_ = this->turnTimer_;  // THÊM DÒNG NÀY

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
