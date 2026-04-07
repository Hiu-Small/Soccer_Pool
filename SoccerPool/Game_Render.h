#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <memory>
#include <SFML/Audio.hpp> // Thêm dòng này ở đầu file .h

namespace SoccerPool {

class GameState;

// View: chỉ vẽ dữ liệu từ Model (GameState), không xử lý logic
class Game_Render {
public:
    Game_Render();

    void setState(GameState* state) { state_ = state; }
    void setViewSize(unsigned width, unsigned height) { viewWidth_ = width; viewHeight_ = height; }

    void draw(sf::RenderWindow& window);
    //Ưvoid drawDragIndicator(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to);
    // Mũi tên chỉ hướng sút (vẽ phía trước cầu thủ khi kéo)
    //void drawDirectionArrow(sf::RenderWindow& window, sf::Vector2f piecePos, sf::Vector2f directionUnit, float length);
    // Gọi trước draw() để vẽ thanh kéo + mũi tên trong cùng view

    // Hàm mới để vẽ toàn bộ hiệu ứng ngắm sút
    void drawShotAiming(sf::RenderWindow& window);
    void setDragState(sf::Vector2f from, sf::Vector2f to, sf::Vector2f piecePos, sf::Vector2f directionUnit);
    void clearDragState() { dragActive_ = false; }

    // Chuyển từ tọa độ thế giới (sân) sang tọa độ màn hình
    sf::Vector2f worldToScreen(sf::Vector2f world) const;
    sf::Vector2f screenToWorld(sf::Vector2f screen) const;

    int getCurrentPage() {
		return currentLineupPage_;
    }

    // Lấy ID của đội hình tại một chỉ số cụ thể trong mảng lineups_
    int getLineupId(size_t index) const {
        if (index < lineups_.size()) return lineups_[index]->id;
        return 0; // Trả về mặc định nếu lỗi
    }

    int getPickingTeam() const { return pickingForTeam_; }
    void setPickingTeam(int team) { pickingForTeam_ = team; }

    void nextPage() {
        // Nếu còn trang tiếp theo (mỗi trang hiện 3 item)
        if ((currentLineupPage_ + 1) * 2 < lineups_.size()) {
            currentLineupPage_++;
        }
    }

    void prevPage() {
        if (currentLineupPage_ > 0) {
            currentLineupPage_--;
        }
    }

    void setSelectedLineupId(int id) { selectedLineupId_ = id; }
    int getSelectedLineupId() const { return selectedLineupId_; }

    size_t getLineupCount() const {
        return lineups_.size();
    }

    void resetSelectionState() {
        currentLineupPage_ = 0;       // Về trang đầu
        selectedLineupId_ = -1;      // Bỏ chọn đội hình
        pickingForTeam_ = 1;         // Reset về Team 1 chọn đầu tiên
    }

    void setCurrentPage(int page) { currentLineupPage_ = page; }

    void handleEvent(const sf::Event& event, sf::RenderWindow& window, sf::Vector2f mousePos);
    void updateVolumeFromMouse(sf::Vector2f mousePos);
    void setSoundVolume(float vol);
    void setSFXVolume(float vol);
    float getSoundVolume() const { return soundVolume_; }
    float getSFXVolume() const { return sfxVolume_; }
    void playHitSound();
    void playCollideSound();
	void playWhistleSound();
    void playGoalSound();
	void playGoalMusic();

    bool isDraggingOptions() const { return isDraggingSound_ || isDraggingSFX_; }

    void startGoalAnimation();
    void updateGoalAnimation(float dt);
    bool isGoalAnimationDone() const;

    // ---> THÊM 2 HÀM NÀY CHO GAME OVER <---
    void resetGameOverAnimation() { gameOverAnimTimer_ = 0.f; }
    void updateGameOverAnimation(float dt) { gameOverAnimTimer_ += dt; }

private:
    void drawField(sf::RenderWindow& window);
    void drawGoals(sf::RenderWindow& window);
    void drawBall(sf::RenderWindow& window);
    void drawPieces(sf::RenderWindow& window);
    void drawUI(sf::RenderWindow& window);
    //void drawMenu(sf::RenderWindow& window);
    void drawGameOver(sf::RenderWindow& window);

    void drawMainMenu(sf::RenderWindow& window);
    void drawSelectMode(sf::RenderWindow& window);

    void drawSelectLineup(sf::RenderWindow& window);
    void drawConfirmQuit(sf::RenderWindow& window);

    void applyHoverEffect(sf::Sprite& sprite, sf::FloatRect bounds, sf::Vector2f mPos, float baseScale);

    void drawOptionsMenu(sf::RenderWindow& window);

    GameState* state_ = nullptr;
    unsigned viewWidth_ = 1000;
    unsigned viewHeight_ = 600;
    float scaleX_ = 1.f;
    float scaleY_ = 1.f;
    float offsetX_ = 0.f;
    float offsetY_ = 0.f;
    void updateTransform();

    sf::Texture fieldTexture_; // Biến chứa dữ liệu ảnh
    sf::Sprite fieldSprite_;   // Biến để vẽ ảnh
    bool isFieldLoaded_ = false; // Biến kiểm tra xem load ảnh thành công không

    sf::Texture ballTexture_;
    sf::Sprite ballSprite_;
    bool isBallLoaded_ = false;
    // --- THÊM 2 BIẾN NÀY ---
    float currentBallFrame_ = 0.f; // Khung hình hiện tại (dùng float để cộng dồn mượt mà)
    sf::Vector2i ballFrameSize_;   // Kích thước của 1 khung hình cắt ra

    sf::Texture team1Texture_;
    sf::Sprite team1Sprite_;
    bool isTeam1Loaded_ = false;

    sf::Texture team2Texture_;
    sf::Sprite team2Sprite_;
    bool isTeam2Loaded_ = false;

    bool dragActive_ = false;
    sf::Vector2f dragFrom_, dragTo_, dragPiecePos_, dragDir_;

    sf::Clock effectClock_;

    // --- THÊM BIẾN LOAD ẢNH SCOREBOARD ---
    sf::Texture sbTexture_;
    sf::Sprite sbSprite_;
    bool isSbLoaded_ = false;


    // Tỉ lệ âm lượng (0.0f đến 1.0f)
    float soundVolume_ = 0.5f;
    float sfxVolume_ = 0.5f;

    // ---> THÊM 2 BIẾN NÀY ĐỂ LÀM MƯỢT ÂM THANH <---
    float targetBgVolumeMult_ = 1.0f;  // Mức âm lượng muốn đạt tới
    float currentBgVolumeMult_ = 1.0f; // Mức âm lượng hiện tại đang phát

    // Các vị trí cố định của slider (để dễ quản lý)
    const sf::Vector2f SOUND_SLIDER_POS = { 525.f, 225.f }; // Khớp với chữ "Sound" trên khung
    const sf::Vector2f SFX_SLIDER_POS = { 525.f, 290.f }; // Khớp với chữ "SFX" trên khung
    //const float SLIDER_WIDTH = 250.f; // Độ dài tối đa của thanh trượt

    sf::Music bgMusic_;          // Nhạc nền (Music dùng cho file dài)

    sf::SoundBuffer hitBufferKick_;  // Bộ đệm âm thanh va chạm
    sf::Sound hitSoundKick_;         // Đối tượng phát âm thanh va chạm

	sf::SoundBuffer hitBufferCollide_;
	sf::Sound hitSoundCollide_;

	sf::SoundBuffer whistleBuffer_;
	sf::Sound whistleSound_;

	sf::SoundBuffer goalScoreBuffer_;
	sf::Sound goalScoreSound_;

    sf::Music stadiumEffect;

	sf::SoundBuffer goalMusicBuffer_;
	sf::Sound goalMusicSound_;

    void updateSoundSliderVisual();
    void updateSFXSliderVisual();

    bool isDraggingSound_ = false; // Trạng thái đang kéo thanh Sound
    bool isDraggingSFX_ = false;   // Trạng thái đang kéo thanh SFX

    // Vị trí và kích thước thanh trượt
    const float SLIDER_START_X = 500.f - 95.f; // 365.f
    const float SLIDER_END_X = 500.f + 120.f;   // 635.f
    const float SLIDER_WIDTH = 215.f;
    const float SOUND_SLIDER_Y = 240.f;
    const float SFX_SLIDER_Y = 330.f;

    sf::Font sbFont_;
    sf::Font goalFont_;

    // Texture cho Menu bóc tách
    sf::Texture menuBgTexture_;      // Nền cỏ
    sf::Texture btnPlayTexture_;     
    sf::Texture btnOptionsTexture_;
    sf::Texture ball8Texture_;       // Quả bóng số 8 trang trí
    sf::Texture goalMenuTexture_;    // Khung lưới trang trí
    sf::Texture iconQuitTexture_;    // Icon quay lại
    sf::Texture iconInforTexture_;
	sf::Texture ballMenuTexture_;   // Bóng trang trí menu
	sf::Texture iconReturnTexture_;  // Icon quay lại
	sf::Texture pvpTexture_;           // Nút PvP
	sf::Texture pvaiTexture_;          // Nút PvAI
	sf::Texture aivaiTexture_;           // Nút AI vs AI
    sf::Texture msbQuitTexture_;
	sf::Texture iconOptionsTexture_; // Icon cài đặt (bánh răng)

    // Sprite tương ứng
    sf::Sprite menuBgSprite_;
    sf::Sprite ball8Sprite_;
    sf::Sprite goalMenuSprite_;
    sf::Sprite ballMenuSprite_;
	sf::Sprite btnPlaySprite_;
	sf::Sprite btnOptionsSprite_;
	sf::Sprite iconQuitSprite_;
	sf::Sprite iconInforSprite_;
	sf::Sprite iconReturnSprite_;
	sf::Sprite pvpSprite_;
	sf::Sprite pvaiSprite_;
	sf::Sprite aivaiSprite_;
	sf::Sprite msbQuitSprite_;
	sf::Sprite iconOptionsSprite_;

    struct LineupOption {
        int id;
        std::string name;

        // ĐƯA TEXTURE LÊN TRƯỚC
        std::shared_ptr<sf::Texture> texture;

        // ĐƯA SPRITE XUỐNG DƯỚI
        sf::Sprite sprite;

        LineupOption()
            : id(0),
            texture(std::make_shared<sf::Texture>()),
            sprite(*texture) // Bây giờ texture đã có trước nên sprite sẽ nhận được
        {
        }
    };

    int selectedLineupId_ = -1;

    std::vector<std::shared_ptr<LineupOption>> lineups_;
    int currentLineupPage_ = 0; // Trang hiện tại
    int pickingForTeam_ = 1;    // Đang chọn cho Team 1 hay Team 2

    // Thêm các icon mũi tên trái/phải và nút START
    sf::Texture arrowLeftTexture_, arrowRightTexture_ ,startBtnTexture_, nextBtnTexture_;
    sf::Sprite arrowLeftSprite_, arrowRightSprite_, startBtnSprite_, nextBtnSprite_;

	sf::Texture slideBarSoundTexture_, slideBarSFXTexture_, slideNodeSoundTexture_, slideNodeSFXTexture_, optionsKhungTexture_;
	sf::Sprite slideBarSoundSprite_, slideBarSFXSprite_, slideNodeSoundSprite_, slideNodeSFXSprite_, optionsKhungSprite_;


    sf::RectangleShape leftGoalBanner_;
    sf::RectangleShape rightGoalBanner_;
    sf::Text goalText_;
    int goalAnimState_ = 0; // 0: Tắt, 1: Trượt vào, 2: Chờ âm thanh, 3: Trượt ra
    float goalAnimTimer_ = 0.f;
    const float GOAL_ANIM_DURATION = 0.5f; // Tốc độ trượt (0.5 giây)
    void drawGoalAnimation(sf::RenderWindow& window);

    float gameOverAnimTimer_ = 0.f; // ---> THÊM DÒNG NÀY

    // Font dành riêng cho Menu (nên dùng font dày)
    sf::Font menuFont_;

};

} // namespace SoccerPool
