#include "Game_Render.h"
#include "GameState.h"
#include "Constants.h"
#include <cmath>
#include <string>
#include <iostream> // Thêm thư viện này để báo lỗi nếu không tìm thấy ảnh

namespace SoccerPool {

Game_Render::Game_Render() : fieldSprite_(fieldTexture_), ballSprite_(ballTexture_), team1Sprite_(team1Texture_), team2Sprite_(team2Texture_), sbSprite_(sbTexture_), menuBgSprite_(menuBgTexture_), ball8Sprite_(ball8Texture_), goalMenuSprite_(goalMenuTexture_), ballMenuSprite_(ballMenuTexture_), btnPlaySprite_(btnPlayTexture_), btnOptionsSprite_(btnOptionsTexture_), iconQuitSprite_(iconQuitTexture_), iconInforSprite_(iconInforTexture_), iconReturnSprite_(iconReturnTexture_), pvpSprite_(pvpTexture_), pvaiSprite_(pvaiTexture_), aivaiSprite_(aivaiTexture_), arrowLeftSprite_(arrowLeftTexture_), arrowRightSprite_(arrowRightTexture_), startBtnSprite_(startBtnTexture_), nextBtnSprite_(nextBtnTexture_) {
    // 1. Cố gắng load file ảnh
    // Đảm bảo bạn đã tạo thư mục 'assets' và bỏ file ảnh vào đó!
    if (!fieldTexture_.loadFromFile("assets/field_4.png")) {
        // Nếu không load được, báo lỗi ra màn hình console
        std::cerr << "LOI: Khong the load anh san bong (assets/field_4.png)!" << std::endl;
        isFieldLoaded_ = false;
    }
    else {
        // Nếu load thành công
        isFieldLoaded_ = true;
        std::cout << "Da load anh san bong thanh cong." << std::endl;

        // Để ảnh mượt hơn khi bị co giãn
        fieldTexture_.setSmooth(true);

        // Gán texture cho sprite
        fieldSprite_.setTexture(fieldTexture_);

        // --- QUAN TRỌNG: CO GIÃN ẢNH CHO VỪA SÂN LOGIC ---
        // Ảnh của bạn có thể là 1920x1080, nhưng sân logic chỉ là 900x500 (FIELD_WIDTH x FIELD_HEIGHT)
        // Ta phải tính tỉ lệ để ép ảnh vừa khít kích thước logic.
        sf::Vector2u imageSize = fieldTexture_.getSize();

        // Cập nhật lại vùng hiển thị của Sprite cho bằng đúng kích thước ảnh vừa load
        fieldSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(imageSize.x), static_cast<int>(imageSize.y) }));

        float scaleX = FIELD_WIDTH / static_cast<float>(imageSize.x);
        float scaleY = FIELD_HEIGHT / static_cast<float>(imageSize.y);

        // Áp dụng tỉ lệ scale (SFML 3 dùng ngoặc nhọn)
        fieldSprite_.setScale({ scaleX, scaleY });

        // Đặt vị trí bắt đầu ở góc 0,0 của thế giới game
        fieldSprite_.setPosition({ 0.f, 0.f });
    }

    // 2. LOAD ẢNH BÓNG
    if (!ballTexture_.loadFromFile("assets/ball.all_.png")) {
        std::cerr << "LOI: Khong the load anh bong (assets/ball.all_.png)!" << std::endl;
        isBallLoaded_ = false;
    }
    else {
        isBallLoaded_ = true;
        ballTexture_.setSmooth(true); // Làm mịn để bóng không bị răng cưa

        // Lấy kích thước ảnh gốc
        sf::Vector2u texSize = ballTexture_.getSize();

        // Ảnh có 8 cột và 8 hàng. Cắt ra lấy kích thước 1 ô.
        ballFrameSize_.x = texSize.x / 8;
        ballFrameSize_.y = texSize.y / 8;

        // Cập nhật vùng hiển thị ban đầu là ô đầu tiên (góc trên cùng bên trái)
        ballSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { ballFrameSize_.x, ballFrameSize_.y }));

        // Đặt tâm về giữa CỦA 1 Ô (chứ không phải giữa cả bức ảnh to)
        ballSprite_.setOrigin({ static_cast<float>(ballFrameSize_.x) / 2.f, static_cast<float>(ballFrameSize_.y) / 2.f });

        // Tính tỷ lệ co giãn (Scale) dựa trên kích thước của 1 ô
        float targetDiameter = 2.f * BALL_RADIUS;
        float scaleX = targetDiameter / static_cast<float>(ballFrameSize_.x);
        float scaleY = targetDiameter / static_cast<float>(ballFrameSize_.y);

        ballSprite_.setScale({ scaleX, scaleY });

        // --- THÊM DÒNG QUAN TRỌNG NÀY ---
        // Nếu thiếu dòng này, Sprite sẽ có kích thước 0x0 và không hiện gì cả
        //ballSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(texSize.x), static_cast<int>(texSize.y) }));

        //// A. ĐẶT TÂM VỀ GIỮA ẢNH (Quan trọng!)
        //// Để tọa độ vật lý (ở tâm bóng) trùng với tâm của bức ảnh
        //ballSprite_.setOrigin({ static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f });

        //// B. TÍNH TỶ LỆ CO GIÃN (Scale)
        //// Đường kính mong muốn = 2 * Bán kính (BALL_RADIUS)
        //// Tỷ lệ = Đường kính mong muốn / Kích thước ảnh thật
        //float targetDiameter = 2.f * BALL_RADIUS;
        //float scaleX = targetDiameter / static_cast<float>(texSize.x);
        //float scaleY = targetDiameter / static_cast<float>(texSize.y);

        //ballSprite_.setScale({ scaleX, scaleY });
    }

    // --- 3. LOAD ẢNH ĐỘI 1 ---
    if (!team1Texture_.loadFromFile("assets/team1.png")) {
        std::cerr << "LOI: Khong load duoc assets/team1.png" << std::endl;
        isTeam1Loaded_ = false;
    }
    else {
        isTeam1Loaded_ = true;
        team1Texture_.setSmooth(true);
        sf::Vector2u t1Size = team1Texture_.getSize();

        // Cập nhật vùng hiển thị (Bắt buộc ở SFML 3)
        team1Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t1Size.x), static_cast<int>(t1Size.y) }));

        // Đặt tâm về giữa ảnh
        team1Sprite_.setOrigin({ static_cast<float>(t1Size.x) / 2.f, static_cast<float>(t1Size.y) / 2.f });

        // Tính tỷ lệ scale (Đường kính = 2 * PIECE_RADIUS)
        float scale = (2.f * PIECE_RADIUS) / static_cast<float>(t1Size.x);
        // (Giả sử ảnh vuông nên scale X Y bằng nhau)
        team1Sprite_.setScale({ scale, scale });
    }

    // --- 4. LOAD ẢNH ĐỘI 2 ---
    if (!team2Texture_.loadFromFile("assets/team2.png")) {
        std::cerr << "LOI: Khong load duoc assets/team2.png" << std::endl;
        isTeam2Loaded_ = false;
    }
    else {
        isTeam2Loaded_ = true;
        team2Texture_.setSmooth(true);
        sf::Vector2u t2Size = team2Texture_.getSize();

        // Cập nhật vùng hiển thị
        team2Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t2Size.x), static_cast<int>(t2Size.y) }));

        // Đặt tâm về giữa ảnh
        team2Sprite_.setOrigin({ static_cast<float>(t2Size.x) / 2.f, static_cast<float>(t2Size.y) / 2.f });

        // Tính tỷ lệ scale
        float scale = (2.f * PIECE_RADIUS) / static_cast<float>(t2Size.x);
        team2Sprite_.setScale({ scale, scale });
    }

    // --- LOAD ẢNH BẢNG TỈ SỐ (image_5.png) ---
    // Đảm bảo bạn đã copy ảnh image_5.png vào thư mục assets/ và đổi tên thành ti_so_xoa_ti_so.png
    // --- LOAD ẢNH BẢNG TỈ SỐ ---
    if (!sbTexture_.loadFromFile("assets/ti_so_xoa_ti_so.png")) {
        std::cerr << "LOI: Khong load duoc assets/ti_so_xoa_ti_so.png" << std::endl;
        isSbLoaded_ = false;
    }
    else {
        isSbLoaded_ = true;
        sbTexture_.setSmooth(true);

        // QUAN TRỌNG: Gán lại texture sau khi đã load dữ liệu vào RAM
        sbSprite_.setTexture(sbTexture_);

        // QUAN TRỌNG: Cập nhật vùng cắt ảnh (TextureRect) cho Sprite
        sf::Vector2u texSize = sbTexture_.getSize();
        sbSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(texSize.x), static_cast<int>(texSize.y) }));

        float targetWidth = 350.f;
        float s = targetWidth / static_cast<float>(texSize.x);
        sbSprite_.setScale({ s, s });

        // Đặt tâm của ảnh vào chính giữa bức ảnh để dễ đặt vào giữa sân
        sbSprite_.setOrigin({ texSize.x / 2.f, texSize.y / 2.f });


        // Đặt ở tọa độ (500, 300) như trong ảnh bạn vừa chụp
        sbSprite_.setPosition({ FIELD_WIDTH / 2.f, 40.f });

        //std::cout << "Da gán texture vao Sprite thanh cong. Kich thuoc anh: " << texSize.x << "x" << texSize.y << std::endl;
    }

    // Load font chữ (Dùng lại Arial hoặc load font thể thao)
    if (!sbFont_.openFromFile("C:/Windows/Fonts/arial.ttf")) { /* báo lỗi */ }



    // Load các thành phần Menu bóc tách
    if (!menuBgTexture_.loadFromFile("assets/menu_bg.png")) {
        std::cerr << "Khong the load file: assets/menu_bg.png" << std::endl;
    }
    if (!ball8Texture_.loadFromFile("assets/ball_8.png")) {
        std::cerr << "Khong the load file: assets/ball_8.png" << std::endl;
    }
    if (!goalMenuTexture_.loadFromFile("assets/goal_menu.png")) {
        std::cerr << "Khong the load file: assets/goal_menu.png" << std::endl;
    }
    if (!ballMenuTexture_.loadFromFile("assets/ball_menu.png")) {
        std::cerr << "Khong the load file: assets/ball_menu.png" << std::endl;
    }
    if (!btnPlayTexture_.loadFromFile("assets/btn_play.png")) {
        std::cerr << "Khong the load file: assets/btn_play.png" << std::endl;
	}
    if (!btnOptionsTexture_.loadFromFile("assets/btn_options.png")) {
        std::cerr << "Khong the load file: assets/btn_options.png" << std::endl;
	}
    if (!iconQuitTexture_.loadFromFile("assets/icon_quit.png")) {
        std::cerr << "Khong the load file: assets/icon_quit.png" << std::endl;
    }
    if (!iconInforTexture_.loadFromFile("assets/icon_infor.png")) {
        std::cerr << "Khong the load file: assets/icon_infor.png" << std::endl;
	}
    if (!iconReturnTexture_.loadFromFile("assets/icon_return.png")) {
        std::cerr << "Khong the load file: assets/icon_return.png" << std::endl;
    }
    if(!pvpTexture_.loadFromFile("assets/pvp.png")) {
        std::cerr << "Khong the load file: assets/pvp.png" << std::endl;
	}
    if (!pvaiTexture_.loadFromFile("assets/pvai.png")) {
        std::cerr << "Khong the load file: assets/pvai.png" << std::endl;
    }
    if (!aivaiTexture_.loadFromFile("assets/aivai.png")) {
        std::cerr << "Khong the load file: assets/aivai.png" << std::endl;
	}
    if (!arrowLeftTexture_.loadFromFile("assets/arrow_left.png")) {
        std::cerr << "Khong the load file: assets/arrow_left.png" << std::endl;
	}
    if (!arrowRightTexture_.loadFromFile("assets/arrow_right.png")) {
        std::cerr << "Khong the load file: assets/arrow_right.png" << std::endl;
    }
    if (!startBtnTexture_.loadFromFile("assets/start_btn.png")) {
        std::cerr << "Khong the load file: assets/start_btn.png" << std::endl;
	}
    if (!nextBtnTexture_.loadFromFile("assets/next_btn.png")) {
        std::cerr << "Khong the load file: assets/next_btn.png" << std::endl;
	}
    //btnGrayTexture_.loadFromFile("assets/btn_gray.png"); // Chỉ có hình chữ nhật bo góc xám
    //iconPlayTexture_.loadFromFile("assets/icon_play.png");
    //iconGearTexture_.loadFromFile("assets/icon_gear.png");
    //iconQuitTexture_.loadFromFile("assets/icon_quit.png");
    //menuFont_.loadFromFile("assets/fonts/arial_bold.ttf");

    //doan nay ko can
 //   menuBgSprite_.setTexture(menuBgTexture_);
 //   ball8Sprite_.setTexture(ball8Texture_);
 //   goalMenuSprite_.setTexture(goalMenuTexture_);
	//ballMenuSprite_.setTexture(ballMenuTexture_);

    // Cập nhật vùng hiển thị (TextureRect) để đảm bảo Sprite có kích thước
    menuBgSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)menuBgTexture_.getSize().x, (int)menuBgTexture_.getSize().y }));
    ball8Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)ball8Texture_.getSize().x, (int)ball8Texture_.getSize().y }));
    goalMenuSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)goalMenuTexture_.getSize().x, (int)goalMenuTexture_.getSize().y }));
    ballMenuSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)ballMenuTexture_.getSize().x, (int)ballMenuTexture_.getSize().y }));
	btnOptionsSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)btnOptionsTexture_.getSize().x, (int)btnOptionsTexture_.getSize().y }));
	btnPlaySprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)btnPlayTexture_.getSize().x, (int)btnPlayTexture_.getSize().y }));
	iconInforSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconInforTexture_.getSize().x, (int)iconInforTexture_.getSize().y }));
	iconQuitSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconQuitTexture_.getSize().x, (int)iconQuitTexture_.getSize().y }));
	iconReturnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconReturnTexture_.getSize().x, (int)iconReturnTexture_.getSize().y }));
	pvpSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)pvpTexture_.getSize().x, (int)pvpTexture_.getSize().y }));
	pvaiSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)pvaiTexture_.getSize().x, (int)pvaiTexture_.getSize().y }));
	aivaiSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)aivaiTexture_.getSize().x, (int)aivaiTexture_.getSize().y }));
	arrowLeftSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)arrowLeftTexture_.getSize().x, (int)arrowLeftTexture_.getSize().y }));
	arrowRightSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)arrowRightTexture_.getSize().x, (int)arrowRightTexture_.getSize().y }));
	startBtnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)startBtnTexture_.getSize().x, (int)startBtnTexture_.getSize().y }));
	nextBtnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)nextBtnTexture_.getSize().x, (int)nextBtnTexture_.getSize().y }));

    // 1. Xử lý Nền Menu (menuBg) - Ép ảnh nền vừa khít kích thước FIELD_WIDTH x FIELD_HEIGHT
    sf::Vector2u bgSize = menuBgTexture_.getSize();
    float scaleBgX = FIELD_WIDTH / static_cast<float>(bgSize.x);
    float scaleBgY = FIELD_HEIGHT / static_cast<float>(bgSize.y);
    menuBgSprite_.setScale({ scaleBgX, scaleBgY });
    menuBgSprite_.setOrigin({ 0.f, 0.f }); // Nền thường đặt gốc ở 0,0 để lấp đầy từ góc trái
    menuBgSprite_.setPosition({ 0.f, 0.f });

    sf::Vector2u ball8Size = ball8Texture_.getSize();
    float targetWidth = 200.f;
    float s = targetWidth / static_cast<float>(ball8Size.x);
    ball8Sprite_.setScale({ s, s });
    ball8Sprite_.setOrigin({ ball8Size.x / 2.f, ball8Size.y / 2.f });
    ball8Sprite_.setPosition({ 150.f, 150.f });

    // 3. Xử lý Khung lưới (goalMenu)
    sf::Vector2u goalSize = goalMenuTexture_.getSize();
    float targetGoalWidth = 250.f; // Cho khung lưới to hơn bóng một chút
    float sGoal = targetGoalWidth / static_cast<float>(goalSize.x);
    goalMenuSprite_.setScale({ sGoal, sGoal });
    // Đặt tâm ở giữa dưới (bottom-center) để dễ đặt vào góc sân
    goalMenuSprite_.setOrigin({ goalSize.x / 2.f, static_cast<float>(goalSize.y) });
    goalMenuSprite_.setPosition({ 850.f, 530.f }); // Đặt ở góc dưới bên phải màn hình

	// 4. Xử lý Bóng trang trí Menu (ballMenu)
    sf::Vector2u ballMenuSize = ballMenuTexture_.getSize();
    float targetBallWidth = 150.f; // Cho khung lưới to hơn bóng một chút
    float sBall = targetBallWidth / static_cast<float>(ballMenuSize.x);
    ballMenuSprite_.setScale({ sBall, sBall });
    ballMenuSprite_.setOrigin({ ballMenuSize.x / 2.f, ballMenuSize.y / 2.f });
    ballMenuSprite_.setPosition({ 850.f, 170.f }); // Đặt ở góc dưới bên phải màn hình

	// 5. Xử lý Nút Play
	sf::Vector2u btnPlaySize = btnPlayTexture_.getSize();
	float targetBtnWidth = 800.f;
	float sBtnPlay = targetBtnWidth / static_cast<float>(btnPlaySize.x);
	btnPlaySprite_.setScale({ sBtnPlay, sBtnPlay });
	btnPlaySprite_.setOrigin({ btnPlaySize.x / 2.f, btnPlaySize.y / 2.f });
	btnPlaySprite_.setPosition({ 500.f, 250.f });

	// 6. Xử lý Nút Options
	sf::Vector2u btnOptionsSize = btnOptionsTexture_.getSize();
	float targetBtnOptionsWidth = 800.f;
	float sBtnOptions = targetBtnOptionsWidth / static_cast<float>(btnOptionsSize.x);
	btnOptionsSprite_.setScale({ sBtnOptions, sBtnOptions });
	btnOptionsSprite_.setOrigin({ btnOptionsSize.x / 2.f, btnOptionsSize.y / 2.f });
	btnOptionsSprite_.setPosition({ 500.f, 380.f });

	sf::Vector2u iconQuitSize = iconQuitTexture_.getSize();
	float targetIconQuitWidth = 180.f;
	float sIconQuit = targetIconQuitWidth / static_cast<float>(iconQuitSize.x);
	iconQuitSprite_.setScale({ sIconQuit, sIconQuit });
	iconQuitSprite_.setOrigin({ iconQuitSize.x / 2.f, iconQuitSize.y / 2.f });
	iconQuitSprite_.setPosition({ 950.f, 50.f });

	sf::Vector2u iconInforSize = iconInforTexture_.getSize();
	float targetIconInforWidth = 40.f;
	float sIconInfor = targetIconInforWidth / static_cast<float>(iconInforSize.x);
	iconInforSprite_.setScale({ sIconInfor, sIconInfor });
	iconInforSprite_.setOrigin({ iconInforSize.x / 2.f, iconInforSize.y / 2.f });
	iconInforSprite_.setPosition({ 900.f, 50.f });

	sf::Vector2u iconReturnSize = iconReturnTexture_.getSize();
	float targetIconReturnWidth = 180.f;
	float sIconReturn = targetIconReturnWidth / static_cast<float>(iconReturnSize.x);
	iconReturnSprite_.setScale({ sIconReturn, sIconReturn });
	iconReturnSprite_.setOrigin({ iconReturnSize.x / 2.f, iconReturnSize.y / 2.f });
	iconReturnSprite_.setPosition({ 50.f, 50.f });

	sf::Vector2u pvpSize = pvpTexture_.getSize();
	float targetPvpWidth = 550.f;
	float sPvp = targetPvpWidth / static_cast<float>(pvpSize.x);
	pvpSprite_.setScale({ sPvp, sPvp });
	pvpSprite_.setOrigin({ pvpSize.x / 2.f, pvpSize.y / 2.f });
	pvpSprite_.setPosition({ 490.f, 200.f });

	sf::Vector2u pvaiSize = pvaiTexture_.getSize();
	float targetPvaiWidth = 550.f;
	float sPvai = targetPvaiWidth / static_cast<float>(pvaiSize.x);
	pvaiSprite_.setScale({ sPvai, sPvai });
	pvaiSprite_.setOrigin({ pvaiSize.x / 2.f, pvaiSize.y / 2.f });
	pvaiSprite_.setPosition({ 490.f, 290.f });

	sf::Vector2u aivaiSize = aivaiTexture_.getSize();
	float targetAivaiWidth = 550.f;
	float sAivai = targetAivaiWidth / static_cast<float>(aivaiSize.x);
	aivaiSprite_.setScale({ sAivai, sAivai });
	aivaiSprite_.setOrigin({ aivaiSize.x / 2.f, aivaiSize.y / 2.f });
	aivaiSprite_.setPosition({ 490.f, 380.f });



    for (int i = 0; i < 7; ++i) {
        auto opt = std::make_shared<LineupOption>();
        // 2. Gán dữ liệu (Dùng dấu -> thay vì dấu .)
        /*opt->id = (i == 0) ? 5 : (10 + i)*/;
		opt->id = i + 1; 

        if(i == 0) {
            opt->name = "1-2-2";
        }
        else if(i == 1) {
            opt->name = "1-1-3";
        }
        else if(i == 2) {
            opt->name = "1-2-1-1";
        }
        else if(i == 3) {
            opt->name = "0-0-5";
        }
        else if(i == 4) {
            opt->name = "0-3-2";
        }
        else if(i == 5) {
            opt->name = "0-2-3";
        }
        else if(i == 6) {
            opt->name = "1-1-1-2";
		}

        std::string path = "assets/lineup/" + std::to_string(i) + ".png";
        if (!opt->texture->loadFromFile(path)) {
            std::cerr << "LOI: Khong load duoc " << path << std::endl;
        }

        //opt->sprite.setTexture(*opt->texture);
        opt->sprite.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(opt->texture->getSize().x), static_cast<int>(opt->texture->getSize().y) }));

        sf::Vector2u optSize = opt->texture->getSize();
        float optTargetWidth = 200.f;
        float sOptLeft = optTargetWidth / static_cast<float>(optSize.x);
        opt->sprite.setScale({ sOptLeft, sOptLeft });
		opt->sprite.setOrigin({ opt->texture->getSize().x / 2.f, opt->texture->getSize().y / 2.f });


        // 3. Đẩy vào vector
        lineups_.push_back(opt);
    }

	sf::Vector2u arrowLeftSize = arrowLeftTexture_.getSize();
	float targetArrowWidth = 200.f;
	float sArrowLeft = targetArrowWidth / static_cast<float>(arrowLeftSize.x);
	arrowLeftSprite_.setScale({ sArrowLeft, sArrowLeft });
	arrowLeftSprite_.setOrigin({ arrowLeftSize.x / 2.f, arrowLeftSize.y / 2.f });
	arrowLeftSprite_.setPosition({ 80.f, 230.f });

	sf::Vector2u arrowRightSize = arrowRightTexture_.getSize();
	float targetArrowRightWidth = 200.f;
	float sArrowRight = targetArrowRightWidth / static_cast<float>(arrowRightSize.x);
	arrowRightSprite_.setScale({ sArrowRight, sArrowRight });
	arrowRightSprite_.setOrigin({ arrowRightSize.x / 2.f, arrowRightSize.y / 2.f });
	arrowRightSprite_.setPosition({ 925.f, 230.f });

	sf::Vector2u startBtnSize = startBtnTexture_.getSize();
	float targetStartBtnWidth = 400.f;
	float sStartBtn = targetStartBtnWidth / static_cast<float>(startBtnSize.x);
	startBtnSprite_.setScale({ sStartBtn, sStartBtn });
	startBtnSprite_.setOrigin({ startBtnSize.x / 2.f, startBtnSize.y / 2.f });
	startBtnSprite_.setPosition({ 500.f, 450.f });

	sf::Vector2u nextBtnSize = nextBtnTexture_.getSize();
	float targetNextBtnWidth = 400.f;
	float sNextBtn = targetNextBtnWidth / static_cast<float>(nextBtnSize.x);
	nextBtnSprite_.setScale({ sNextBtn, sNextBtn });
	nextBtnSprite_.setOrigin({ nextBtnSize.x / 2.f, nextBtnSize.y / 2.f });
	nextBtnSprite_.setPosition({ 500.f, 450.f });

    updateTransform();
}

void Game_Render::updateTransform() {
    scaleX_ = static_cast<float>(viewWidth_) / FIELD_WIDTH;
    scaleY_ = static_cast<float>(viewHeight_) / FIELD_HEIGHT;
    //float scale = (scaleX_ < scaleY_) ? scaleX_ : scaleY_;
    //scaleX_ = scale;
    //scaleY_ = scale;
    //offsetX_ = (viewWidth_ - FIELD_WIDTH * scale) / 2.f;
    //offsetY_ = (viewHeight_ - FIELD_HEIGHT * scale) / 2.f;

    offsetX_ = 0.f;
    offsetY_ = 0.f;
}

sf::Vector2f Game_Render::worldToScreen(sf::Vector2f world) const {
    return sf::Vector2f(world.x * scaleX_ + offsetX_, world.y * scaleY_ + offsetY_);
}

sf::Vector2f Game_Render::screenToWorld(sf::Vector2f screen) const {
    float vpW = (FIELD_WIDTH * scaleX_) / viewWidth_;
    float vpH = (FIELD_HEIGHT * scaleY_) / viewHeight_;
    float vpX = (1.f - vpW) / 2.f;
    float vpY = (1.f - vpH) / 2.f;
    float nx = (screen.x / viewWidth_ - vpX) / vpW;
    float ny = (screen.y / viewHeight_ - vpY) / vpH;
    return sf::Vector2f(nx * FIELD_WIDTH, ny * FIELD_HEIGHT);
}

void Game_Render::drawField(sf::RenderWindow& window) {
    if (isFieldLoaded_) {
        // Nếu load ảnh thành công thì vẽ ảnh
        window.draw(fieldSprite_);
    }
    else {
        // Nền sân xanh
        sf::RectangleShape rect(sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT));
        rect.setPosition({ 0.f, 0.f });
        rect.setFillColor(sf::Color(34, 139, 34));
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(LINE_WIDTH);
        window.draw(rect);

        // Đường giữa sân
        sf::RectangleShape midLine(sf::Vector2f(LINE_WIDTH, FIELD_HEIGHT));
        midLine.setPosition({ FIELD_WIDTH / 2.f - LINE_WIDTH / 2.f, 0.f });
        midLine.setFillColor(sf::Color::White);
        window.draw(midLine);

        // Vòng tròn giữa sân
        float r = 80.f;
        sf::CircleShape centerCircle(r);
        centerCircle.setPosition({ FIELD_WIDTH / 2.f - r, FIELD_HEIGHT / 2.f - r });
        centerCircle.setFillColor(sf::Color::Transparent);
        centerCircle.setOutlineColor(sf::Color::White);
        centerCircle.setOutlineThickness(LINE_WIDTH);
        window.draw(centerCircle);

        // Chấm giữa sân
        sf::CircleShape centerDot(4.f);
        centerDot.setPosition({ FIELD_WIDTH / 2.f - 4.f, FIELD_HEIGHT / 2.f - 4.f });
        centerDot.setFillColor(sf::Color::White);
        window.draw(centerDot);

        // Khu cấm địa & khu thành - khung thành trái
        sf::RectangleShape penalty1(sf::Vector2f(PENALTY_AREA_DEPTH, FIELD_HEIGHT));
        penalty1.setPosition({ 0.f, 0.f });
        penalty1.setFillColor(sf::Color::Transparent);
        penalty1.setOutlineColor(sf::Color::White);
        penalty1.setOutlineThickness(LINE_WIDTH);
        window.draw(penalty1);
        sf::RectangleShape goalArea1(sf::Vector2f(GOAL_AREA_DEPTH, FIELD_HEIGHT));
        goalArea1.setPosition({ 0.f, 0.f });
        goalArea1.setFillColor(sf::Color::Transparent);
        goalArea1.setOutlineColor(sf::Color::White);
        goalArea1.setOutlineThickness(LINE_WIDTH);
        window.draw(goalArea1);

        // Khu cấm địa & khu thành - khung thành phải
        sf::RectangleShape penalty2(sf::Vector2f(PENALTY_AREA_DEPTH, FIELD_HEIGHT));
        penalty2.setPosition({ FIELD_WIDTH - PENALTY_AREA_DEPTH, 0.f });
        penalty2.setFillColor(sf::Color::Transparent);
        penalty2.setOutlineColor(sf::Color::White);
        penalty2.setOutlineThickness(LINE_WIDTH);
        window.draw(penalty2);
        sf::RectangleShape goalArea2(sf::Vector2f(GOAL_AREA_DEPTH, FIELD_HEIGHT));
        goalArea2.setPosition({ FIELD_WIDTH - GOAL_AREA_DEPTH, 0.f });
        goalArea2.setFillColor(sf::Color::Transparent);
        goalArea2.setOutlineColor(sf::Color::White);
        goalArea2.setOutlineThickness(LINE_WIDTH);
        window.draw(goalArea2);
    }
}

void Game_Render::drawGoals(sf::RenderWindow& window) {
    // Đường cầu môn (vạch khung thành) + cột dọc trắng
    const float postW = LINE_WIDTH;
    sf::RectangleShape leftGoal(sf::Vector2f(postW, GOAL_HEIGHT));
    leftGoal.setPosition({ FIELD_MARGIN_X, GOAL_Y_OFFSET});
    leftGoal.setFillColor(sf::Color::Red);
    window.draw(leftGoal);

    sf::RectangleShape rightGoal(sf::Vector2f(postW, GOAL_HEIGHT));
    rightGoal.setPosition({ FIELD_WIDTH - FIELD_MARGIN_X - postW - 2.f, GOAL_Y_OFFSET});
    rightGoal.setFillColor(sf::Color::Red);
    window.draw(rightGoal);
}

void Game_Render::drawBall(sf::RenderWindow& window) {
    if (!state_) return;
    const auto& ball = state_->getBall();

    if (isBallLoaded_) {
        // CÁCH 1: VẼ BẰNG ẢNH
        // Chỉ cần đặt vị trí (Sprite đã được chỉnh tâm ở Bước 3 nên đặt thẳng vào pos là khớp)
        ballSprite_.setPosition(ball.getPosition());

        // (Nâng cao) Nếu bạn muốn bóng xoay khi lăn, bạn cần thêm logic tính góc xoay
        // Nhưng tạm thời cứ vẽ tĩnh trước đã.

        // 2. --- THÊM ĐOẠN NÀY: TÍNH TOÁN XOAY ---
        sf::Vector2f v = ball.getVelocity(); // Lấy vận tốc hiện tại
        float speed = std::sqrt(v.x * v.x + v.y * v.y); // Tính tốc độ (độ dài vector)

        // Nếu bóng đang di chuyển (tốc độ > 0.1)
        if (speed > 5.0f) {
            // Tốc độ càng cao, currentBallFrame_ tăng càng nhanh
            // Hệ số 0.05f là độ nhạy (có thể tăng giảm tùy ý xem bóng lăn hợp lý chưa)
            currentBallFrame_ += speed * 0.01f;

            // Tổng số frame là 64 (từ 0 đến 63). Nếu vượt quá thì quay lại từ đầu.
            if (currentBallFrame_ >= 64.f) {
                currentBallFrame_ -= 64.f;
            }

            int currentFrameInt = static_cast<int>(currentBallFrame_);

            // Toán học cắt lưới:
            // Lấy Số Frame % 8 = Ra thứ tự Cột
            // Lấy Số Frame / 8 = Ra thứ tự Hàng
            int col = currentFrameInt % 8;
            int row = currentFrameInt / 8;

            // Tính tọa độ X, Y góc trên bên trái của ô đó trong ảnh gốc
            int rectX = col * ballFrameSize_.x;
            int rectY = row * ballFrameSize_.y;

            // Dịch chuyển khung cắt tới đúng ô đó
            ballSprite_.setTextureRect(sf::IntRect({ rectX, rectY }, { ballFrameSize_.x, ballFrameSize_.y }));
        }

        window.draw(ballSprite_);
    }
    else {
        // CÁCH 2: VẼ DỰ PHÒNG (NẾU QUÊN ẢNH) - Giữ lại code cũ
        sf::CircleShape circle(BALL_RADIUS);
        circle.setPosition({ ball.getPosition().x - BALL_RADIUS, ball.getPosition().y - BALL_RADIUS });
        circle.setFillColor(sf::Color::White);
        circle.setOutlineColor(sf::Color(100, 100, 100));
        circle.setOutlineThickness(1.f);
        window.draw(circle);
    }
}

void Game_Render::drawPieces(sf::RenderWindow& window) {
    if (!state_) return;

    // Lấy thời gian trôi qua (tính bằng giây)
    float time = effectClock_.getElapsedTime().asSeconds();

    // Hàm sin(time) sẽ tạo ra một sóng dao động lượn lên lượn xuống từ -1 đến 1.
    // Ta biến đổi một chút để nó dao động từ 0 đến 1 (dùng làm tỷ lệ).
    // Số 6.f là tốc độ nhấp nháy (bạn có thể tăng/giảm để chớp nhanh/chậm).
    float pulse = (std::sin(time * 6.f) + 1.f) / 2.f;

    Team currentTurn = state_->getCurrentTurn();

    for (const auto& p : state_->getPieces()) {
        sf::Vector2f pos = p->getPosition();
        Team team = p->getTeam();

        if (team == currentTurn && state_->isEverythingStopped()) {
            // Tạo hình tròn to hơn bán kính cầu thủ 4px, cộng thêm độ rung (pulse)
            float auraRadius = PIECE_RADIUS + 4.f + (pulse * 5.f);
            sf::CircleShape aura(auraRadius);

            // Đặt tâm hình tròn về giữa
            aura.setOrigin({ auraRadius, auraRadius });
            aura.setPosition(pos);

            // Cài màu sắc: Đội 1 hào quang Xanh lơ (Cyan), Đội 2 hào quang Vàng (Yellow)
            sf::Color auraColor;
            if (team == Team::Team1) auraColor = sf::Color(0, 255, 255); // Xanh Cyan giống trong ảnh
            else auraColor = sf::Color(255, 255, 0); // Vàng

            // Chỉnh độ lấp lánh (Alpha - Độ trong suốt) từ 100 đến 200 dựa theo nhịp thở
            auraColor.a = static_cast<std::uint8_t>(100 + pulse * 100);

            aura.setFillColor(auraColor);

            // BẮT BUỘC: Vẽ hào quang TRƯỚC để nó nằm lót dưới đáy quân cờ
            window.draw(aura);
        }

        // --- VẼ ĐỘI 1 (Nếu load ảnh thành công) ---
        if (team == Team::Team1 && isTeam1Loaded_) {
            team1Sprite_.setPosition(pos); // Đặt vị trí

            // (Tùy chọn) Xoay cầu thủ theo hướng di chuyển cho ngầu
            /*
            sf::Vector2f v = p->getVelocity();
            float speed = std::sqrt(v.x*v.x + v.y*v.y);
            if(speed > 0.1f) team1Sprite_.setRotation(sf::degrees(std::atan2(v.y, v.x) * 180.f / 3.14159f));
            */

            window.draw(team1Sprite_);
        }
        // --- VẼ ĐỘI 2 (Nếu load ảnh thành công) ---
        else if (team == Team::Team2 && isTeam2Loaded_) {
            team2Sprite_.setPosition(pos);
            window.draw(team2Sprite_);
        }
        // --- VẼ DỰ PHÒNG (Nếu chưa có ảnh hoặc lỗi load) ---
        else {
            sf::CircleShape circle(PIECE_RADIUS);
            circle.setPosition({ pos.x - PIECE_RADIUS, pos.y - PIECE_RADIUS });

            if (team == Team::Team1) circle.setFillColor(TEAM1_COLOR);
            else circle.setFillColor(TEAM2_COLOR);

            circle.setOutlineColor(sf::Color::Black);
            circle.setOutlineThickness(2.f);
            window.draw(circle);
        }
    }
    

}

// Hàm bổ trợ để chuyển giây thành định dạng MM:SS
std::string formatTime(float seconds) {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, secs);
    return std::string(buffer);
}

void Game_Render::drawUI(sf::RenderWindow& window) {
    if (!state_ || !isSbLoaded_) return;

    window.draw(iconReturnSprite_);

    // CHỈ VẼ DUY NHẤT CÁI BẢNG TỈ SỐ ĐỂ KIỂM TRA
    if (isSbLoaded_) {
        // Vẽ bảng tỉ số
        window.draw(sbSprite_);

    }

    sf::Vector2f sbCenter = sbSprite_.getPosition();

    // 2. Khởi tạo Text để vẽ
    sf::Text text(sbFont_, "", SB_FONT_SIZE_SCORE);
    text.setFillColor(sf::Color::White);

    // Lambda hàm để căn giữa chữ vào một điểm cụ thể
    auto centerTextAt = [&](sf::Text& t, sf::Vector2f pos) {
        sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin({ bounds.position.x + bounds.size.x / 2.f,
                      bounds.position.y + bounds.size.y / 2.f });
        t.setPosition(pos);
        };

    // --- VẼ TỈ SỐ ĐỘI 1 (Ô tròn đen bên trái) ---
    text.setString(std::to_string(state_->getScore1()));
    text.setCharacterSize(SB_FONT_SIZE_SCORE);
    centerTextAt(text, sbCenter + SB_OFFSET_SCORE1);
    window.draw(text);

    // --- VẼ TỈ SỐ ĐỘI 2 (Ô tròn đen bên phải) ---
    text.setString(std::to_string(state_->getScore2()));
    centerTextAt(text, sbCenter + SB_OFFSET_SCORE2);
    window.draw(text);

    // --- VẼ ĐẾM NGƯỢC ĐỊNH DẠNG 00:30 ---
    float timeLeft = state_->getTurnTimer();
    if (timeLeft < 0.f) timeLeft = 0.f; // Không để hiện số âm

    int minutes = static_cast<int>(timeLeft) / 60;
    int seconds = static_cast<int>(timeLeft) % 60;

    // Tạo chuỗi định dạng MM:SS
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);

    text.setString(buffer);
    text.setCharacterSize(SB_FONT_SIZE_TIME + 2);

    // Đổi màu cảnh báo khi dưới 5 giây
    if (timeLeft <= 5.f) {
        text.setFillColor(sf::Color::Red);

        // Hiệu ứng nhấp nháy nhẹ (tùy chọn)
        if (static_cast<int>(timeLeft * 5) % 2 == 0)
            text.setFillColor(sf::Color(255, 100, 100));
    }
    else {
        text.setFillColor(sf::Color::White);
    }
    centerTextAt(text, sbCenter + SB_OFFSET_TIME);
    window.draw(text);
}

//void Game_Render::drawUI(sf::RenderWindow& window) {
//    if (!state_) return;
//    sf::Font font;
//    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return;
//
//    std::string scoreStr = std::to_string(state_->getScore1()) + " - " + std::to_string(state_->getScore2());
//    sf::Text scoreText(font, scoreStr, 36);
//    scoreText.setPosition({ FIELD_WIDTH / 2.f - 40.f, 15.f });
//    scoreText.setFillColor(sf::Color::White);
//    scoreText.setOutlineColor(sf::Color::Black);
//    scoreText.setOutlineThickness(1.f);
//    window.draw(scoreText);
//
//    std::string turnStr = (state_->getCurrentTurn() == Team::Team1) ? "Luot: Doi 1" : "Luot: Doi 2";
//    sf::Text turnText(font, turnStr, 20);
//    turnText.setPosition({ 10.f, 10.f });
//    turnText.setFillColor(sf::Color::White);
//    window.draw(turnText);
//}

//void Game_Render::drawMenu(sf::RenderWindow& window) {
//    sf::Font font;
//    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return;
//    sf::Text title(font, "SOCCER POOL", 48);
//    title.setPosition({ FIELD_WIDTH / 2.f - 120.f, 80.f });
//    title.setFillColor(sf::Color::White);
//    window.draw(title);
//
//    sf::Text t1(font, "1. Nguoi vs Nguoi",  24);
//    t1.setPosition({ FIELD_WIDTH / 2.f - 100.f, 200.f });
//    t1.setFillColor(sf::Color::White);
//    window.draw(t1);
//    sf::Text t2(font, "2. Nguoi vs May (De)",  24);
//    t2.setPosition({ FIELD_WIDTH / 2.f - 100.f, 240.f });
//    t2.setFillColor(sf::Color::White);
//    window.draw(t2);
//    sf::Text t3(font, "3. Nguoi vs May (TB)",  24);
//    t3.setPosition({ FIELD_WIDTH / 2.f - 100.f, 280.f });
//    t3.setFillColor(sf::Color::White);
//    window.draw(t3);
//    sf::Text t4(font, "4. Nguoi vs May (Kho)",  24);
//    t4.setPosition({ FIELD_WIDTH / 2.f - 100.f, 320.f });
//    t4.setFillColor(sf::Color::White);
//    window.draw(t4);
//    sf::Text t5(font,"5. May vs May", 24);
//    t5.setPosition({ FIELD_WIDTH / 2.f - 100.f, 360.f });
//    t5.setFillColor(sf::Color::White);
//    window.draw(t5);
//}

void Game_Render::drawGameOver(sf::RenderWindow& window) {
    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return;
    std::string msg = (state_->getScore1() >= state_->getConfig().goalsToWin) ? "DOI 1 THANG!" : "DOI 2 THANG!";
    sf::Text over(font, msg,  42);
    over.setPosition({ FIELD_WIDTH / 2.f - 100.f, FIELD_HEIGHT / 2.f - 30.f });
    over.setFillColor(sf::Color::Yellow);
    window.draw(over);
    sf::Text hint(font, "Nhan SPACE de choi lai",  20);
    hint.setPosition({ FIELD_WIDTH / 2.f - 90.f, FIELD_HEIGHT / 2.f + 30.f });
    hint.setFillColor(sf::Color::White);
    window.draw(hint);
}

//void Game_Render::drawDragIndicator(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to) {
//    sf::Vertex line[] = {
//        sf::Vertex({from, sf::Color(255, 255, 0)}),
//        sf::Vertex({to, sf::Color(255, 200, 0)})
//    };
//    window.draw(line, 2, sf::PrimitiveType::Lines);
//}

void Game_Render::setDragState(sf::Vector2f from, sf::Vector2f to, sf::Vector2f piecePos, sf::Vector2f directionUnit) {
    dragActive_ = true;
    dragFrom_ = from;
    dragTo_ = to;
    dragPiecePos_ = piecePos;
    dragDir_ = directionUnit;
}

//void Game_Render::drawDirectionArrow(sf::RenderWindow& window, sf::Vector2f piecePos, sf::Vector2f directionUnit, float length) {
//    const float arrowHeadLen = 28.f;
//    const float arrowHeadW = 14.f;
//    sf::Vector2f tip = sf::Vector2f(piecePos.x + directionUnit.x * length, piecePos.y + directionUnit.y * length);
//    sf::Vertex line[] = {
//        sf::Vertex({piecePos, sf::Color(255, 255, 0)}),
//        sf::Vertex({tip, sf::Color(255, 220, 0)})
//    };
//    window.draw(line, 2, sf::PrimitiveType::Lines);
//    float perpX = -directionUnit.y, perpY = directionUnit.x;
//    sf::Vector2f back = sf::Vector2f(
//        tip.x - directionUnit.x * arrowHeadLen + perpX * arrowHeadW,
//        tip.y - directionUnit.y * arrowHeadLen + perpY * arrowHeadW
//    );
//    sf::Vector2f back2 = sf::Vector2f(
//        tip.x - directionUnit.x * arrowHeadLen - perpX * arrowHeadW,
//        tip.y - directionUnit.y * arrowHeadLen - perpY * arrowHeadW
//    );
//    sf::Vertex head[] = {
//        sf::Vertex({tip, sf::Color(255, 220, 0)}),
//        sf::Vertex({back, sf::Color(255, 220, 0)}),
//        sf::Vertex({back2, sf::Color(255, 220, 0)})
//    };
//    window.draw(head, 3, sf::PrimitiveType::Triangles);
//}

// Cài đặt hàm vẽ đường ngắm sút
void Game_Render::drawShotAiming(sf::RenderWindow& window) {
    if (!dragActive_) return;

    // 1. TÍNH TOÁN CÁC THÔNG SỐ CƠ BẢN
    // dragDir_ là vector hướng sút (từ chuột hướng về phía cầu thủ)
    float len = std::sqrt(dragDir_.x * dragDir_.x + dragDir_.y * dragDir_.y);
    if (len <= 0.1f) return; // Kéo quá nhẹ thì không vẽ

    sf::Vector2f dirUnit = dragDir_ / len; // Vector đơn vị (chỉ hướng, độ dài = 1)

    // Giới hạn độ dài hiển thị (Vòng tròn max lực). Bạn có thể chỉnh số 90.f to nhỏ tùy ý
    float currentLength = std::min(len, MAX_VISUAL_DRAG); // Chiều dài thực tế bị kẹp bởi max

    // 2. VẼ VÒNG TRÒN GIỚI HẠN (MAX POWER CIRCLE)
    sf::CircleShape maxCircle(currentLength);
    maxCircle.setOrigin({ currentLength, currentLength }); // Đặt tâm vào giữa
    maxCircle.setPosition(dragPiecePos_);
    maxCircle.setFillColor(sf::Color::Transparent); // Rỗng ruột
    maxCircle.setOutlineColor(sf::Color(255, 255, 255, 80)); // Viền trắng mờ (Alpha = 80)
    maxCircle.setOutlineThickness(2.f);
    window.draw(maxCircle);

    // Tính góc quay (để xoay mũi tên)
    float angleRad = std::atan2(dirUnit.y, dirUnit.x);
    float angleDeg = angleRad * 180.f / 3.14159f;

    // 3. VẼ MŨI TÊN CAM (HƯỚNG SÚT)
    // 3.1 Thân mũi tên (Dùng hình chữ nhật thay vì đường thẳng để nét vẽ được dày)
    sf::RectangleShape arrowShaft(sf::Vector2f(currentLength, 6.f)); // Độ dày 6px
    arrowShaft.setOrigin({ 0.f, 3.f }); // Tâm nằm ở giữa gốc thân
    arrowShaft.setPosition(dragPiecePos_);
    arrowShaft.setFillColor(sf::Color(255, 140, 0)); // Màu cam (Orange)
    arrowShaft.setRotation(sf::degrees(angleDeg)); // Xoay bằng sf::degrees cho SFML 3.0
    window.draw(arrowShaft);

    // 3.2 Đầu mũi tên (Hình tam giác)
    sf::ConvexShape arrowHead(3);
    arrowHead.setPoint(0, sf::Vector2f(0.f, -12.f)); // Đỉnh trên
    arrowHead.setPoint(1, sf::Vector2f(20.f, 0.f));  // Mũi nhọn hướng về trước
    arrowHead.setPoint(2, sf::Vector2f(0.f, 12.f));  // Đỉnh dưới
    arrowHead.setFillColor(sf::Color(255, 140, 0));

    // Đặt vị trí đầu mũi tên ở ngay đầu thân mũi tên
    sf::Vector2f tipPos = dragPiecePos_ + dirUnit * currentLength;
    arrowHead.setPosition(tipPos);
    arrowHead.setRotation(sf::degrees(angleDeg));
    window.draw(arrowHead);

    // 4. VẼ ĐƯỜNG CHẤM ĐEN (HƯỚNG CHUỘT KÉO LÙI)
    // Đường này ngược hướng với hướng sút, tức là -dirUnit
    float dotSpacing = 16.f; // Khoảng cách giữa các chấm
    int numDots = static_cast<int>(currentLength / dotSpacing); // Càng kéo xa càng nhiều chấm

    sf::CircleShape dot(3.5f); // Bán kính chấm tròn
    dot.setFillColor(sf::Color(30, 30, 30, 180)); // Màu đen hơi trong suốt
    dot.setOrigin({ 3.5f, 3.5f });

    for (int i = 1; i <= numDots; ++i) {
        // Vị trí chấm = Tâm cầu thủ - (Hướng sút * Khoảng cách)
        sf::Vector2f dotPos = dragPiecePos_ - dirUnit * (static_cast<float>(i) * dotSpacing);
        dot.setPosition(dotPos);
        window.draw(dot);
    }
}



void Game_Render::drawMainMenu(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);

    // Vẽ họa tiết trang trí
    //ball8Sprite_.setPosition({ 150.f, 150.f });
    window.draw(ball8Sprite_);
    //goalMenuSprite_.setPosition({ 750.f, 400.f });
    window.draw(goalMenuSprite_);

	window.draw(ballMenuSprite_);

    window.draw(btnPlaySprite_);

    window.draw(btnOptionsSprite_);

	window.draw(iconQuitSprite_);

	window.draw(iconInforSprite_);

    sf::Font titleFont;
    if (!titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) { // Đường dẫn font mới
        std::cerr << "LOI: Khong load duoc font tieu de!" << std::endl;
        // Nếu lỗi, hiện text tạm bằng font cơ bản để game không crash
        titleFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    }

    // 2. Tạo đối tượng Text
    sf::Text titleText(titleFont);
    titleText.setString("SOCCER POOL");

    // 3. Chỉnh kích thước (Trong ảnh chữ rất to, khoảng 70-80)
    titleText.setCharacterSize(80);

    // 4. Chỉnh màu sắc (Trong ảnh là màu đen tuyền)
    titleText.setFillColor(sf::Color::Black);

    // 5. Căn giữa tiêu đề vào chính giữa sân
    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                          textBounds.position.y + textBounds.size.y / 2.f });

    // Đặt ở tọa độ (500, 100), cao hơn một chút để không bị 8-ball đè
    titleText.setPosition({ FIELD_WIDTH / 2.f, 100.f });

    // 6. Vẽ
    window.draw(titleText);

}


void Game_Render::drawSelectMode(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);

    sf::Font titleFont;
    if (!titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) { // Đường dẫn font mới
        std::cerr << "LOI: Khong load duoc font tieu de!" << std::endl;
        // Nếu lỗi, hiện text tạm bằng font cơ bản để game không crash
        titleFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    }

    sf::Text titleText(titleFont);
    titleText.setString("GAME MODE");

    titleText.setCharacterSize(80);

    titleText.setFillColor(sf::Color::Black);

    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                          textBounds.position.y + textBounds.size.y / 2.f });

    titleText.setPosition({ FIELD_WIDTH / 2.f, 100.f });

    window.draw(titleText);

	window.draw(pvpSprite_);
	window.draw(pvaiSprite_);
	window.draw(aivaiSprite_);

    //// --- BƯỚC 1: THIẾT LẬP LẠI VIEW ĐỂ DÙNG TỌA ĐỘ THẾ GIỚI (1000x600) ---
    //sf::View worldView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
    //window.setView(worldView);

    //// --- BƯỚC 2: KHỞI TẠO VÀ VẼ CHẤM ĐỎ ---
    //sf::CircleShape debugDot(5.f); // Bán kính 5px cho dễ nhìn
    //debugDot.setFillColor(sf::Color::Red);
    //debugDot.setOrigin({ 5.f, 5.f }); // Đặt tâm vào chính giữa chấm
    //debugDot.setPosition({ 490.f - 130.f, 200.f - 3.f + 95 }); // Tọa độ Hiếu yêu cầu

    //window.draw(debugDot);

    // Nút quay lại (iconQuit đã load)
    window.draw(iconReturnSprite_);
}


void Game_Render::drawSelectLineup(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);

    sf::Font titleFont;
    if (!titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) { // Đường dẫn font mới
        std::cerr << "LOI: Khong load duoc font tieu de!" << std::endl;
        // Nếu lỗi, hiện text tạm bằng font cơ bản để game không crash
        titleFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    }

    sf::Text titleText(titleFont);
    titleText.setString("Pick Team " + std::to_string(pickingForTeam_) + " Lineup");

    titleText.setCharacterSize(80);

    titleText.setFillColor(sf::Color::Black);

    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                          textBounds.position.y + textBounds.size.y / 2.f });

    titleText.setPosition({ FIELD_WIDTH / 2.f, 70.f });

    window.draw(titleText);

    //sf::Text title(menuFont_, "Pick Team " + std::to_string(pickingForTeam_) + " Lineup", 50);
    //title.setPosition({ 300.f, 50.f });
    //window.draw(title);

    // Vẽ 3 đội hình dựa trên currentLineupPage_
    int startIdx = currentLineupPage_ * 2;
    for (int i = 0; i < 2 && (startIdx + i) < lineups_.size(); ++i) {
        auto& lineup = lineups_[startIdx + i];
        //lineup->sprite.setScale({ 0.5f, 0.5f }); // Chỉnh kích thước cho vừa
        lineup->sprite.setPosition({ 330.f + i * 345.f, 240.f });
        window.draw(lineup->sprite);

        // --- ĐOẠN VẼ VIỀN XÁC NHẬN ---
        if (lineup->id == selectedLineupId_) {
            sf::RectangleShape highlight({ 200.f, 220.f }); // Kích thước khớp cardRect
            highlight.setPosition({ 230.f + i * 345.f, 130.f });
            highlight.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
            highlight.setOutlineColor(sf::Color::Yellow);    // Viền màu vàng rực rỡ
            highlight.setOutlineThickness(5.f);             // Độ dày viền 5px

            window.draw(highlight);
        }

        // Vẽ tên hoặc chỉ số dưới ảnh
        sf::Font titleFont;
        if (!titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) { // Đường dẫn font mới
            std::cerr << "LOI: Khong load duoc font tieu de!" << std::endl;
            // Nếu lỗi, hiện text tạm bằng font cơ bản để game không crash
            titleFont.openFromFile("C:/Windows/Fonts/arial.ttf");
        }

        sf::Text titleText(titleFont);
        titleText.setString(lineup->name);
        titleText.setCharacterSize(30);
        titleText.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = titleText.getLocalBounds();
        titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                              textBounds.position.y + textBounds.size.y / 2.f });
        titleText.setPosition({ 335.f + i * 350, 380.f });
        window.draw(titleText);
        //window.draw(name);
    }

    // Vẽ mũi tên chuyển trang và nút Start
    window.draw(arrowLeftSprite_); // Vị trí trái
    window.draw(arrowRightSprite_); // Vị trí phải

    window.draw(iconReturnSprite_);

    if (pickingForTeam_ == 1 && state_->getConfig().mode != GameMode::PvAI) {
        // Nếu là đội 1 và còn phải chọn tiếp đội 2
        window.draw(nextBtnSprite_);
    }
    else {
        // Nếu là đội 2 hoặc chơi với máy (chọn xong là vào luôn)
        window.draw(startBtnSprite_);
    }

    // 2. Tạo hình chữ nhật để vẽ
    sf::RectangleShape debugRect(sf::Vector2f(160.f, 50.f));
    debugRect.setPosition({ 420.f, 425.f });

    //// 3. Thiết lập hiển thị (Chỉ vẽ viền để không che ảnh đội hình)
    debugRect.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
    debugRect.setOutlineColor(sf::Color::Red);       // Viền đỏ cho nổi bật
    debugRect.setOutlineThickness(2.f);              // Độ dày viền 2px

    //// 4. Vẽ lên cửa sổ
    window.draw(debugRect);
}


void Game_Render::draw(sf::RenderWindow& window) {
    if (!state_) return;
    updateTransform();

    if (state_->getPhase() == GamePhase::Menu) {
        sf::View menuView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(menuView);
        drawMainMenu(window);
        return;
    }
    else if (state_->getPhase() == GamePhase::Setup) {
        sf::View setupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(setupView);
        drawSelectMode(window);
        return;
    }
    else if (state_->getPhase() == GamePhase::PickLineup) {
        sf::View lineupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(lineupView);
        drawSelectLineup(window);
        return;
    }

    sf::View gameView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
    float vpW = (FIELD_WIDTH * scaleX_) / viewWidth_;
    float vpH = (FIELD_HEIGHT * scaleY_) / viewHeight_;
    float vpX = (1.f - vpW) / 2.f;
    float vpY = (1.f - vpH) / 2.f;
    gameView.setViewport(sf::FloatRect({ vpX, vpY }, { vpW, vpH }));
    window.setView(gameView);

    drawField(window);
    drawGoals(window);
    drawBall(window);
    drawPieces(window);
    if (dragActive_) {
        //drawDragIndicator(window, dragFrom_, dragTo_);
        //float len = std::sqrt(dragDir_.x * dragDir_.x + dragDir_.y * dragDir_.y);
        //
            //ƯdrawDirectionArrow(window, dragPiecePos_, sf::Vector2f(dragDir_.x / len, dragDir_.y / len), 55.f);
        drawShotAiming(window);
        dragActive_ = false;
    }

    

    window.setView(window.getDefaultView());

    if (state_->getPhase() == GamePhase::GameOver)
        drawGameOver(window);
    else
        drawUI(window);
}

} // namespace SoccerPool
