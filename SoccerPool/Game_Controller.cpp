#include "Game_Controller.h"
#include "Constants.h"
#include <cmath>
#include <optional>
#include <memory>
#include <algorithm>

namespace SoccerPool {

const float Game_Controller::AI_DELAY_SEC = 0.8f;

Game_Controller::Game_Controller(GameState& state, Game_Render& view)
    : state_(state), view_(view) {
    state_.setOnGoal([this](Team t) { onGoalScored(t); });
    state_.setOnGameOver([this](Team w) { onGameOver(w); });
}

void Game_Controller::setViewSize(unsigned width, unsigned height) {
    viewWidth_ = width;
    viewHeight_ = height;
    view_.setViewSize(width, height);
}

int Game_Controller::getPieceIndexAt(sf::Vector2f worldPos) const {
    const auto& pieces = state_.getPieces();
    Team turn = state_.getCurrentTurn();
    for (size_t i = 0; i < pieces.size(); ++i) {
        if (pieces[i]->getTeam() != turn) continue;
        sf::Vector2f p = pieces[i]->getPosition();
        float dx = worldPos.x - p.x, dy = worldPos.y - p.y;
        if (dx * dx + dy * dy <= PIECE_RADIUS * PIECE_RADIUS)
            return static_cast<int>(i);
    }
    return -1;
}

bool Game_Controller::isCurrentPlayerHuman() const {
    GameMode mode = state_.getConfig().mode;
    Team turn = state_.getCurrentTurn();
    if (mode == GameMode::PvP) return true;
    if (mode == GameMode::PvAI) return turn == Team::Team1;
    return false; // AI vs AI
}

void Game_Controller::startGameWithMode(int menuChoice) {
    GameConfig cfg;
    cfg.pieceCount = 3;
    cfg.goalsToWin = DEFAULT_GOALS_TO_WIN;
    if (menuChoice == 1) { cfg.mode = GameMode::PvP; }
    else if (menuChoice == 2) { cfg.mode = GameMode::PvAI; cfg.aiDifficulty = AIDifficulty::Easy; }
    //else if (menuChoice == 3) { cfg.mode = GameMode::PvAI; cfg.aiDifficulty = AIDifficulty::Medium; }
    //else if (menuChoice == 4) { cfg.mode = GameMode::PvAI; cfg.aiDifficulty = AIDifficulty::Hard; }
    else if (menuChoice == 5) { cfg.mode = GameMode::AIvsAI; cfg.aiDifficulty = AIDifficulty::Medium; }
    else return;
    state_.setConfig(cfg);
    state_.startNewMatch();
    state_.setPhase(GamePhase::Playing);
    aiPlayer1_ = std::make_unique<AIPlayer>();
    aiPlayer2_ = std::make_unique<AIPlayer>();
    aiPlayer1_->setState(&state_);
    aiPlayer2_->setState(&state_);
    if (cfg.mode == GameMode::AIvsAI)
        aiThinkTimer_ = AI_DELAY_SEC;
}

void Game_Controller::onGoalScored(Team scoringTeam) {
    (void)scoringTeam;
    selectedPieceIndex_ = -1;
    dragging_ = false;
}

void Game_Controller::onGameOver(Team winner) {
    (void)winner;
    selectedPieceIndex_ = -1;
    dragging_ = false;
}

void Game_Controller::endTurn() {
    state_.switchTurn();
    selectedPieceIndex_ = -1;
    dragging_ = false;
    if (state_.getPhase() == GamePhase::Playing && state_.getConfig().mode == GameMode::AIvsAI)
        aiThinkTimer_ = AI_DELAY_SEC;
    else if (state_.getPhase() == GamePhase::Playing && !isCurrentPlayerHuman())
        aiThinkTimer_ = AI_DELAY_SEC;
}

void Game_Controller::tryShoot(sf::Vector2f velocity) {
    if (selectedPieceIndex_ < 0) return;
    auto& pieces = state_.getPieces();
    if (selectedPieceIndex_ >= static_cast<int>(pieces.size())) return;
    pieces[selectedPieceIndex_]->setVelocity(velocity);
    endTurn();
}

void Game_Controller::triggerAITurn() {
    Team turn = state_.getCurrentTurn();
    AIPlayer* ai = (turn == Team::Team1) ? aiPlayer1_.get() : aiPlayer2_.get();
    if (!ai) return;
    AIShot shot = ai->computeShot();
    if (!shot.valid || shot.pieceIndex < 0) return;
    auto& pieces = state_.getPieces();
    if (shot.pieceIndex >= static_cast<int>(pieces.size())) return;
    if (pieces[shot.pieceIndex]->getTeam() != turn) return;
    pieces[shot.pieceIndex]->setVelocity(shot.velocity);
    endTurn();
}


void Game_Controller::handlePickLineup(sf::Vector2f mPos) {
    // 1. CHỌN ĐỘI HÌNH (Chỉ để hiện viền, chưa chốt)
    int startIdx = view_.getCurrentPage() * 2;
    for (int i = 0; i < 2; ++i) {
        sf::FloatRect cardRect({ 230.f + i * 345.f, 130.f }, { 200.f, 220.f });
        if (cardRect.contains(mPos)) {
            int selectedId = view_.getLineupId(startIdx + i);
            view_.setSelectedLineupId(selectedId); // Render sẽ tự vẽ viền vàng
            return; // Click chọn xong thì thoát ra chờ bấm NEXT
        }
    }

    // 2. LOGIC BẤM NÚT "NEXT" HOẶC "START" (Vị trí 500, 450 trong Render của bạn)
    // Giả sử nút NEXT có kích thước khoảng 200x60
    sf::FloatRect nextBtnRect({ 420.f, 425.f }, { 160.f, 50.f });
    if (nextBtnRect.contains(mPos)) {
        int currentSelected = view_.getSelectedLineupId(); // Hiếu cần thêm hàm getter này ở Render

        // Nếu chưa chọn gì mà đã bấm NEXT thì không làm gì cả
        if (currentSelected == -1) return;

        if (view_.getPickingTeam() == 1) {
            // Chốt đội hình Team 1
            state_.setTeam1Formation(currentSelected);

            if (state_.getConfig().mode == GameMode::PvAI) {
                state_.setTeam2Formation(5); // Máy tự chọn 5
                state_.startNewMatch();      // VÀO CHƠI LUÔN
            }
            else {
                // Chuyển sang cho Team 2 chọn
                view_.setPickingTeam(2);
                view_.setSelectedLineupId(-1); // Reset viền để Team 2 chọn mới
            }
        }
        else {
            // Đã là Team 2 chọn xong
            state_.setTeam2Formation(currentSelected);
            state_.startNewMatch(); // VÀO CHƠI
        }
        return;
    }

    // 3. Logic bấm mũi tên chuyển trang (Vị trí cũ)
    if (sf::FloatRect({ 40.f, 240.f }, { 85.f, 50.f }).contains(mPos)) view_.prevPage();
    if (sf::FloatRect({ 880.f, 240.f }, { 85.f, 50.f }).contains(mPos)) view_.nextPage();

    // 4. Nút Back (Return) góc trên trái
    if (sf::FloatRect({ 25.f, 25.f }, { 50.f, 50.f }).contains(mPos)) {
        state_.setPhase(GamePhase::Setup);
        view_.setSelectedLineupId(-1); // Reset khi quay lại
    }
}


void Game_Controller::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (event.is<sf::Event::Closed>()) return;

    // 1. XỬ LÝ CLICK CHUỘT
    if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
        if (me->button == sf::Mouse::Button::Left) {

            // Lấy tọa độ chuột (dùng View mặc định cho Menu)
            //sf::Vector2f mPos(static_cast<float>(me->position.x), static_cast<float>(me->position.y));

            sf::Vector2i pixelPos = { me->position.x, me->position.y };
            sf::Vector2f mPos = window.mapPixelToCoords(pixelPos);

            // --- TRẠNG THÁI MENU CHÍNH ---
            if (state_.getPhase() == GamePhase::Menu) {
                // Vùng nút PLAY (Vị trí {500, 250}, kích thước khoảng {260, 60})
                sf::FloatRect playRect({ 500.f - 120.f, 250.f - 5.f },{ 240.f, 100.f });
                if (playRect.contains(mPos)) {
                    state_.setPhase(GamePhase::Setup); // Chuyển sang chọn Mode
                    return;
                }

                // Vùng nút QUIT (Góc trên phải {950, 50})
                sf::FloatRect quitRect({ 950.f - 25.f, 50.f - 25.f },{ 50.f, 50.f});
                if (quitRect.contains(mPos)) {
                    // Lệnh thoát game (tùy vào cách bạn quản lý window)
                }
            }

            // --- TRẠNG THÁI CHỌN GAME MODE (Setup) ---
            else if (state_.getPhase() == GamePhase::Setup) {
                // Nút Player vs Player 
                if (sf::FloatRect({ 490.f - 130.f, 200.f - 3.f }, { 280.f, 95.f }).contains(mPos)) {
                    //startGameWithMode(1); // Gọi hàm khởi tạo PvP của bạn
                    GameConfig cfg; cfg.mode = GameMode::PvP; state_.setConfig(cfg);
                    view_.setPickingTeam(1);
                    state_.setPhase(GamePhase::PickLineup);
                    return;
                }
                // Nút Player vs AI 
                else if (sf::FloatRect({ 490.f - 130.f, 200.f + 110 }, { 280.f, 85.f }).contains(mPos)) {
                    //startGameWithMode(2); // Giả sử 3 là PvAI Medium
                    GameConfig cfg; cfg.mode = GameMode::PvAI; state_.setConfig(cfg);
                    view_.setPickingTeam(1);
                    state_.setPhase(GamePhase::PickLineup);
                    return;
                }
                // Nút Player vs AI 
                else if (sf::FloatRect({ 490.f - 130.f, 200.f + 110 + 110 }, { 280.f, 85.f }).contains(mPos)) {
                    //startGameWithMode(5); // Giả sử 5 là AIvAI Medium
                    GameConfig cfg; cfg.mode = GameMode::AIvsAI; state_.setConfig(cfg);
                    view_.setPickingTeam(1);
                    state_.setPhase(GamePhase::PickLineup);
                    return;
                }
                // Nút Back (Góc trên trái {50, 50})
                else if (sf::FloatRect({ 50.f - 25.f, 50.f - 25.f }, { 50.f, 50.f }).contains(mPos)) {
                    state_.setPhase(GamePhase::Menu);
                }
            }

            // --- LOGIC CHỌN ĐỘI HÌNH ---
            else if (state_.getPhase() == GamePhase::PickLineup) {
                handlePickLineup(mPos);
                //int startIdx = view_.getCurrentPage() * 2;
                //for (int i = 0; i < 2; ++i) {
                //    // Vùng click của 2 cái ảnh đội hình (khớp với vị trí vẽ 150 + i * 300)
                //    sf::FloatRect cardRect({ 150.f + i * 300.f, 200.f }, { 250.f, 200.f });
                //    if (cardRect.contains(mPos)) {
                //        int selectedId = view_.getLineupId(startIdx + i);

                //        if (view_.getPickingTeam() == 1) {
                //            state_.setTeam1Formation(selectedId);

                //            // Nếu là PvAI, team 2 tự chọn và vào game luôn
                //            if (state_.getConfig().mode == GameMode::PvAI) {
                //                state_.setTeam2Formation(5); // Máy chọn sơ đồ 5
                //                state_.startNewMatch(); // VÀO CHƠI
                //            }
                //            else {
                //                view_.setPickingTeam(2); // Chuyển sang chọn cho Team 2
                //            }
                //        }
                //        else {
                //            state_.setTeam2Formation(selectedId);
                //            state_.startNewMatch(); // VÀO CHƠI
                //        }
                //        return;
                //    }
                //}

                // Nút Back (Góc trên trái {50, 50})
                /*if (sf::FloatRect({ 50.f - 25.f, 50.f - 25.f }, { 50.f, 50.f }).contains(mPos)) {
                    state_.setPhase(GamePhase::Setup);
                }*/

                // Click Mũi tên chuyển trang
                // if (arrowRightSpriteBounds.contains(mPos)) view_.nextPage();
            }

            else if (state_.getPhase() == GamePhase::Playing ) {
                if (sf::FloatRect({ 50.f - 25.f, 50.f - 25.f }, { 50.f, 50.f }).contains(mPos)) {
                    state_.setPhase(GamePhase::Setup);
                }
            }
        }
    }

    //if (state_.getPhase() == GamePhase::Menu) {
    //    if (event.is<sf::Event::KeyPressed>()) {
    //        const auto& ke = event.getIf<sf::Event::KeyPressed>();
    //        if (ke->scancode == sf::Keyboard::Scan::Num1) startGameWithMode(1);
    //        else if (ke->scancode == sf::Keyboard::Scan::Num2) startGameWithMode(2);
    //        else if (ke->scancode == sf::Keyboard::Scan::Num3) startGameWithMode(3);
    //        else if (ke->scancode == sf::Keyboard::Scan::Num4) startGameWithMode(4);
    //        else if (ke->scancode == sf::Keyboard::Scan::Num5) startGameWithMode(5);
    //    }
    //    return;
    //}

    if (state_.getPhase() == GamePhase::GameOver) {
        if (event.is<sf::Event::KeyPressed>()) {
            const auto& ke = event.getIf<sf::Event::KeyPressed>();
            if (ke->scancode == sf::Keyboard::Scan::Space) {
                state_.setPhase(GamePhase::Menu);
            }
        }
        return;
    }

    if (state_.getPhase() == GamePhase::GoalScored) {
        return;
    }

    if (state_.getPhase() != GamePhase::Playing) return;
    if (!isCurrentPlayerHuman()) return;
    if (!state_.isEverythingStopped()) return;

    if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
        if (me->button == sf::Mouse::Button::Left) {
            lastMouseWorld_ = view_.screenToWorld(sf::Vector2f(static_cast<float>(me->position.x), static_cast<float>(me->position.y)));
            selectedPieceIndex_ = getPieceIndexAt(lastMouseWorld_);
            if (selectedPieceIndex_ >= 0) {
                dragging_ = true;
                dragStart_ = state_.getPieces()[selectedPieceIndex_]->getPosition();
                dragCurrent_ = dragStart_;
            }
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        const auto& me = event.getIf<sf::Event::MouseButtonReleased>();
        if (me->button == sf::Mouse::Button::Left && dragging_ && selectedPieceIndex_ >= 0) {
            sf::Vector2f end = view_.screenToWorld(sf::Vector2f(static_cast<float>(me->position.x), static_cast<float>(me->position.y)));
            sf::Vector2f delta = dragStart_ - end;
            float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (len > 2.f) {
                //float power = std::min(MAX_SHOOT_POWER, len * DRAG_POWER_FACTOR);
                // 1. Chốt chiều dài kéo không được vượt quá vòng tròn
                float currentLength = std::min(len, MAX_VISUAL_DRAG);

                // 2. CÔNG THỨC CỦA BẠN: Tính lực theo tỷ lệ phần trăm
                float power = (currentLength * MAX_SHOOT_POWER) / MAX_VISUAL_DRAG;
                delta.x /= len;
                delta.y /= len;
                tryShoot(sf::Vector2f(delta.x * power, delta.y * power));
            }
            dragging_ = false;
        }
    } else if (event.is<sf::Event::MouseMoved>()) {
        const auto& me = event.getIf<sf::Event::MouseMoved>();
        lastMouseWorld_ = view_.screenToWorld(sf::Vector2f(static_cast<float>(me->position.x), static_cast<float>(me->position.y)));
        if (dragging_)
            dragCurrent_ = lastMouseWorld_;
    }
}


void Game_Controller::update(float dt) {
    if (state_.getPhase() == GamePhase::GoalScored) {
        state_.resetPositionsAfterGoal();
        return;
    }
    state_.update(dt);

    sf::Vector2f ballPos = state_.getBall().getPosition();
    //printf("Ball Position: X = %.2f, Y = %.2f\n", ballPos.x, ballPos.y);

    if (state_.getPhase() == GamePhase::Playing && !isCurrentPlayerHuman()) {
        aiThinkTimer_ -= dt;
        if (aiThinkTimer_ <= 0.f && state_.isEverythingStopped()) {
            triggerAITurn();
        }
    }
}

void Game_Controller::draw(sf::RenderWindow& window) {
    if (state_.getPhase() == GamePhase::Playing && dragging_ && selectedPieceIndex_ >= 0) {
        sf::Vector2f delta = dragStart_ - dragCurrent_;
        float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (len > 2.f)
            view_.setDragState(dragStart_, dragCurrent_, state_.getPieces()[selectedPieceIndex_]->getPosition(), delta);
    }
    view_.draw(window);
}

} // namespace SoccerPool
