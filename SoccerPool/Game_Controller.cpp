#include "Game_Controller.h"
#include "Constants.h"
#include <cmath>
#include <optional>
#include <memory>
#include <algorithm>
#include <iostream>

namespace SoccerPool {

    const float Game_Controller::AI_DELAY_SEC = 2.0f;

    Game_Controller::Game_Controller(GameState& state, Game_Render& view)
        : state_(state), view_(view) {
        state_.setOnGoal([this](Team t) { onGoalScored(t); });
        state_.setOnGameOver([this](Team w) { onGameOver(w); });

        handCursor_ = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
        defaultCursor_ = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
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
        cfg.lineUp = 0;
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

        view_.startGoalAnimation();
    }

    void Game_Controller::onGameOver(Team winner) {
        (void)winner;
        selectedPieceIndex_ = -1;
        dragging_ = false;

        view_.resetGameOverAnimation();
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

        state_.recordShot();

        endTurn();
    }

    void Game_Controller::triggerAITurn() {
        Team turn = state_.getCurrentTurn();
        AIPlayer* ai = (turn == Team::Team1) ? aiPlayer1_.get() : aiPlayer2_.get();
        if (!ai) {
            aiThinkTimer_ = AI_DELAY_SEC; // Reset timer để thử lại
            return;
        }

        // Kích hoạt suy nghĩ AI ở tiểu trình phụ để Game Loop/UI không bị đóng băng
        aiThinking_ = true;
        aiFuture_ = std::async(std::launch::async, [ai]() { return ai->computeShot(); });
    }

    void Game_Controller::handlePickTeam(sf::Vector2f mPos) {
        // 1. CHỌN ĐỘI
        int startIdx = view_.getCurrentTeamPage() * 4;
        for (int i = 0; i < 4 && (startIdx + i) < view_.getTeamCount(); ++i) {
            int row = i / 2;
            int col = i % 2;
            sf::FloatRect cardRect({ 350.f + col * 300.f - 100.f, 160.f + row * 160.f - 60.f }, { 200.f, 150.f });

            if (cardRect.contains(mPos)) {
                // ---> NẾU ĐANG LÀ P2 VÀ BẤM VÀO ĐỘI P1 ĐÃ CHỌN -> BỎ QUA KHÔNG LÀM GÌ CẢ
                if (view_.getPickingTeamFor() == 2 && view_.getTeamAbbrById(startIdx + i) == state_.getTeamAbbr(Team::Team1)) {
                    return;
                }

                view_.setSelectedTeamId(startIdx + i);
                return;
            }
        }

        // 2. BẤM NEXT HOẶC START
        sf::FloatRect nextBtnRect({ 420.f, 425.f }, { 160.f, 50.f });
        if (nextBtnRect.contains(mPos)) {
            int currentSelected = view_.getSelectedTeamId();
            if (currentSelected == -1) return; // Chưa chọn không cho qua

            if (view_.getPickingTeamFor() == 1) {
                state_.setTeamAbbr(Team::Team1, view_.getTeamAbbrById(currentSelected));

                if (state_.getConfig().mode == GameMode::PvAI) {
                    // Tự động random 1 đội cho AI
                    state_.setTeamAbbr(Team::Team2, view_.getTeamAbbrById(rand() % 20));

                    // MỘT MÌNH CHƠI THÌ NHẢY THẲNG SANG CHỌN ĐỘI HÌNH
                    state_.setPhase(GamePhase::PickLineup);
                    view_.setPickingTeam(1);
                    view_.setCurrentPage(0);

                    // ---> THÊM Ở ĐÂY: Xóa viền vàng chọn đội để game sau không bị lưu lại
                    view_.setSelectedTeamId(-1);
                }
                else {
                    // CHỜ NGƯỜI THỨ 2 CHỌN ĐỘI
                    view_.setPickingTeamFor(2);
                    view_.setSelectedTeamId(-1);
                    view_.setCurrentTeamPage(0);
                }
            }
            else {
                // Người thứ 2 đã chọn xong
                state_.setTeamAbbr(Team::Team2, view_.getTeamAbbrById(currentSelected));
                state_.setPhase(GamePhase::PickLineup); // Chuyển sang chọn Đội hình
                view_.setPickingTeam(1);
                view_.setCurrentPage(0);

                // ---> THÊM Ở ĐÂY: Xóa viền vàng chọn đội để game sau không bị lưu lại
                view_.setSelectedTeamId(-1);

                // Reset luôn trang chọn đội về đầu tiên cho game sau
                view_.setCurrentTeamPage(0);
            }
            return;
        }

        // 3. MŨI TÊN CHUYỂN TRANG
        if (view_.getCurrentTeamPage() > 0 && sf::FloatRect({ 40.f, 230.f }, { 80.f, 40.f }).contains(mPos)) {
            view_.prevTeamPage();
            view_.setSelectedTeamId(-1);
        }
        if ((view_.getCurrentTeamPage() + 1) * 4 < view_.getTeamCount() && sf::FloatRect({ 885.f, 230.f }, { 80.f, 40.f }).contains(mPos)) {
            view_.nextTeamPage();
            view_.setSelectedTeamId(-1);
        }

        // 4. NÚT BACK
        if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mPos)) {
            if (view_.getPickingTeamFor() == 2) {
                // Nếu P2 đang chọn, back lại cho P1 chọn lại
                view_.setPickingTeamFor(1);
                view_.setSelectedTeamId(-1);
                view_.setCurrentTeamPage(0);
            }
            else {
                // Nếu P1 đang chọn, back hẳn ra Menu Setup Mode
                state_.setPhase(GamePhase::Setup);
                view_.setSelectedTeamId(-1);
                view_.setCurrentTeamPage(0);
            }
        }
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
                    state_.setTeam2Formation(0); // Máy tự chọn 5

                    // --- THÊM ĐOẠN NÀY ---
                    aiPlayer2_ = std::make_unique<AIPlayer>(); // Khởi tạo não cho Máy
                    aiPlayer2_->setState(&state_);
                    aiThinkTimer_ = AI_DELAY_SEC; // Kích hoạt đồng hồ chờ
                    // ---------------------

                    state_.startNewMatch();      // VÀO CHƠI LUÔN

                    // ---> GỌI HÀM LOAD LOGO Ở ĐÂY <---
                    view_.loadTeamLogos();

                    // ---> THÊM DÒNG NÀY ĐỂ THỔI CÒI <---
                    view_.playWhistleSound();
                }
                else {
                    // Chuyển sang cho Team 2 chọn
                    view_.setPickingTeam(2);
                    view_.setSelectedLineupId(-1); // Reset viền để Team 2 chọn mới

                    view_.setCurrentPage(0);
                }
            }
            else {
                // Đã là Team 2 chọn xong
                state_.setTeam2Formation(currentSelected);
                state_.startNewMatch(); // VÀO CHƠI

                // ---> GỌI HÀM LOAD LOGO Ở ĐÂY <---
                view_.loadTeamLogos();

                // ---> THÊM DÒNG NÀY ĐỂ THỔI CÒI <---
                view_.playWhistleSound();
                aiThinkTimer_ = AI_DELAY_SEC;
            }
            return;
        }

        // 3. Logic bấm mũi tên chuyển trang (Vị trí cũ)
        /*if (sf::FloatRect({ 40.f, 210.f }, { 80.f, 40.f }).contains(mPos)) view_.prevPage();
        if (sf::FloatRect({ 885.f, 210.f }, { 80.f, 40.f }).contains(mPos)) view_.nextPage();*/
        if (view_.getCurrentPage() > 0) {
            if (sf::FloatRect({ 40.f, 210.f }, { 80.f, 40.f }).contains(mPos)) {
                view_.prevPage();
                view_.setSelectedLineupId(-1); // Nên reset viền khi sang trang
            }
        }

        int totalLineups = view_.getLineupCount();
        // Cho phép bấm nếu trang tiếp theo vẫn còn ít nhất 1 đội hình
        if ((view_.getCurrentPage() + 1) * 2 < totalLineups) {
            if (sf::FloatRect({ 885.f, 210.f }, { 80.f, 40.f }).contains(mPos)) {
                view_.nextPage();
                view_.setSelectedLineupId(-1); // Reset viền
            }
        }

        // 4. Nút Back (Return) góc trên trái
        if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mPos)) {
            if (view_.getPickingTeam() == 2) {
                // Đang ở Pick Lineup 2 -> Về Pick Lineup 1 (Trang 0)
                view_.setPickingTeam(1);
                view_.setSelectedLineupId(-1);
                view_.setCurrentPage(0); // FIX: Ép về trang 0
            }
            else {
                // Đang ở Pick Lineup 1 -> Về Pick Team 2 (Trang 0)
                state_.setPhase(GamePhase::PickTeam);
                view_.setPickingTeamFor(2); // FIX: Set state về cho P2 chọn đội
                view_.setSelectedLineupId(-1);
                view_.setCurrentTeamPage(0); // FIX: Ép về trang 0 của màn hình chọn đội
                view_.resetSelectionState(); // Reset luôn viền vàng bên Pick Team
            }
        }
    }

    //handle event

    //void Game_Controller::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    //    if (event.is<sf::Event::Closed>()) return;
    //
    //    sf::Vector2f mPos;
    //    if (event.is<sf::Event::MouseButtonPressed>() ||
    //        event.is<sf::Event::MouseButtonReleased>() ||
    //        event.is<sf::Event::MouseMoved>())
    //    {
    //        // Lấy vị trí chuột hiện tại (pixel) và chuyển sang thế giới (coords)
    //        sf::Vector2i mousePos;
    //        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) mousePos = { mbp->position.x, mbp->position.y };
    //        else if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>()) mousePos = { mbr->position.x, mbr->position.y };
    //        else if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) mousePos = { mm->position.x, mm->position.y };
    //
    //        mPos = window.mapPixelToCoords(mousePos);
    //    }
    //
    //    // 1. XỬ LÝ CLICK CHUỘT
    //    if (event.is<sf::Event::MouseButtonPressed>()) {
    //        const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
    //        if (me->button == sf::Mouse::Button::Left) {
    //
    //            // Lấy tọa độ chuột (dùng View mặc định cho Menu)
    //            //sf::Vector2f mPos(static_cast<float>(me->position.x), static_cast<float>(me->position.y));
    //
    //            //sf::Vector2i pixelPos = { me->position.x, me->position.y };
    //            //sf::Vector2f mPos = window.mapPixelToCoords(pixelPos);
    //
    //            // --- TRẠNG THÁI MENU CHÍNH ---
    //            if (state_.getPhase() == GamePhase::Menu) {
    //                // Vùng nút PLAY (Vị trí {500, 250}, kích thước khoảng {260, 60})
    //                sf::FloatRect playRect({ 375.f, 210.f },{ 245.f, 80.f });
    //                if (playRect.contains(mPos)) {
    //                    state_.setPhase(GamePhase::Setup); // Chuyển sang chọn Mode
    //                    return;
    //                }
    //
    //                // Vùng nút QUIT (Góc trên phải {950, 50})
    //                sf::FloatRect quitRect({ 950.f - 25.f, 50.f - 25.f },{ 50.f, 50.f});
    //                if (quitRect.contains(mPos)) {
    //                    // Lệnh thoát game (tùy vào cách bạn quản lý window)
    //                }
    //            }
    //
    //            // --- TRẠNG THÁI CHỌN GAME MODE (Setup) ---
    //            else if (state_.getPhase() == GamePhase::Setup) {
    //                // Nút Player vs Player 
    //                if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f }, { 280.f, 70.f }).contains(mPos)) {
    //                    //startGameWithMode(1); // Gọi hàm khởi tạo PvP của bạn
    //                    GameConfig cfg; cfg.mode = GameMode::PvP; state_.setConfig(cfg);
    //                    view_.setPickingTeam(1);
    //                    state_.setPhase(GamePhase::PickLineup);
    //                    return;
    //                }
    //                // Nút Player vs AI 
    //                else if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f + 90.f }, { 280.f, 70.f }).contains(mPos)) {
    //                    //startGameWithMode(2); // Giả sử 3 là PvAI Medium
    //                    GameConfig cfg; cfg.mode = GameMode::PvAI; state_.setConfig(cfg);
    //                    view_.setPickingTeam(1);
    //                    state_.setPhase(GamePhase::PickLineup);
    //                    return;
    //                }
    //                // Nút Player vs AI 
    //                else if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f + 90.f + 90.f }, { 280.f, 70.f }).contains(mPos)) {
    //                    //startGameWithMode(5); // Giả sử 5 là AIvAI Medium
    //                    GameConfig cfg; cfg.mode = GameMode::AIvsAI; state_.setConfig(cfg);
    //                    view_.setPickingTeam(1);
    //                    state_.setPhase(GamePhase::PickLineup);
    //                    return;
    //                }
    //                // Nút Back (Góc trên trái {50, 50})
    //                else if (sf::FloatRect({ 50.f - 20.f, 50.f - 20.f }, { 40.f, 40.f }).contains(mPos)) {
    //                    state_.setPhase(GamePhase::Menu);
    //                }
    //            }
    //
    //            // --- LOGIC CHỌN ĐỘI HÌNH ---
    //            else if (state_.getPhase() == GamePhase::PickLineup) {
    //                handlePickLineup(mPos);
    //            }
    //
    //            else if (state_.getPhase() == GamePhase::Playing ) {
    //                if (sf::FloatRect({ 50.f - 20.f, 50.f - 20.f }, { 40.f, 40.f }).contains(mPos)) {
    //                    state_.setPhase(GamePhase::Setup);
    //                }
    //            }
    //
    //            lastMouseWorld_ = mPos;
    //            selectedPieceIndex_ = getPieceIndexAt(lastMouseWorld_);
    //            if (selectedPieceIndex_ >= 0) {
    //                dragging_ = true;
    //                dragStart_ = state_.getPieces()[selectedPieceIndex_]->getPosition();
    //                dragCurrent_ = dragStart_;
    //            }
    //        }
    //    }
    //
    //    if (state_.getPhase() == GamePhase::GameOver) {
    //        if (event.is<sf::Event::KeyPressed>()) {
    //            const auto& ke = event.getIf<sf::Event::KeyPressed>();
    //            if (ke->scancode == sf::Keyboard::Scan::Space) {
    //                state_.setPhase(GamePhase::Menu);
    //            }
    //        }
    //        return;
    //    }
    //
    //    if (state_.getPhase() == GamePhase::GoalScored) {
    //        return;
    //    }
    //
    //    if (state_.getPhase() != GamePhase::Playing) return;
    //    if (!isCurrentPlayerHuman()) return;
    //    if (!state_.isEverythingStopped()) return;
    //
    //     if (event.is<sf::Event::MouseButtonReleased>()) {
    //        const auto& me = event.getIf<sf::Event::MouseButtonReleased>();
    //        if (me->button == sf::Mouse::Button::Left && dragging_ && selectedPieceIndex_ >= 0) {
    //            //sf::Vector2f end = view_.screenToWorld(sf::Vector2f(static_cast<float>(me->position.x), static_cast<float>(me->position.y)));
    //            sf::Vector2f delta = dragStart_ - mPos;
    //            float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    //            if (len > 2.f) {
    //                //float power = std::min(MAX_SHOOT_POWER, len * DRAG_POWER_FACTOR);
    //                // 1. Chốt chiều dài kéo không được vượt quá vòng tròn
    //                float currentLength = std::min(len, MAX_VISUAL_DRAG);
    //
    //                // 2. CÔNG THỨC CỦA BẠN: Tính lực theo tỷ lệ phần trăm
    //                float power = (currentLength * MAX_SHOOT_POWER) / MAX_VISUAL_DRAG;
    //                delta.x /= len;
    //                delta.y /= len;
    //                tryShoot(sf::Vector2f(delta.x * power, delta.y * power));
    //            }
    //            dragging_ = false;
    //        }
    //    } else if (event.is<sf::Event::MouseMoved>()) {
    //        const auto& me = event.getIf<sf::Event::MouseMoved>();
    //        lastMouseWorld_ = mPos;
    //        if (dragging_)
    //            dragCurrent_ = mPos;
    //    }
    //}

    void Game_Controller::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
        if (event.is<sf::Event::Closed>()) return;

        bool isInteractive = false;

        // 1. QUY ĐỔI TỌA ĐỘ CHUỘT (Dùng chung cho tất cả các Phase)
        sf::Vector2f mPos;
        bool isMouseEvent = false;
        if (event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseButtonReleased>() || event.is<sf::Event::MouseMoved>()) {
            sf::Vector2i mousePos;
            if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) mousePos = { mbp->position.x, mbp->position.y };
            else if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>()) mousePos = { mbr->position.x, mbr->position.y };
            else if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) mousePos = { mm->position.x, mm->position.y };

            mPos = window.mapPixelToCoords(mousePos);
            isMouseEvent = true;
        }

        // ===== CẬP NHẬT CON TRỎ KHI DI CHUYỂN CHUỘT =====
        if (event.is<sf::Event::MouseMoved>()) {
            if (isMouseOverInteractive(mPos))
                window.setMouseCursor(*handCursor_);
            else
                window.setMouseCursor(*defaultCursor_);
        }

        // 2. XỬ LÝ THEO TỪNG PHASE CỤ THỂ
        GamePhase currentPhase = state_.getPhase();

        if (currentPhase == GamePhase::ConfirmQuit) {
            if (isMouseEvent && event.is<sf::Event::MouseButtonPressed>()) {
                const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
                if (me->button == sf::Mouse::Button::Left) {
                    sf::FloatRect yesRect({ 353.f, 273.f }, { 135.f, 47.f });
                    sf::FloatRect noRect({ 515.f, 273.f }, { 135.f, 47.f });

                    if (yesRect.contains(mPos)) {
                        if (state_.getPreviousPhase() == GamePhase::Playing) {
                            state_.setPhase(GamePhase::Menu);
                            view_.resetSelectionState();
                        }
                        else {
                            window.close();
                        }
                        return; // Thoát luôn sau khi xử lý
                    }
                    if (noRect.contains(mPos)) {
                        state_.setPhase(state_.getPreviousPhase());
                        return; // Thoát luôn
                    }
                }
            }
            return; // CHẶN TOÀN BỘ các phase khác khi đang hiện MSB
        }

        // --- PHASE MENU / SETUP / LINEUP / OPTIONS ---
        if (currentPhase == GamePhase::Menu || currentPhase == GamePhase::Setup || currentPhase == GamePhase::PickLineup || currentPhase == GamePhase::Options || currentPhase == GamePhase::PickTeam) {
            // --- CHUYỂN DÒNG NÀY RA NGOÀI ĐỂ BẮT ĐƯỢC SỰ KIỆN KÉO VÀ THẢ CHUỘT ---
            if (currentPhase == GamePhase::Options && isMouseEvent) {
                view_.handleEvent(event, window, mPos);
            }

            if (isMouseEvent && event.is<sf::Event::MouseButtonPressed>()) {
                const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
                if (me->button == sf::Mouse::Button::Left) {
                    // Chỉ xử lý Click chuột ở đây
                    if (currentPhase == GamePhase::Menu) {
                        // Vùng nút PLAY (Vị trí {500, 250}, kích thước khoảng {260, 60})
                        sf::FloatRect playRect({ 375.f, 210.f }, { 250.f, 80.f });
                        if (playRect.contains(mPos)) {
                            state_.setPhase(GamePhase::Setup); // Chuyển sang chọn Mode
                            return;
                        }


                        sf::FloatRect optionsRect({ 375.f, 340.f }, { 250.f, 80.f });
                        if (optionsRect.contains(mPos)) {
                            state_.setPreviousPhase(GamePhase::Menu);
                            state_.setPhase(GamePhase::Options);
                            return;
                        }


                        // Vùng nút QUIT (Góc trên phải {950, 50})
                        sf::FloatRect quitRect({ 950.f - 25.f, 50.f - 25.f }, { 50.f, 50.f });
                        if (quitRect.contains(mPos)) {
                            state_.setPreviousPhase(GamePhase::Menu);
                            state_.setPhase(GamePhase::ConfirmQuit);
                            return;
                        }
                    }
                    else if (currentPhase == GamePhase::Options) {
                        // Nút Back (Góc trên trái {50, 50})
                        if (sf::FloatRect({ 50.f - 20.f, 50.f - 20.f }, { 40.f, 40.f }).contains(mPos)) {
                            state_.setPhase(state_.getPreviousPhase());
                            return;
                        }
                    }
                    else if (currentPhase == GamePhase::Setup) {
                        //Nút Player vs Player 
                        if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f }, { 280.f, 70.f }).contains(mPos)) {
                            //startGameWithMode(1); // Gọi hàm khởi tạo PvP của bạn
                            GameConfig cfg; cfg.mode = GameMode::PvP; state_.setConfig(cfg);
                            view_.setPickingTeam(1);
                            state_.setPhase(GamePhase::PickTeam); // Đổi từ PickLineup sang PickTeam
                            view_.setPickingTeamFor(1);

                            // ---> ĐÃ FIX: Dọn sạch rác (Viền vàng, Số trang) của ván game trước
                            view_.setSelectedTeamId(-1);
                            view_.setCurrentTeamPage(0);
                            view_.setSelectedLineupId(-1);
                            view_.setCurrentPage(0);
                            return;
                        }
                        // Nút Player vs AI 
                        else if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f + 90.f }, { 280.f, 70.f }).contains(mPos)) {
                            //startGameWithMode(2); // Giả sử 3 là PvAI Medium
                            GameConfig cfg; cfg.mode = GameMode::PvAI; state_.setConfig(cfg);
                            view_.setPickingTeam(1);
                            state_.setPhase(GamePhase::PickTeam); // Đổi từ PickLineup sang PickTeam
                            view_.setPickingTeamFor(1);

                            // ---> ĐÃ FIX: Dọn sạch rác
                            view_.setSelectedTeamId(-1);
                            view_.setCurrentTeamPage(0);
                            view_.setSelectedLineupId(-1);
                            view_.setCurrentPage(0);
                            return;
                        }
                        // Nút AI vs AI 
                        else if (sf::FloatRect({ 490.f - 125.f, 200.f - 30.f + 90.f + 90.f }, { 280.f, 70.f }).contains(mPos)) {
                            //startGameWithMode(5); // Giả sử 5 là AIvAI Medium
                            GameConfig cfg; cfg.mode = GameMode::AIvsAI; state_.setConfig(cfg);

                            // --- QUAN TRỌNG: Khởi tạo 2 não AI ở đây ---
                            aiPlayer1_ = std::make_unique<AIPlayer>();
                            aiPlayer1_->setState(&state_);
                            aiPlayer2_ = std::make_unique<AIPlayer>();
                            aiPlayer2_->setState(&state_);

                            view_.setPickingTeam(1);
                            state_.setPhase(GamePhase::PickTeam); // Đổi từ PickLineup sang PickTeam
                            view_.setPickingTeamFor(1);

                            // ---> ĐÃ FIX: Dọn sạch rác
                            view_.setSelectedTeamId(-1);
                            view_.setCurrentTeamPage(0);
                            view_.setSelectedLineupId(-1);
                            view_.setCurrentPage(0);
                            return;
                        }
                        // Nút Back (Góc trên trái {50, 50})
                        else if (sf::FloatRect({ 50.f - 20.f, 50.f - 20.f }, { 40.f, 40.f }).contains(mPos)) {
                            state_.setPhase(GamePhase::Menu);
                        }
                    }
                    //else if (currentPhase == GamePhase::ConfirmQuit) {
                    //    if (isMouseEvent && event.is<sf::Event::MouseButtonPressed>()) {
                    //        const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
                    //        if (me->button == sf::Mouse::Button::Left) {
                    //            sf::FloatRect yesRect({ 353.f, 273.f }, { 135.f, 47.f });
                    //            sf::FloatRect noRect({ 515.f, 273.f }, { 135.f, 47.f });

                    //            if (yesRect.contains(mPos)) {
                    //                // KIỂM TRA XEM TRƯỚC ĐÓ ĐANG Ở ĐÂU
                    //                if (state_.getPreviousPhase() == GamePhase::Playing) {
                    //                    // Nếu đang chơi -> Về Menu Setup
                    //                    state_.setPhase(GamePhase::Menu);
                    //                    view_.resetSelectionState();
                    //                }
                    //                else if (state_.getPreviousPhase() == GamePhase::Menu) {
                    //                    // Nếu đang ở Menu chính -> Thoát hẳn game
                    //                    window.close();
                    //                }
                    //                return;
                    //            }

                    //            if (noRect.contains(mPos)) {
                    //                // Quay lại Phase trước đó
                    //                state_.setPhase(state_.getPreviousPhase());
                    //                return;
                    //            }
                    //        }
                    //    }
                    //    return; // Chặn mọi thao tác khác khi đang hiện bảng
                    //}
                    else if (currentPhase == GamePhase::PickTeam) {
                        handlePickTeam(mPos);
                    }
                    else if (currentPhase == GamePhase::PickLineup) handlePickLineup(mPos);
                }
            }
            return; // Thoát ra vì không phải phase Playing
        }

        // --- PHASE GAME OVER ---
        if (currentPhase == GamePhase::GameOver) {
            if (event.is<sf::Event::KeyPressed>()) {
                if (event.getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scan::Space)
                    state_.setPhase(GamePhase::Menu);
            }
            return;
        }

        // --- PHASE PLAYING (LOGIC SÚT BÓNG) ---
        if (currentPhase != GamePhase::Playing) return;

        if (isMouseEvent && event.is<sf::Event::MouseButtonPressed>()) {
            if (event.getIf<sf::Event::MouseButtonPressed>()->button == sf::Mouse::Button::Left) {
                // Check nút Back (Góc trên trái {50, 50})
                if (sf::FloatRect({ 50.f - 20.f, 50.f - 20.f }, { 40.f, 40.f }).contains(mPos)) {
                    //state_.setPhase(GamePhase::Setup); // Quay lại chọn Mode
                    //view_.resetSelectionState();
                    state_.setPreviousPhase(GamePhase::Playing);
                    state_.setPhase(GamePhase::ConfirmQuit);
                    return; // Thoát luôn để không kéo trúng cầu thủ nằm dưới nút
                }

                if (sf::FloatRect({ 950.f - 20.f, 50.f - 20.f }, { 50.f, 50.f }).contains(mPos)) {
                    state_.setPreviousPhase(GamePhase::Playing);
                    state_.setPhase(GamePhase::Options);
                    return; // Thoát luôn để không kéo trúng cầu thủ nằm dưới nút
                }
            }
        }

        if (!isCurrentPlayerHuman() || !state_.isEverythingStopped()) return;

        if (event.is<sf::Event::MouseButtonPressed>()) {
            const auto& me = event.getIf<sf::Event::MouseButtonPressed>();
            if (me->button == sf::Mouse::Button::Left) {
                selectedPieceIndex_ = getPieceIndexAt(mPos);
                if (selectedPieceIndex_ >= 0) {
                    dragging_ = true;
                    dragStart_ = state_.getPieces()[selectedPieceIndex_]->getPosition();
                    dragCurrent_ = mPos;
                }
            }
        }
        else if (event.is<sf::Event::MouseButtonReleased>()) {
            const auto& me = event.getIf<sf::Event::MouseButtonReleased>();
            if (me->button == sf::Mouse::Button::Left && dragging_ && selectedPieceIndex_ >= 0) {
                sf::Vector2f delta = dragStart_ - mPos;
                float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                if (len > 2.f) {
                    float currentLength = std::min(len, MAX_VISUAL_DRAG);
                    float power = (currentLength * MAX_SHOOT_POWER) / MAX_VISUAL_DRAG;
                    delta /= len;
                    tryShoot(sf::Vector2f(delta.x * power, delta.y * power));
                }
                dragging_ = false;
            }
        }
        else if (event.is<sf::Event::MouseMoved>()) {
            if (dragging_) dragCurrent_ = mPos;
        }

        //if (isInteractive)
        //    window.setMouseCursor(*handCursor_);
        //else
        //    window.setMouseCursor(*defaultCursor_);
    }

    void Game_Controller::update(float dt) {
        if (state_.getPhase() == GamePhase::GoalScored) {
            // 1. Phải gọi update thì Banner chữ GOAL mới trượt ra trượt vào được
            view_.updateGoalAnimation(dt);

            // 2. CHỈ reset bóng và cầu thủ KHI hiệu ứng chữ GOAL đã chạy xong 100%
            if (view_.isGoalAnimationDone()) {
                state_.resetPositionsAfterGoal();
                // --- RESET AI TIMER SAU KHI GHI BÀN ---
                if (!isCurrentPlayerHuman()) {
                    aiThinkTimer_ = AI_DELAY_SEC;
                }
            }
            return;
        }

        // ---> THÊM ĐOẠN NÀY ĐỂ CHẠY ANIMATION KHI KẾT THÚC GAME <---
        if (state_.getPhase() == GamePhase::GameOver) {
            view_.updateGameOverAnimation(dt);
        }
        // -----------------------------------------------------------

        state_.update(dt);

        // ====== THÊM ĐOẠN NÀY ĐỂ BẮT SỰ KIỆN PHÁT ÂM THANH ======
        // Kiểm tra cờ playHitSoundFlag từ PhysicsEngine thông qua GameState
        if (state_.getPhysicsEngine().playHitSoundFlag) {
            view_.playHitSound(); // Kích hoạt tiếng sút bên Game_Render
            state_.getPhysicsEngine().playHitSoundFlag = false; // Tắt cờ để không bị kêu liên tục
        }
        // ==========================================================
        // ====== BỔ SUNG ĐOẠN NÀY ĐỂ BẮT SỰ KIỆN TIẾNG VA CHẠM ======
        if (state_.getPhysicsEngine().playCollideSoundFlag) {
            view_.playCollideSound();
            state_.getPhysicsEngine().playCollideSoundFlag = false;
        }
        // ============================================================

        //sf::Vector2f ballPos = state_.getBall().getPosition();
        //printf("Ball Position: X = %.2f, Y = %.2f\n", ballPos.x, ballPos.y);

        if (state_.getPhase() == GamePhase::Playing && !isCurrentPlayerHuman()) {
            // CHỈ bắt đầu trừ thời gian chờ khi mọi thứ trên sân ĐÃ DỪNG LẠI HẲN
            if (state_.isEverythingStopped()) {
                if (aiThinking_) {
                    // Kiểm tra xem luồng AI đã mô phỏng xong chưa (không chặn Game Loop)
                    if (aiFuture_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                        AIShot shot = aiFuture_.get();
                        aiThinking_ = false;

                        Team turn = state_.getCurrentTurn();
                        if (!shot.valid || shot.pieceIndex < 0) {
                            endTurn();
                            return;
                        }
                        auto& pieces = state_.getPieces();
                        if (shot.pieceIndex >= static_cast<int>(pieces.size()) || pieces[shot.pieceIndex]->getTeam() != turn) {
                            endTurn();
                            return;
                        }
                        // Áp dụng cú sút
                        pieces[shot.pieceIndex]->setVelocity(shot.velocity);
                        state_.recordShot();
                        endTurn();
                    }
                }
                else {
                    aiThinkTimer_ -= dt;

                    // Nếu đã chờ đủ 2 giây
                    if (aiThinkTimer_ <= 0.f) {
                        // Ép GameState phải dọn dẹp cầu thủ kẹt trong gôn trước khi cho AI nhắm bắn
                        state_.resolveGoalCollisions();

                        triggerAITurn();
                    }
                }
            }
            else {
                // QUAN TRỌNG: Nếu bóng hoặc cầu thủ vẫn còn đang lăn, 
                // liên tục reset đồng hồ về 2 giây.
                aiThinkTimer_ = AI_DELAY_SEC;
                aiThinking_ = false;
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


    bool Game_Controller::isMouseOverInteractive(sf::Vector2f mousePos) const {
        GamePhase phase = state_.getPhase();

        if (phase == GamePhase::Menu) {
            // Nút PLAY
            if (sf::FloatRect({ 375.f, 210.f }, { 250.f, 80.f }).contains(mousePos)) return true;
            // Nút QUIT
            if (sf::FloatRect({ 925.f, 25.f }, { 50.f, 50.f }).contains(mousePos)) return true;
            // Nút OPTIONS
            if (sf::FloatRect({ 375.f, 340.f }, { 250.f, 80.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::Options) {
            // Nút Back
            if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mousePos)) return true;

            // 1. Nếu đang GIỮ chuột để kéo thanh trượt (dù chuột có trượt ra ngoài viền vẫn hiện bàn tay)
            if (view_.isDraggingOptions()) return true;

            // 2. Nếu đang DI CHUỘT lướt qua vùng chứa thanh Sound hoặc nút Sound
            if (sf::FloatRect({ 320.f, 220.f }, { 280.f, 50.f }).contains(mousePos)) return true;

            // 3. Nếu đang DI CHUỘT lướt qua vùng chứa thanh SFX hoặc nút SFX
            if (sf::FloatRect({ 320.f, 310.f }, { 280.f, 50.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::Setup) {
            // 3 nút chọn chế độ chơi
            if (sf::FloatRect({ 365.f, 170.f }, { 280.f, 70.f }).contains(mousePos)) return true;
            if (sf::FloatRect({ 365.f, 260.f }, { 280.f, 70.f }).contains(mousePos)) return true;
            if (sf::FloatRect({ 365.f, 350.f }, { 280.f, 70.f }).contains(mousePos)) return true;
            // Nút Back
            if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::PickTeam) {
            int startIdx = view_.getCurrentTeamPage() * 4;
            for (int i = 0; i < 4 && (startIdx + i) < view_.getTeamCount(); ++i) {
                int row = i / 2; int col = i % 2;
                if (sf::FloatRect({ 350.f + col * 300.f - 100.f, 160.f + row * 160.f - 60.f }, { 200.f, 150.f }).contains(mousePos)) {
                    // ---> CHẶN CON TRỎ CHUỘT BÀN TAY <---
                    if (view_.getPickingTeamFor() == 2 && view_.getTeamAbbrById(startIdx + i) == state_.getTeamAbbr(Team::Team1)) {
                        continue; // Trùng thì bỏ qua, coi như không di chuột vào
                    }
                    return true;
                }
            }
            if (sf::FloatRect({ 420.f, 425.f }, { 160.f, 50.f }).contains(mousePos)) return true; // Btn
            if (view_.getCurrentTeamPage() > 0 && sf::FloatRect({ 40.f, 230.f }, { 80.f, 40.f }).contains(mousePos)) return true;
            if ((view_.getCurrentTeamPage() + 1) * 4 < view_.getTeamCount() && sf::FloatRect({ 885.f, 230.f }, { 80.f, 40.f }).contains(mousePos)) return true;
            if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::PickLineup) {
            // Các card chọn đội hình
            int startIdx = view_.getCurrentPage() * 2;
            for (int i = 0; i < 2; ++i) {
                if (sf::FloatRect({ 230.f + i * 345.f, 130.f }, { 200.f, 220.f }).contains(mousePos)) return true;
            }
            // Nút NEXT/START
            if (sf::FloatRect({ 420.f, 425.f }, { 160.f, 50.f }).contains(mousePos)) return true;
            // Nút Previous Page
            if (view_.getCurrentPage() > 0) {
                if (sf::FloatRect({ 40.f, 210.f }, { 80.f, 40.f }).contains(mousePos)) return true;
            }
            // Nút Next Page
            if ((view_.getCurrentPage() + 1) * 2 < view_.getLineupCount()) {
                if (sf::FloatRect({ 885.f, 210.f }, { 80.f, 40.f }).contains(mousePos)) return true;
            }
            // Nút Back
            if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::ConfirmQuit) {
            // Nút YES
            if (sf::FloatRect({ 353.f, 273.f }, { 135.f, 47.f }).contains(mousePos)) return true;
            // Nút NO
            if (sf::FloatRect({ 515.f, 273.f }, { 135.f, 47.f }).contains(mousePos)) return true;
        }
        else if (phase == GamePhase::Playing) {
            // Nút Back
            if (sf::FloatRect({ 30.f, 30.f }, { 40.f, 40.f }).contains(mousePos)) return true;
            if (sf::FloatRect({ 930.f, 30.f }, { 50.f, 50.f }).contains(mousePos)) return true;
            if (state_.isEverythingStopped() && isCurrentPlayerHuman()) {
                if (getPieceIndexAt(mousePos) != -1) return true;
            }
        }

        return false;
    }


} // namespace SoccerPool
