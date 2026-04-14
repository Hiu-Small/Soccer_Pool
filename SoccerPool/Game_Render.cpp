#include "Game_Render.h"
#include "GameState.h"
#include "Constants.h"
#include <cmath>
#include <string>
#include <iostream> // Thêm thư viện này để báo lỗi nếu không tìm thấy ảnh

namespace SoccerPool {

    Game_Render::Game_Render() : fieldSprite_(fieldTexture_), ballSprite_(ballTexture_), team1Sprite_(team1Texture_), team2Sprite_(team2Texture_), sbSprite_(sbTexture_), menuBgSprite_(menuBgTexture_), ball8Sprite_(ball8Texture_), goalMenuSprite_(goalMenuTexture_), ballMenuSprite_(ballMenuTexture_), btnPlaySprite_(btnPlayTexture_), btnOptionsSprite_(btnOptionsTexture_), iconQuitSprite_(iconQuitTexture_), iconInforSprite_(iconInforTexture_), iconReturnSprite_(iconReturnTexture_), pvpSprite_(pvpTexture_), pvaiSprite_(pvaiTexture_), aivaiSprite_(aivaiTexture_), arrowLeftSprite_(arrowLeftTexture_), arrowRightSprite_(arrowRightTexture_), startBtnSprite_(startBtnTexture_), nextBtnSprite_(nextBtnTexture_), msbQuitSprite_(msbQuitTexture_), slideBarSoundSprite_(slideBarSoundTexture_), slideNodeSoundSprite_(slideNodeSoundTexture_), slideBarSFXSprite_(slideBarSFXTexture_), slideNodeSFXSprite_(slideNodeSFXTexture_), optionsKhungSprite_(optionsKhungTexture_), bgMusic_(), hitSoundKick_(hitBufferKick_), hitSoundCollide_(hitBufferCollide_), whistleSound_(whistleBuffer_), goalScoreSound_(goalScoreBuffer_), goalText_(goalFont_), goalMusicSound_(goalMusicBuffer_), iconOptionsSprite_(iconOptionsTexture_), 
        argSprite_(argTexture_),
        braSprite_(braTexture_),
        fraSprite_(fraTexture_),
        gerSprite_(gerTexture_),
        nedSprite_(nedTexture_),
        espSprite_(espTexture_),
        engSprite_(engTexture_),
        usaSprite_(usaTexture_),
        uruSprite_(uruTexture_),
        sweSprite_(sweTexture_),
        rusSprite_(rusTexture_),
        korSprite_(korTexture_),
        jpnSprite_(jpnTexture_),
        irnSprite_(irnTexture_),
        colSprite_(colTexture_),
        chnSprite_(chnTexture_),
        chiSprite_(chiTexture_),
        vieSprite_(vieTexture_),
        porSprite_(porTexture_),
        itaSprite_(itaTexture_) {
    // 1. Cố gắng load file ảnh
    // Đảm bảo bạn đã tạo thư mục 'assets' và bỏ file ảnh vào đó!
    if (!fieldTexture_.loadFromFile("assets/field_5.png")) {
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

    //// --- 3. LOAD ẢNH ĐỘI 1 ---
    //if (!team1Texture_.loadFromFile("assets/team1.png")) {
    //    std::cerr << "LOI: Khong load duoc assets/team1.png" << std::endl;
    //    isTeam1Loaded_ = false;
    //}
    //else {
    //    isTeam1Loaded_ = true;
    //    team1Texture_.setSmooth(true);
    //    sf::Vector2u t1Size = team1Texture_.getSize();

    //    // Cập nhật vùng hiển thị (Bắt buộc ở SFML 3)
    //    team1Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t1Size.x), static_cast<int>(t1Size.y) }));

    //    // Đặt tâm về giữa ảnh
    //    team1Sprite_.setOrigin({ static_cast<float>(t1Size.x) / 2.f, static_cast<float>(t1Size.y) / 2.f });

    //    // Tính tỷ lệ scale (Đường kính = 2 * PIECE_RADIUS)
    //    float scale = (2.f * PIECE_RADIUS) / static_cast<float>(t1Size.x);
    //    // (Giả sử ảnh vuông nên scale X Y bằng nhau)
    //    team1Sprite_.setScale({ scale, scale });
    //}

    //// --- 4. LOAD ẢNH ĐỘI 2 ---
    //if (!team2Texture_.loadFromFile("assets/team2.png")) {
    //    std::cerr << "LOI: Khong load duoc assets/team2.png" << std::endl;
    //    isTeam2Loaded_ = false;
    //}
    //else {
    //    isTeam2Loaded_ = true;
    //    team2Texture_.setSmooth(true);
    //    sf::Vector2u t2Size = team2Texture_.getSize();

    //    // Cập nhật vùng hiển thị
    //    team2Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t2Size.x), static_cast<int>(t2Size.y) }));

    //    // Đặt tâm về giữa ảnh
    //    team2Sprite_.setOrigin({ static_cast<float>(t2Size.x) / 2.f, static_cast<float>(t2Size.y) / 2.f });

    //    // Tính tỷ lệ scale
    //    float scale = (2.f * PIECE_RADIUS) / static_cast<float>(t2Size.x);
    //    team2Sprite_.setScale({ scale, scale });
    //}

    // --- LOAD ẢNH BẢNG TỈ SỐ (image_5.png) ---
    // Đảm bảo bạn đã copy ảnh image_5.png vào thư mục assets/ và đổi tên thành ti_so_xoa_ti_so.png
    // --- LOAD ẢNH BẢNG TỈ SỐ ---
    if (!sbTexture_.loadFromFile("assets/bang_ti_so_xoa_phong.png")) {
        std::cerr << "LOI: Khong load duoc assets/bang_ti_so_xoa_phong.png" << std::endl;
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

    if (!goalFont_.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) {
        std::cerr << "LOI: Khong load duoc font GulfsDisplay!" << std::endl;
    }



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
    if (!msbQuitTexture_.loadFromFile("assets/msb_quit.png")) {
        std::cerr << "Khong the load file: assets/msb_quit.png" << std::endl;
    }
    if (!slideBarSoundTexture_.loadFromFile("assets/slidebar.png")) {
        std::cerr << "Khong the load file: assets/slide_bar.png" << std::endl;
	}
    if (!slideNodeSoundTexture_.loadFromFile("assets/slidenode.png")) {
        std::cerr << "Khong the load file: assets/slide_node.png" << std::endl;
    }
    if (!slideBarSFXTexture_.loadFromFile("assets/slidebar.png")) {
        std::cerr << "Khong the load file: assets/slide_bar.png" << std::endl;
    }
    if (!slideNodeSFXTexture_.loadFromFile("assets/slidenode.png")) {
        std::cerr << "Khong the load file: assets/slide_node.png" << std::endl;
	}
    if (!optionsKhungTexture_.loadFromFile("assets/options_khung.png")) {
        std::cerr << "Khong the load file: assets/options_khung.png" << std::endl;
	}
    if (!iconOptionsTexture_.loadFromFile("assets/icon_options.png")) {
        std::cerr << "Khong the load file: assets/icon_options.png" << std::endl;
	}

    if (!argTexture_.loadFromFile("assets/team/argentina.png")) {
        std::cerr << "Khong the load file: assets/team/argentina.png" << std::endl;
	}
    if (!braTexture_.loadFromFile("assets/team/brazil.png")) {
        std::cerr << "Khong the load file: assets/team/brazil.png" << std::endl;
    }
    if (!fraTexture_.loadFromFile("assets/team/france.png")) {
        std::cerr << "Khong the load file: assets/team/france.png" << std::endl;
	}
    if (!gerTexture_.loadFromFile("assets/team/germany.png")) {
        std::cerr << "Khong the load file: assets/team/germany.png" << std::endl;
    }
    if (!nedTexture_.loadFromFile("assets/team/netherlands.png")) {
        std::cerr << "Khong the load file: assets/team/netherlands.png" << std::endl;
	}
    if (!espTexture_.loadFromFile("assets/team/spain.png")) {
        std::cerr << "Khong the load file: assets/team/spain.png" << std::endl;
	}
    if (!engTexture_.loadFromFile("assets/team/england.png")) {
        std::cerr << "Khong the load file: assets/team/england.png" << std::endl;
	}
    if (!usaTexture_.loadFromFile("assets/team/usa.png")) {
        std::cerr << "Khong the load file: assets/team/usa.png" << std::endl;
    }
    if (!uruTexture_.loadFromFile("assets/team/uruguay.png")) {
        std::cerr << "Khong the load file: assets/team/uruguay.png" << std::endl;
	}
    if (!sweTexture_.loadFromFile("assets/team/sweden.png")) {
        std::cerr << "Khong the load file: assets/team/sweden.png" << std::endl;
	}
    if (!rusTexture_.loadFromFile("assets/team/russia.png")) {
        std::cerr << "Khong the load file: assets/team/russia.png" << std::endl;
    }
    if (!korTexture_.loadFromFile("assets/team/korea.png")) {
        std::cerr << "Khong the load file: assets/team/korea.png" << std::endl;
    }
    if (!jpnTexture_.loadFromFile("assets/team/japan.png")) {
        std::cerr << "Khong the load file: assets/team/japan.png" << std::endl;
	}
    if (!irnTexture_.loadFromFile("assets/team/iran.png")) {
        std::cerr << "Khong the load file: assets/team/iran.png" << std::endl;
	}
    if (!colTexture_.loadFromFile("assets/team/colombia.png")) {
        std::cerr << "Khong the load file: assets/team/colombia.png" << std::endl;
    }
    if (!chnTexture_.loadFromFile("assets/team/china.png")) {
        std::cerr << "Khong the load file: assets/team/china.png" << std::endl;
	}
    if (!chiTexture_.loadFromFile("assets/team/chile.png")) {
        std::cerr << "Khong the load file: assets/team/chile.png" << std::endl;
    }
    if (!vieTexture_.loadFromFile("assets/team/vietnam.png")) {
        std::cerr << "Khong the load file: assets/team/vietnam.png" << std::endl;
    }
    if (!porTexture_.loadFromFile("assets/team/portugal.png")) {
        std::cerr << "Khong the load file: assets/team/portugal.png" << std::endl;
    }
    if (!itaTexture_.loadFromFile("assets/team/italy.png")) {
        std::cerr << "Khong the load file: assets/team/italy.png" << std::endl;
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
    menuBgTexture_.setSmooth(true);
    ball8Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)ball8Texture_.getSize().x, (int)ball8Texture_.getSize().y }));
    ball8Texture_.setSmooth(true);
    goalMenuSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)goalMenuTexture_.getSize().x, (int)goalMenuTexture_.getSize().y }));
    goalMenuTexture_.setSmooth(true);
    ballMenuSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)ballMenuTexture_.getSize().x, (int)ballMenuTexture_.getSize().y }));
    ballMenuTexture_.setSmooth(true);
	btnOptionsSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)btnOptionsTexture_.getSize().x, (int)btnOptionsTexture_.getSize().y }));
    btnOptionsTexture_.setSmooth(true);
	btnPlaySprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)btnPlayTexture_.getSize().x, (int)btnPlayTexture_.getSize().y }));
    btnPlayTexture_.setSmooth(true);
	iconInforSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconInforTexture_.getSize().x, (int)iconInforTexture_.getSize().y }));
    iconInforTexture_.setSmooth(true);
	iconQuitSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconQuitTexture_.getSize().x, (int)iconQuitTexture_.getSize().y }));
    iconQuitTexture_.setSmooth(true);
	iconReturnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconReturnTexture_.getSize().x, (int)iconReturnTexture_.getSize().y }));
    iconReturnTexture_.setSmooth(true);
	pvpSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)pvpTexture_.getSize().x, (int)pvpTexture_.getSize().y }));
    pvpTexture_.setSmooth(true);
	pvaiSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)pvaiTexture_.getSize().x, (int)pvaiTexture_.getSize().y }));
    pvaiTexture_.setSmooth(true);
	aivaiSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)aivaiTexture_.getSize().x, (int)aivaiTexture_.getSize().y }));
    aivaiTexture_.setSmooth(true);
	arrowLeftSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)arrowLeftTexture_.getSize().x, (int)arrowLeftTexture_.getSize().y }));
    arrowLeftTexture_.setSmooth(true);
	arrowRightSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)arrowRightTexture_.getSize().x, (int)arrowRightTexture_.getSize().y }));
    arrowRightTexture_.setSmooth(true);
	startBtnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)startBtnTexture_.getSize().x, (int)startBtnTexture_.getSize().y }));
    startBtnTexture_.setSmooth(true);
	nextBtnSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)nextBtnTexture_.getSize().x, (int)nextBtnTexture_.getSize().y }));
    nextBtnTexture_.setSmooth(true);
	msbQuitSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)msbQuitTexture_.getSize().x, (int)msbQuitTexture_.getSize().y }));
	msbQuitTexture_.setSmooth(true);
	slideBarSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideBarSoundTexture_.getSize().x, (int)slideBarSoundTexture_.getSize().y }));
	slideBarSoundTexture_.setSmooth(true);
	slideNodeSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideNodeSoundTexture_.getSize().x, (int)slideNodeSoundTexture_.getSize().y }));
	slideNodeSoundTexture_.setSmooth(true);
	slideBarSFXSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideBarSFXTexture_.getSize().x, (int)slideBarSFXTexture_.getSize().y }));
	slideBarSFXTexture_.setSmooth(true);
	slideNodeSFXSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideNodeSFXTexture_.getSize().x, (int)slideNodeSFXTexture_.getSize().y }));
	slideNodeSFXTexture_.setSmooth(true);
	optionsKhungSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)optionsKhungTexture_.getSize().x, (int)optionsKhungTexture_.getSize().y }));
	optionsKhungTexture_.setSmooth(true);
	iconOptionsSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)iconOptionsTexture_.getSize().x, (int)iconOptionsTexture_.getSize().y }));
	iconOptionsTexture_.setSmooth(true);

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



    for (int i = 0; i < 8; ++i) {
        auto opt = std::make_shared<LineupOption>();
        // 2. Gán dữ liệu (Dùng dấu -> thay vì dấu .)
        /*opt->id = (i == 0) ? 5 : (10 + i)*/;
		opt->id = i; 

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
            opt->name = "1-3-1";
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
        else if(i == 7) {
            opt->name = "2-2-1";
		}

        std::string path = "assets/lineup/" + std::to_string(i) + ".png";
        if (!opt->texture->loadFromFile(path)) {
            std::cerr << "LOI: Khong load duoc " << path << std::endl;
        }

        //opt->sprite.setTexture(*opt->texture);
        opt->sprite.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(opt->texture->getSize().x), static_cast<int>(opt->texture->getSize().y) }));
        opt->texture->setSmooth(true);
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

	sf::Vector2u msbQuitSize = msbQuitTexture_.getSize();
	float targetMsbQuitWidth = 1000.f;
	float sMsbQuit = targetMsbQuitWidth / static_cast<float>(msbQuitSize.x);
	msbQuitSprite_.setScale({ sMsbQuit, sMsbQuit });
	msbQuitSprite_.setOrigin({ msbQuitSize.x / 2.f, msbQuitSize.y / 2.f });
	msbQuitSprite_.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f });

	//sf::Vector2u slideBarSize = slideBarTexture_.getSize();
	//float targetSlideBarWidth = 270.f;
	//float sSlideBar = targetSlideBarWidth / static_cast<float>(slideBarSize.x);
	//slideBarSprite_.setScale({ sSlideBar, sSlideBar });
	//slideBarSprite_.setOrigin({ slideBarSize.x / 2.f, slideBarSize.y / 2.f });
	//slideBarSprite_.setPosition({ FIELD_WIDTH / 2.f + 10.f, FIELD_HEIGHT / 2.f - 10.f });

	//sf::Vector2u slideNodeSize = slideNodeTexture_.getSize();
	//float targetSlideNodeWidth = 140.f;
	//float sSlideNode = targetSlideNodeWidth / static_cast<float>(slideNodeSize.x);
	//slideNodeSprite_.setScale({ sSlideNode, sSlideNode });
	//slideNodeSprite_.setOrigin({ slideNodeSize.x / 2.f, slideNodeSize.y / 2.f });
	//slideNodeSprite_.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f - 10.f });

	sf::Vector2u slideBarSoundSize = slideBarSoundTexture_.getSize();
	float targetSlideBarSoundWidth = 270.f;
	float sSlideBarSound = targetSlideBarSoundWidth / static_cast<float>(slideBarSoundSize.x);
	slideBarSoundSprite_.setScale({ sSlideBarSound, sSlideBarSound });
	slideBarSoundSprite_.setOrigin({ slideBarSoundSize.x / 2.f, slideBarSoundSize.y / 2.f });
	slideBarSoundSprite_.setPosition({ FIELD_WIDTH / 2.f + 10.f, FIELD_HEIGHT / 2.f - 10.f });

	sf::Vector2u slideNodeSoundSize = slideNodeSoundTexture_.getSize();
	float targetSlideNodeSoundWidth = 140.f;
	float sSlideNodeSound = targetSlideNodeSoundWidth / static_cast<float>(slideNodeSoundSize.x);
	slideNodeSoundSprite_.setScale({ sSlideNodeSound, sSlideNodeSound });
	slideNodeSoundSprite_.setOrigin({ slideNodeSoundSize.x / 2.f, slideNodeSoundSize.y / 2.f });
	slideNodeSoundSprite_.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f - 10.f });

	sf::Vector2u slideBarSFXSize = slideBarSFXTexture_.getSize();
	float targetSlideBarSFXWidth = 270.f;
	float sSlideBarSFX = targetSlideBarSFXWidth / static_cast<float>(slideBarSFXSize.x);
	slideBarSFXSprite_.setScale({ sSlideBarSFX, sSlideBarSFX });
	slideBarSFXSprite_.setOrigin({ slideBarSFXSize.x / 2.f, slideBarSFXSize.y / 2.f });
	slideBarSFXSprite_.setPosition({ FIELD_WIDTH / 2.f + 10.f, FIELD_HEIGHT / 2.f + 80.f });

	sf::Vector2u slideNodeSFXSize = slideNodeSFXTexture_.getSize();
	float targetSlideNodeSFXWidth = 140.f;
	float sSlideNodeSFX = targetSlideNodeSFXWidth / static_cast<float>(slideNodeSFXSize.x);
	slideNodeSFXSprite_.setScale({ sSlideNodeSFX, sSlideNodeSFX });
	slideNodeSFXSprite_.setOrigin({ slideNodeSFXSize.x / 2.f, slideNodeSFXSize.y / 2.f });
	slideNodeSFXSprite_.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 80.f });

	sf::Vector2u optionsKhungSize = optionsKhungTexture_.getSize();
	float targetOptionsKhungWidth = 1000.f;
	float sOptionsKhung = targetOptionsKhungWidth / static_cast<float>(optionsKhungSize.x);
	optionsKhungSprite_.setScale({ sOptionsKhung, sOptionsKhung });
	optionsKhungSprite_.setOrigin({ optionsKhungSize.x / 2.f, optionsKhungSize.y / 2.f });
	optionsKhungSprite_.setPosition({ FIELD_WIDTH / 2.f + 25.f, FIELD_HEIGHT / 2.f });

	sf::Vector2u icOptionsSize = iconOptionsTexture_.getSize();
	float targetIconOptionsWidth = 240.f;
	float sIconOptions = targetIconOptionsWidth / static_cast<float>(icOptionsSize.x);
	iconOptionsSprite_.setScale({ sIconOptions, sIconOptions });
	iconOptionsSprite_.setOrigin({ icOptionsSize.x / 2.f, icOptionsSize.y / 2.f });
	iconOptionsSprite_.setPosition({ 950.f, 50.f });

    updateTransform();

    // 1. Load nhạc nền
    if (bgMusic_.openFromFile("assets/music/glory_fix.mp3")) {
        bgMusic_.setLooping(true);          // Lặp lại liên tục
        bgMusic_.setVolume(soundVolume_ * 100.f); // Chuyển từ tỉ lệ 0-1 sang 0-100
        bgMusic_.play();                 // Phát nhạc ngay khi mở game
    }

    // 2. Load hiệu ứng âm thanh (SFX)
    if (hitBufferKick_.loadFromFile("assets/music/tieng_sut.mp3")) {
        hitSoundKick_.setBuffer(hitBufferKick_);
        hitSoundKick_.setVolume(sfxVolume_ * 100.f);
    }

	// 3. Load hiệu ứng âm thanh va cham bong voi cau thu va tuong (SFX)
    if (hitBufferCollide_.loadFromFile("assets/music/tieng_va_cham.mp3")) {
        hitSoundCollide_.setBuffer(hitBufferCollide_);
        hitSoundCollide_.setVolume(sfxVolume_ * 100.f);
	}

	// 4. Load hiệu ứng tiếng còi (SFX)
    if (whistleBuffer_.loadFromFile("assets/music/tieng_coi.mp3")) {
        whistleSound_.setBuffer(whistleBuffer_);
        whistleSound_.setVolume(sfxVolume_ * 100.f);
    }

	//5. Load hiệu ứng tiếng ăn bàn (SFX)
    if (goalScoreBuffer_.loadFromFile("assets/music/goal_effect (mp3cut.net).mp3")) {
        goalScoreSound_.setBuffer(goalScoreBuffer_);
        goalScoreSound_.setVolume(sfxVolume_ * 100.f);
	}

	//6. Load hiệu ứng tiếng sân
    if (stadiumEffect.openFromFile("assets/music/stadium_effect.mp3")) {
        stadiumEffect.setLooping(true);          // Lặp lại liên tục
        stadiumEffect.setVolume(soundVolume_ * 100.f); // Chuyển từ tỉ lệ 0-1 sang 0-100
        stadiumEffect.play();                 // Phát nhạc ngay khi mở game
    }

	//7. Load hiệu ứng tiếng nhạc ghi bàn
    if (goalMusicBuffer_.loadFromFile("assets/music/goal_music_effect.mp3")) {
        goalMusicSound_.setBuffer(goalMusicBuffer_);
        goalMusicSound_.setVolume(sfxVolume_ * 100.f);
	}

    updateSoundSliderVisual();
    updateSFXSliderVisual();

    // --- SETUP HIỆU ỨNG GOAL ---
    // Nửa banner bên trái
    leftGoalBanner_.setSize({ FIELD_WIDTH / 2.f, 160.f });
    leftGoalBanner_.setFillColor(sf::Color(0, 0, 0, 180)); // Màu đen trong suốt giống ảnh
    leftGoalBanner_.setOrigin({ FIELD_WIDTH / 2.f, 80.f });

    // Nửa banner bên phải
    rightGoalBanner_.setSize({ FIELD_WIDTH / 2.f, 160.f });
    rightGoalBanner_.setFillColor(sf::Color(0, 0, 0, 180));
    rightGoalBanner_.setOrigin({ 0.f, 80.f });

    goalText_ = sf::Text(goalFont_);

    // Chữ GOAL!
    goalText_.setFont(goalFont_);
    goalText_.setString("GOAL!");
    goalText_.setCharacterSize(100);
    goalText_.setFillColor(sf::Color::White);
    goalText_.setOutlineColor(sf::Color::Black);
    goalText_.setOutlineThickness(3.f);
    sf::FloatRect textBounds = goalText_.getLocalBounds();
    goalText_.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                          textBounds.position.y + textBounds.size.y / 2.f });
    goalText_.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f });


    teams_ = {
        {0, "Argentina", "ARG", &argSprite_}, {1, "Brazil", "BRA", &braSprite_},
        {2, "France", "FRA", &fraSprite_}, {3, "Germany", "GER", &gerSprite_},
        {4, "Netherlands", "NED", &nedSprite_}, {5, "Spain", "ESP", &espSprite_},
        {6, "England", "ENG", &engSprite_}, {7, "USA", "USA", &usaSprite_},
        {8, "Uruguay", "URU", &uruSprite_}, {9, "Sweden", "SWE", &sweSprite_},
        {10, "Russia", "RUS", &rusSprite_}, {11, "South Korea", "KOR", &korSprite_},
        {12, "Japan", "JPN", &jpnSprite_}, {13, "Iran", "IRN", &irnSprite_},
        {14, "Colombia", "COL", &colSprite_}, {15, "China", "CHN", &chnSprite_},
        {16, "Chile", "CHI", &chiSprite_}, {17, "Vietnam", "VIE", &vieSprite_},
        {18, "Portugal", "POR", &porSprite_}, {19, "Italy", "ITA", &itaSprite_}
    };

    // Cập nhật lại kích thước hiển thị (TextureRect) cho toàn bộ 20 logo đội bóng
    // Cập nhật lại kích thước hiển thị (TextureRect) cho toàn bộ 20 logo đội bóng
    for (auto& team : teams_) {
        const sf::Texture& tex = team.sprite->getTexture();
        sf::Vector2u texSize = tex.getSize();

        if (texSize.x > 0 && texSize.y > 0) {
            team.sprite->setTextureRect(sf::IntRect({ 0, 0 }, { (int)texSize.x, (int)texSize.y }));
            team.sprite->setOrigin({ static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f });

            // ---> FIX LỖI LOGO BÉ KHI VÀO TRẬN TẠI ĐÂY <---
            // Tính tỷ lệ scale sao cho đường kính ảnh vừa khít với đường kính vật lý của cầu thủ (2 * PIECE_RADIUS)
            float pieceScale = (2.f * PIECE_RADIUS) / static_cast<float>(texSize.x);
            
            // Gán tỷ lệ này làm mặc định cho tất cả các Sprite của các đội
            team.sprite->setScale({ pieceScale, pieceScale });
        }
    }
}

void Game_Render::loadTeamLogos() {
    if (!state_) return;

    std::string t1Abbr = state_->getTeamAbbr(Team::Team1);
    std::string t2Abbr = state_->getTeamAbbr(Team::Team2);

    // Hệ số bù đắp (Compensation Factor) cho các ảnh 1920x1080 có logo nhỏ ở giữa
    // Bạn có thể chỉnh con số này (ví dụ 6.5f, 7.0f) để vừa với vòng hào quang
    float compFactor = 2.0f;

    // --- Xử lý Đội 1 ---
    bool found1 = false;
    for (const auto& team : teams_) {
        if (team.abbr == t1Abbr) {
            team1Texture_ = team.sprite->getTexture();
            found1 = true;
            break;
        }
    }

    if (found1) {
        isTeam1Loaded_ = true;
        team1Texture_.setSmooth(true);
        sf::Vector2u t1Size = team1Texture_.getSize();

        team1Sprite_.setTexture(team1Texture_);
        team1Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t1Size.x), static_cast<int>(t1Size.y) }));
        team1Sprite_.setOrigin({ static_cast<float>(t1Size.x) / 2.f, static_cast<float>(t1Size.y) / 2.f });

        // ---> FIX LỖI LOGO BÉ: ÁP DỤNG HỆ SỐ BÙ TRỪ VÀO ĐÂY <---
        float baseScale1 = (2.f * PIECE_RADIUS) / static_cast<float>(t1Size.y);
        float finalScale1 = baseScale1 * compFactor;
        team1Sprite_.setScale({ finalScale1, finalScale1 });
    }
    else {
        std::cerr << "Khong tim thay logo cho doi 1: " << t1Abbr << std::endl;
        isTeam1Loaded_ = false;
    }

    // --- Xử lý Đội 2 ---
    bool found2 = false;
    for (const auto& team : teams_) {
        if (team.abbr == t2Abbr) {
            team2Texture_ = team.sprite->getTexture();
            found2 = true;
            break;
        }
    }

    if (found2) {
        isTeam2Loaded_ = true;
        team2Texture_.setSmooth(true);
        sf::Vector2u t2Size = team2Texture_.getSize();

        team2Sprite_.setTexture(team2Texture_);
        team2Sprite_.setTextureRect(sf::IntRect({ 0, 0 }, { static_cast<int>(t2Size.x), static_cast<int>(t2Size.y) }));
        team2Sprite_.setOrigin({ static_cast<float>(t2Size.x) / 2.f, static_cast<float>(t2Size.y) / 2.f });

        // ---> FIX LỖI LOGO BÉ: ÁP DỤNG HỆ SỐ BÙ TRỪ VÀO ĐÂY <---
        float baseScale2 = (2.f * PIECE_RADIUS) / static_cast<float>(t2Size.y);
        float finalScale2 = baseScale2 * compFactor;
        team2Sprite_.setScale({ finalScale2, finalScale2 });
    }
    else {
        std::cerr << "Khong tim thay logo cho doi 2: " << t2Abbr << std::endl;
        isTeam2Loaded_ = false;
    }
}

void Game_Render::updateTransform() {
    //scaleX_ = static_cast<float>(viewWidth_) / FIELD_WIDTH;
    //scaleY_ = static_cast<float>(viewHeight_) / FIELD_HEIGHT;

    //float scale = (scaleX_ < scaleY_) ? scaleX_ : scaleY_;
    //scaleX_ = scale;
    //scaleY_ = scale;
    //offsetX_ = (viewWidth_ - FIELD_WIDTH * scale) / 2.f;
    //offsetY_ = (viewHeight_ - FIELD_HEIGHT * scale) / 2.f;

    //offsetX_ = 0.f;
    //offsetY_ = 0.f;

    // 1. Tính toán tỷ lệ khung hình (Aspect Ratio) mong muốn (ví dụ 1000:600)
    //float targetAspectRatio = 1000.f / 600.f;
    //float windowAspectRatio = static_cast<float>(viewWidth_) / static_cast<float>(viewHeight_);

    //float viewW = 1000.f;
    //float viewH = 600.f;
    //float posX = 0;
    //float posY = 0;

    //// 2. So sánh để tạo hiệu ứng Letterbox (giữ tỷ lệ 16:10 hoặc tỷ lệ Hiếu chọn)
    //if (windowAspectRatio > targetAspectRatio) {
    //    viewW = viewH * windowAspectRatio;
    //    posX = (viewW - 1000.f) / 2.f;
    //}
    //else {
    //    viewH = viewW / windowAspectRatio;
    //    posY = (viewH - 600.f) / 2.f;
    //}

    //// Cập nhật scale cho các tọa độ logic
    //scaleX_ = static_cast<float>(viewWidth_) / viewW;
    //scaleY_ = static_cast<float>(viewHeight_) / viewH;
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
    leftGoal.setFillColor(sf::Color::White);
    window.draw(leftGoal);

    sf::RectangleShape rightGoal(sf::Vector2f(postW, GOAL_HEIGHT));
    rightGoal.setPosition({ FIELD_WIDTH - FIELD_MARGIN_X - postW - 2.f, GOAL_Y_OFFSET});
    rightGoal.setFillColor(sf::Color::White);
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

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    float sRet = 180.f / iconReturnTexture_.getSize().x;
    applyHoverEffect(iconReturnSprite_, { {30.f, 30.f}, {40.f, 40.f} }, mPos, sRet);
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

    // In tên đội 1 (Bên trái tỉ số)
    sf::Text t1Abbr(goalFont_, state_->getTeamAbbr(Team::Team1), 36);
    t1Abbr.setFillColor(sf::Color::Red);
    centerTextAt(t1Abbr, sbCenter + sf::Vector2f(-85.f, 0.f)); // Lệch sang trái
    window.draw(t1Abbr);

    // In tên đội 2 (Bên phải tỉ số)
    sf::Text t2Abbr(goalFont_, state_->getTeamAbbr(Team::Team2), 36);
    t2Abbr.setFillColor(sf::Color::Blue);
    centerTextAt(t2Abbr, sbCenter + sf::Vector2f(85.f, 0.f)); // Lệch sang phải
    window.draw(t2Abbr);

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

	window.draw(iconOptionsSprite_);
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
    // 1. Tính toán tiến độ chạy của animation
    float progress = std::min(gameOverAnimTimer_ / GOAL_ANIM_DURATION, 1.0f);
    float ease = 1.f - std::pow(1.f - progress, 3.f); // Trượt nhanh rồi chậm dần

    // 2. Kéo 2 thanh đen (Dùng lại biến banner của Goal cho tiết kiệm bộ nhớ)
    leftGoalBanner_.setPosition({ (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });
    rightGoalBanner_.setPosition({ FIELD_WIDTH - (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });

    window.draw(leftGoalBanner_);
    window.draw(rightGoalBanner_);

    // 3. Vẽ 4 dải viền vàng trên dưới mép banner
    float lineThick = 4.f;
    sf::Color yellowColor = sf::Color(255, 220, 0);

    sf::RectangleShape topL({ FIELD_WIDTH / 2.f, lineThick });
    topL.setFillColor(yellowColor);
    topL.setOrigin({ FIELD_WIDTH / 2.f, 0.f });
    topL.setPosition({ leftGoalBanner_.getPosition().x, leftGoalBanner_.getPosition().y - 80.f });

    sf::RectangleShape botL({ FIELD_WIDTH / 2.f, lineThick });
    botL.setFillColor(yellowColor);
    botL.setOrigin({ FIELD_WIDTH / 2.f, lineThick });
    botL.setPosition({ leftGoalBanner_.getPosition().x, leftGoalBanner_.getPosition().y + 80.f });

    sf::RectangleShape topR({ FIELD_WIDTH / 2.f, lineThick });
    topR.setFillColor(yellowColor);
    topR.setOrigin({ 0.f, 0.f });
    topR.setPosition({ rightGoalBanner_.getPosition().x, rightGoalBanner_.getPosition().y - 80.f });

    sf::RectangleShape botR({ FIELD_WIDTH / 2.f, lineThick });
    botR.setFillColor(yellowColor);
    botR.setOrigin({ 0.f, lineThick });
    botR.setPosition({ rightGoalBanner_.getPosition().x, rightGoalBanner_.getPosition().y + 80.f });

    window.draw(topL); window.draw(botL);
    window.draw(topR); window.draw(botR);

    // 4. In chữ PLAYER WIN!
    if (progress > 0.1f) {
        float alpha = progress * 255.f;
        sf::Text winText(goalFont_);
        std::string msg = (state_->getScore1() >= state_->getConfig().goalsToWin) ? "PLAYER 1 WIN!" : "PLAYER 2 WIN!";

        winText.setString(msg);
        winText.setCharacterSize(80); // Cỡ chữ 80 để vừa vặn khung hình
        winText.setFillColor(sf::Color(255, 255, 0, static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, alpha)))));
        winText.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, alpha)))));
        winText.setOutlineThickness(3.f);

        sf::FloatRect textBounds = winText.getLocalBounds();
        winText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                            textBounds.position.y + textBounds.size.y / 2.f });
        winText.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f - 15.f }); // Nhích chữ WIN lên tí

        window.draw(winText);
    }

    // 5. In chữ Nhấn Space nhấp nháy khi 2 thanh đã đập vào nhau
    if (progress >= 1.0f) {
        // Hàm sin tạo sóng lượn từ 0 đến 1
        float blink = (std::sin(effectClock_.getElapsedTime().asSeconds() * 8.f) + 1.f) / 2.f;

        sf::Text hintText(sbFont_); // Dùng font sbFont_ (Arial) để ko bị lỗi Tiếng Việt
        hintText.setString(L"Press Space to exit to the home screen"); // Dùng chữ L để hỗ trợ Unicode tiếng Việt
        hintText.setCharacterSize(22);

        // Màu trắng nhấp nháy độ mờ từ 100 đến 255
        hintText.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(100.f + blink * 155.f)));

        sf::FloatRect hintBounds = hintText.getLocalBounds();
        hintText.setOrigin({ hintBounds.position.x + hintBounds.size.x / 2.f,
                             hintBounds.position.y + hintBounds.size.y / 2.f });
        // Canh chữ xuống góc dưới
        hintText.setPosition({ FIELD_WIDTH / 2.f, FIELD_HEIGHT / 2.f + 45.f });

        window.draw(hintText);
    }
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

void Game_Render::drawOptionsMenu(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);
	window.draw(optionsKhungSprite_);

    sf::Font titleFont;
    if (!titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf")) { // Đường dẫn font mới
        std::cerr << "LOI: Khong load duoc font tieu de!" << std::endl;
        // Nếu lỗi, hiện text tạm bằng font cơ bản để game không crash
        titleFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    }

    // 2. Tạo đối tượng Text
    sf::Text titleText(titleFont);
    titleText.setString("OPTIONS");

    // 3. Chỉnh kích thước (Trong ảnh chữ rất to, khoảng 70-80)
    titleText.setCharacterSize(80);

    // 4. Chỉnh màu sắc (Trong ảnh là màu đen tuyền)
    titleText.setFillColor(sf::Color::Black);

    // 5. Căn giữa tiêu đề vào chính giữa sân
    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                          textBounds.position.y + textBounds.size.y / 2.f });

    // Đặt ở tọa độ (500, 100), cao hơn một chút để không bị 8-ball đè
    titleText.setPosition({ FIELD_WIDTH / 2.f + 10.f, 100.f });

    // 6. Vẽ
    window.draw(titleText);

    // 2. Tạo đối tượng Text
    sf::Text titleSound(titleFont);
    titleSound.setString("SOUND:");

    // 3. Chỉnh kích thước (Trong ảnh chữ rất to, khoảng 70-80)
    titleSound.setCharacterSize(40);

    // 4. Chỉnh màu sắc (Trong ảnh là màu đen tuyền)
    titleSound.setFillColor(sf::Color::White);

    // 5. Căn giữa tiêu đề vào chính giữa sân
    sf::FloatRect textBoundsSound = titleSound.getLocalBounds();
    titleSound.setOrigin({ textBoundsSound.position.x + textBoundsSound.size.x / 2.f,
                          textBoundsSound.position.y + textBoundsSound.size.y / 2.f });

    // Đặt ở tọa độ (500, 100), cao hơn một chút để không bị 8-ball đè
    titleSound.setPosition({ 280.f, 238.f });

    // 6. Vẽ
    window.draw(titleSound);

    // 2. Tạo đối tượng Text
    sf::Text titleSFX(titleFont);
    titleSFX.setString("SFX:");

    // 3. Chỉnh kích thước (Trong ảnh chữ rất to, khoảng 70-80)
    titleSFX.setCharacterSize(40);

    // 4. Chỉnh màu sắc (Trong ảnh là màu đen tuyền)
    titleSFX.setFillColor(sf::Color::White);

    // 5. Căn giữa tiêu đề vào chính giữa sân
    sf::FloatRect textBoundsSFX = titleSFX.getLocalBounds();
    titleSFX.setOrigin({ textBoundsSFX.position.x + textBoundsSFX.size.x / 2.f,
                          textBoundsSFX.position.y + textBoundsSFX.size.y / 2.f });

    // Đặt ở tọa độ (500, 100), cao hơn một chút để không bị 8-ball đè
    titleSFX.setPosition({ 280.f, 330.f });

    // 6. Vẽ
    window.draw(titleSFX);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    float sRet = 180.f / iconReturnTexture_.getSize().x;
    applyHoverEffect(iconReturnSprite_, { { 30.f, 30.f},{ 40.f, 40.f} }, mPos, sRet);
    window.draw(iconReturnSprite_);


    // ===== 1. VẼ THANH SOUND =====
    // Cập nhật vị trí nút trước khi vẽ
    updateSoundSliderVisual();

    // Vẽ thanh nền tối (phần chưa được fill)
    slideBarSoundSprite_.setColor(sf::Color(0, 0, 0));
    slideBarSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 },
        { (int)slideBarSoundTexture_.getSize().x, (int)slideBarSoundTexture_.getSize().y }));
    window.draw(slideBarSoundSprite_);

    // Vẽ phần thanh đã được fill (màu vàng)
    slideBarSoundSprite_.setColor(sf::Color(255, 255, 0));
    int cutWidthSound = static_cast<int>(slideBarSoundTexture_.getSize().x * soundVolume_);
    slideBarSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { cutWidthSound, (int)slideBarSoundTexture_.getSize().y }));
    window.draw(slideBarSoundSprite_);

    // Vẽ nút tròn
    window.draw(slideNodeSoundSprite_);

    // ===== 2. VẼ THANH SFX =====
    updateSFXSliderVisual();

    // Nền Đen
    slideBarSoundSprite_.setColor(sf::Color(0, 0, 0));
    slideBarSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideBarSoundTexture_.getSize().x, (int)slideBarSoundTexture_.getSize().y }));
    window.draw(slideBarSoundSprite_);

    // Vạch Vàng
    slideBarSoundSprite_.setColor(sf::Color(255, 255, 0));
    float bgWidth = 270.f;
    float bgStartX = (FIELD_WIDTH / 2.f) + 10.f - (bgWidth / 2.f);

    float nodeXSound = slideNodeSoundSprite_.getPosition().x;
    float fillRatioSound = (nodeXSound - bgStartX) / bgWidth;
    cutWidthSound = static_cast<int>(slideBarSoundTexture_.getSize().x * fillRatioSound);
    slideBarSoundSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { cutWidthSound, (int)slideBarSoundTexture_.getSize().y }));
    window.draw(slideBarSoundSprite_);

    window.draw(slideNodeSoundSprite_);

    // ---------------------------------------------------------
    // THÊM MỚI: VẼ % ÂM LƯỢNG SOUND NGAY DƯỚI THANH
    // ---------------------------------------------------------
    int soundPercent = static_cast<int>(soundVolume_ * 100.f);
    sf::Text soundPercentText(titleFont, std::to_string(soundPercent) + "%", 26);
    soundPercentText.setFillColor(sf::Color::White);
    sf::FloatRect boundsSound = soundPercentText.getLocalBounds();
    soundPercentText.setOrigin({ boundsSound.position.x + boundsSound.size.x / 2.f, boundsSound.position.y + boundsSound.size.y / 2.f });

    // Đặt ở chính giữa thanh trượt, dịch xuống dưới khoảng 35 pixel
    soundPercentText.setPosition({ (FIELD_WIDTH / 2.f) + 10.f, (FIELD_HEIGHT / 2.f) + 25.f });
    window.draw(soundPercentText);
    // ---------------------------------------------------------



    // ===== 2. VẼ THANH SFX =====
    updateSFXSliderVisual();

    slideBarSFXSprite_.setColor(sf::Color(0, 0, 0));
    slideBarSFXSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { (int)slideBarSFXTexture_.getSize().x, (int)slideBarSFXTexture_.getSize().y }));
    window.draw(slideBarSFXSprite_);

    slideBarSFXSprite_.setColor(sf::Color(255, 255, 0));
    float nodeXSFX = slideNodeSFXSprite_.getPosition().x;
    float fillRatioSFX = (nodeXSFX - bgStartX) / bgWidth;
    int cutWidthSFX = static_cast<int>(slideBarSFXTexture_.getSize().x * fillRatioSFX);
    slideBarSFXSprite_.setTextureRect(sf::IntRect({ 0, 0 }, { cutWidthSFX, (int)slideBarSFXTexture_.getSize().y }));
    window.draw(slideBarSFXSprite_);

    window.draw(slideNodeSFXSprite_);

    // ---------------------------------------------------------
    // THÊM MỚI: VẼ % ÂM LƯỢNG SFX NGAY DƯỚI THANH
    // ---------------------------------------------------------
    int sfxPercent = static_cast<int>(sfxVolume_ * 100.f);
    sf::Text sfxPercentText(titleFont, std::to_string(sfxPercent) + "%", 26);
    sfxPercentText.setFillColor(sf::Color::White);
    sf::FloatRect boundsSFX = sfxPercentText.getLocalBounds();
    sfxPercentText.setOrigin({ boundsSFX.position.x + boundsSFX.size.x / 2.f, boundsSFX.position.y + boundsSFX.size.y / 2.f });

    // Đặt ở chính giữa thanh trượt SFX, dịch xuống dưới khoảng 35 pixel
    sfxPercentText.setPosition({ (FIELD_WIDTH / 2.f) + 10.f, (FIELD_HEIGHT / 2.f) + 115.f });
    window.draw(sfxPercentText);
    // ---------------------------------------------------------
}


void Game_Render::drawMainMenu(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Vẽ họa tiết trang trí
    //ball8Sprite_.setPosition({ 150.f, 150.f });
    window.draw(ball8Sprite_);
    //goalMenuSprite_.setPosition({ 750.f, 400.f });
    window.draw(goalMenuSprite_);

    window.draw(ballMenuSprite_);

    float sPlay = 800.f / btnPlayTexture_.getSize().x;
    applyHoverEffect(btnPlaySprite_, {{ 375.f, 210.f }, { 250.f, 80.f }}, mPos, sPlay);
    window.draw(btnPlaySprite_);

    float sOpt = 800.f / btnOptionsTexture_.getSize().x;
    applyHoverEffect(btnOptionsSprite_, { { 375.f, 340.f },{ 250.f, 80.f}}, mPos, sOpt);
    window.draw(btnOptionsSprite_);

    float sQuit = 180.f / iconQuitTexture_.getSize().x;
    applyHoverEffect(iconQuitSprite_, { { 925.f, 25.f }, {50.f, 50.f}}, mPos, sQuit);
    window.draw(iconQuitSprite_);

    float sInfo = 40.f / iconInforTexture_.getSize().x;
    applyHoverEffect(iconInforSprite_, {{ 880.f, 30.f }, { 40.f, 40.f }}, mPos, sInfo);
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

    //// 2. Tạo hình chữ nhật để vẽ
    //sf::RectangleShape debugRect(sf::Vector2f(250.f, 80.f));
    //debugRect.setPosition({ 375.f, 210.f });

    ////// 3. Thiết lập hiển thị (Chỉ vẽ viền để không che ảnh đội hình)
    //debugRect.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
    //debugRect.setOutlineColor(sf::Color::Red);       // Viền đỏ cho nổi bật
    //debugRect.setOutlineThickness(2.f);              // Độ dày viền 2px

    ////// 4. Vẽ lên cửa sổ
    //window.draw(debugRect);

}


void Game_Render::drawSelectMode(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

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

    float sPvp = 550.f / pvpTexture_.getSize().x;
    applyHoverEffect(pvpSprite_, { { 365.f, 170.f}, {280.f, 70.f} }, mPos, sPvp);
    window.draw(pvpSprite_);

    // PVAI
    float sPvai = 550.f / pvaiTexture_.getSize().x;
    applyHoverEffect(pvaiSprite_, {{ 365.f, 260.f }, { 280.f, 70.f }}, mPos, sPvai);
    window.draw(pvaiSprite_);

    // AIVAI
    float sAivai = 550.f / aivaiTexture_.getSize().x;
    applyHoverEffect(aivaiSprite_, {{ 365.f, 350.f }, { 280.f, 70.f }}, mPos, sAivai);
    window.draw(aivaiSprite_);

    // 2. Tạo hình chữ nhật để vẽ
    //sf::RectangleShape debugRect(sf::Vector2f(40.f, 40.f));
    //debugRect.setPosition({ 50.f - 20.f, 50.f - 20.f });

    ////// 3. Thiết lập hiển thị (Chỉ vẽ viền để không che ảnh đội hình)
    //debugRect.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
    //debugRect.setOutlineColor(sf::Color::Red);       // Viền đỏ cho nổi bật
    //debugRect.setOutlineThickness(2.f);              // Độ dày viền 2px

    ////// 4. Vẽ lên cửa sổ
    //window.draw(debugRect);

    // Nút quay lại (iconQuit đã load)
    float sRet = 180.f / iconReturnTexture_.getSize().x;
    applyHoverEffect(iconReturnSprite_, { { 30.f, 30.f},{ 40.f, 40.f} }, mPos, sRet);
    window.draw(iconReturnSprite_);
}


void Game_Render::drawSelectLineup(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

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
        // 
        // Tọa độ và vùng va chạm của card (Khớp với cardRect trong Controller)
        sf::Vector2f cardPos = { 330.f + i * 345.f, 240.f };
        sf::FloatRect cardRect({ 230.f + i * 345.f, 130.f }, { 200.f, 220.f });

        lineup->sprite.setPosition({ 330.f + i * 345.f, 240.f });

        float baseScaleLineup = 200.f / lineup->texture->getSize().x;

        // --- ĐOẠN FIX TẠI ĐÂY ---
        if (lineup->id == selectedLineupId_) {
            // Nếu đã chọn: Ép về scale và màu sắc bình thường, không chạy hiệu ứng Hover
            lineup->sprite.setScale({ baseScaleLineup, baseScaleLineup });
            lineup->sprite.setColor(sf::Color::White); // Luôn sáng rõ vì đã chọn
        }
        else {
            // Nếu chưa chọn: Cho phép chạy hiệu ứng Hover
            applyHoverEffect(lineup->sprite, cardRect, mPos, baseScaleLineup);
        }

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

    // Lấy vị trí chuột hiện tại (cần quy đổi sang tọa độ World/Logic)
    sf::FloatRect btnRect({ 420.f, 425.f }, { 160.f, 50.f });
    sf::Sprite* currentBtn = (pickingForTeam_ == 1 && state_->getConfig().mode != GameMode::PvAI)
        ? &nextBtnSprite_ : &startBtnSprite_;

    float baseScaleBtn = 400.f / nextBtnTexture_.getSize().x;
    applyHoverEffect(*currentBtn, btnRect, mPos, baseScaleBtn);
    window.draw(*currentBtn);


    // Vẽ mũi tên chuyển trang và nút Start
    if (currentLineupPage_ > 0) {
        sf::FloatRect prevRect({ 40.f, 210.f }, { 80.f, 40.f });
        arrowLeftSprite_.setPosition({ 80.f, 230.f });
        float sArrow = 200.f / arrowLeftTexture_.getSize().x;
        applyHoverEffect(arrowLeftSprite_, prevRect, mPos, sArrow);
        window.draw(arrowLeftSprite_);
    }

    if ((currentLineupPage_ + 1) * 2 < lineups_.size()) {
        sf::FloatRect nextRect({ 885.f, 210.f }, { 80.f, 40.f });
        arrowRightSprite_.setPosition({ 925.f, 230.f });
        float sArrow = 200.f / arrowRightTexture_.getSize().x;
        applyHoverEffect(arrowRightSprite_, nextRect, mPos, sArrow);
        window.draw(arrowRightSprite_);
    }

    float sRet = 180.f / iconReturnTexture_.getSize().x;
    applyHoverEffect(iconReturnSprite_, { {30.f, 30.f}, {40.f, 40.f} }, mPos, sRet);
    window.draw(iconReturnSprite_);


    //// 2. Tạo hình chữ nhật để vẽ
    //sf::RectangleShape debugRect(sf::Vector2f(80.f, 40.f));
    //debugRect.setPosition({ 885.f, 210.f });

    ////// 3. Thiết lập hiển thị (Chỉ vẽ viền để không che ảnh đội hình)
    //debugRect.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
    //debugRect.setOutlineColor(sf::Color::Red);       // Viền đỏ cho nổi bật
    //debugRect.setOutlineThickness(2.f);              // Độ dày viền 2px

    ////// 4. Vẽ lên cửa sổ
    //window.draw(debugRect);
}


void Game_Render::drawConfirmQuit(sf::RenderWindow& window) {
    // --- BƯỚC 1: VẼ LỚP NỀN MỜ (OVERLAY) ---
    // Tạo một hình chữ nhật to bằng đúng kích thước cửa sổ game
    sf::RectangleShape overlay(sf::Vector2f(FIELD_WIDTH, FIELD_HEIGHT));
    overlay.setPosition({ 0.f, 0.f });

    // Màu đen (0,0,0) với độ trong suốt là 150 (giá trị từ 0 đến 255)
    // 0 là trong suốt hoàn toàn, 255 là đen kịt. 150-180 là con số đẹp để làm mờ nền.
    overlay.setFillColor(sf::Color(0, 0, 0, 150));

    window.draw(overlay);

    window.draw(msbQuitSprite_);

    // --- BƯỚC 3: VẼ DEBUG RECT (NẾU CẦN) ---
    sf::RectangleShape debugRect(sf::Vector2f(135.f, 47.f));
    debugRect.setPosition({ 515.f, 273.f });
    debugRect.setFillColor(sf::Color::Transparent);
    debugRect.setOutlineColor(sf::Color::Red);
    debugRect.setOutlineThickness(2.f);
    // window.draw(debugRect); // Tắt đi khi đã khớp vị trí

    //// 2. Tạo hình chữ nhật để vẽ
    //sf::RectangleShape debugRect(sf::Vector2f(135.f, 47.f));
    //debugRect.setPosition({ 515.f, 273.f });

    ////// 3. Thiết lập hiển thị (Chỉ vẽ viền để không che ảnh đội hình)
    //debugRect.setFillColor(sf::Color::Transparent); // Trong suốt bên trong
    //debugRect.setOutlineColor(sf::Color::Red);       // Viền đỏ cho nổi bật
    //debugRect.setOutlineThickness(2.f);              // Độ dày viền 2px

    ////// 4. Vẽ lên cửa sổ
    //window.draw(debugRect);
}


// Hàm bổ trợ xử lý hiệu ứng Hover cho Sprite
void Game_Render::applyHoverEffect(sf::Sprite& sprite, sf::FloatRect bounds, sf::Vector2f mPos, float baseScale) {
    if (bounds.contains(mPos)) {
        sprite.setScale({ baseScale * 1.08f, baseScale * 1.08f }); // Phóng to 8%
        sprite.setColor(sf::Color::White); // Sáng rõ
    }
    else {
        sprite.setScale({ baseScale, baseScale }); // Về bình thường
        sprite.setColor(sf::Color(230, 230, 230)); // Hơi tối nhẹ
    }
}

void Game_Render::drawSelectTeam(sf::RenderWindow& window) {
    window.draw(menuBgSprite_);
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    sf::Font titleFont;
    titleFont.openFromFile("assets/font/GulfsDisplay-SemiCondensed.ttf");

    sf::Text titleText(titleFont, "Pick Team " + std::to_string(pickingTeamFor_), 80);
    titleText.setFillColor(sf::Color::Black);
    sf::FloatRect textBounds = titleText.getLocalBounds();
    titleText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f });
    titleText.setPosition({ FIELD_WIDTH / 2.f, 60.f });
    window.draw(titleText);

    // Vẽ lưới 2x2 (4 đội 1 trang)
    int startIdx = currentTeamPage_ * 4;
    for (int i = 0; i < 4 && (startIdx + i) < teams_.size(); ++i) {
        auto& team = teams_[startIdx + i];

        int row = i / 2;
        int col = i % 2;
        float xPos = 350.f + col * 300.f;
        float yPos = 160.f + row * 160.f;

        sf::FloatRect cardRect({ xPos - 100.f, yPos - 60.f }, { 200.f, 150.f });

        sf::Sprite* spr = team.sprite;
        spr->setPosition({ xPos, yPos });
        spr->setOrigin({ spr->getTexture().getSize().x / 2.f, spr->getTexture().getSize().y / 2.f });

        float baseScale = 280.f / spr->getTexture().getSize().x; // Ép logo về 280px

        // ---> THÊM BIẾN KIỂM TRA ĐỘI ĐÃ BỊ CHỌN CHƯA <---
        bool isTaken = (pickingTeamFor_ == 2 && team.abbr == state_->getTeamAbbr(Team::Team1));

        if (isTaken) {
            // NẾU ĐÃ BỊ LẤY BỞI P1: Làm mờ xám đi và không cho Hover
            spr->setScale({ baseScale, baseScale });
            spr->setColor(sf::Color(150, 150, 150, 160)); // Màu xám, độ mờ 100
        }
        else if (team.id == selectedTeamId_) {
            spr->setScale({ baseScale, baseScale });
            spr->setColor(sf::Color::White);
            // Vẽ viền vàng xác nhận
            sf::RectangleShape highlight({ 160.f, 120.f });
            highlight.setPosition({ xPos - 80.f, yPos - 45.f });
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineColor(sf::Color::Yellow);
            highlight.setOutlineThickness(4.f);
            window.draw(highlight);
        }
        else {
            applyHoverEffect(*spr, cardRect, mPos, baseScale);
        }
        window.draw(*spr);

        // Vẽ tên đội ngay bên dưới logo
        sf::Text nameText(titleFont, team.name, 28);
        nameText.setFillColor(isTaken ? sf::Color(150, 150, 150, 160) : sf::Color::White);
        sf::FloatRect nBounds = nameText.getLocalBounds();
        nameText.setOrigin({ nBounds.position.x + nBounds.size.x / 2.f, nBounds.position.y + nBounds.size.y / 2.f });
        nameText.setPosition({ xPos, yPos + 50.f });
        window.draw(nameText);
    }

    // Nút Next/Start
    sf::FloatRect btnRect({ 420.f, 425.f }, { 160.f, 50.f });
    sf::Sprite* currentBtn = &nextBtnSprite_;
    float baseScaleBtn = 400.f / nextBtnTexture_.getSize().x;
    applyHoverEffect(*currentBtn, btnRect, mPos, baseScaleBtn);
    window.draw(*currentBtn);


    // Mũi tên chuyển trang
    if (currentTeamPage_ > 0) {
        sf::FloatRect prevRect({ 40.f, 230.f }, { 80.f, 40.f });
        arrowLeftSprite_.setPosition({ 80.f, 250.f });
        float sArrow = 200.f / arrowLeftTexture_.getSize().x;
        applyHoverEffect(arrowLeftSprite_, prevRect, mPos, sArrow);
        window.draw(arrowLeftSprite_);
    }
    if ((currentTeamPage_ + 1) * 4 < teams_.size()) {
        sf::FloatRect nextRect({ 885.f, 230.f }, { 80.f, 40.f });
        arrowRightSprite_.setPosition({ 925.f, 250.f });
        float sArrow = 200.f / arrowRightTexture_.getSize().x;
        applyHoverEffect(arrowRightSprite_, nextRect, mPos, sArrow);
        window.draw(arrowRightSprite_);
    }

    // Nút Back
    float sRet = 180.f / iconReturnTexture_.getSize().x;
    applyHoverEffect(iconReturnSprite_, { {30.f, 30.f}, {40.f, 40.f} }, mPos, sRet);
    window.draw(iconReturnSprite_);
}

void Game_Render::draw(sf::RenderWindow& window) {
    if (!state_) return;
    updateTransform();

    // ====== THÊM ĐOẠN NÀY ĐỂ FADE OUT TIẾNG GOAL ======
    // Kiểm tra xem tiếng goal_effect có đang phát không
    if (goalScoreSound_.getStatus() == sf::Sound::Status::Playing) {
        float currentGoalVol = goalScoreSound_.getVolume();

        // Nếu âm lượng vẫn lớn hơn 0, tiến hành trừ dần
        if (currentGoalVol > 0.f) {
            // Trừ đi một lượng nhỏ mỗi frame. 
            // 0.3f ở 60 FPS sẽ mất khoảng 5 giây để giảm từ 100 xuống 0.
            // (Bạn có thể tăng lên 0.5f hoặc 1.0f nếu muốn nó tắt nhanh hơn)
            currentGoalVol -= 0.3f;

            if (currentGoalVol <= 0.f) {
                currentGoalVol = 0.f; // Chốt chặn không cho xuống âm
                // (Tùy chọn) Có thể tắt hẳn âm thanh luôn khi đã về 0 cho nhẹ máy:
                // goalScoreSound_.stop(); 
            }

            goalScoreSound_.setVolume(currentGoalVol);
        }
    }
    // ===================================================


    GamePhase current = state_->getPhase();


    // ====== THÊM ĐOẠN NÀY ĐỂ TỰ ĐỘNG CHỈNH ÂM LƯỢNG NHẠC NỀN ======
    // Nếu đang ở trong trận đấu, HOẶC đang mở bảng hỏi Thoát khi đang trong trận đấu
    // TRƯỜNG HỢP 1: ĐANG TRONG TRẬN ĐẤU (Bóng lăn bình thường)
    if (current == GamePhase::Playing ||
        (current == GamePhase::ConfirmQuit && state_->getPreviousPhase() == GamePhase::Playing) || 
        (current == GamePhase::Options && state_->getPreviousPhase() == GamePhase::Playing)) {

        // Tắt nhạc Menu
        if (bgMusic_.getStatus() == sf::Sound::Status::Playing) bgMusic_.stop();

        // Bật tiếng râm ran sân vận động
        stadiumEffect.setVolume(soundVolume_ * 70.f);
        if (stadiumEffect.getStatus() != sf::Sound::Status::Playing) {
            stadiumEffect.play();
        }
    }
    // TRƯỜNG HỢP 2: ĐANG GHI BÀN (Ăn mừng)
    else if (current == GamePhase::GoalScored) {

        // Chắc chắn tắt nhạc Menu
        if (bgMusic_.getStatus() == sf::Sound::Status::Playing) bgMusic_.stop();

        // ---> TẠM DỪNG (TẮT) TIẾNG SÂN VẬN ĐỘNG ĐỂ NHƯỜNG SÂN KHẤU CHO NHẠC GOAL <---
        if (stadiumEffect.getStatus() == sf::Sound::Status::Playing) {
            stadiumEffect.stop();
        }
    }
    // TRƯỜNG HỢP 3: CÁC MÀN HÌNH KHÁC (Menu, Options, Pick Lineup, Game Over...)
    else {
        // Tắt tiếng sân vận động
        if (stadiumEffect.getStatus() == sf::Sound::Status::Playing) {
            stadiumEffect.stop();
        }

        // Bật lại nhạc nền Menu to rõ ràng từ đầu
        bgMusic_.setVolume(soundVolume_ * 100.f);
        if (bgMusic_.getStatus() != sf::Sound::Status::Playing) {
            bgMusic_.play();
        }
    }
    // =============================================================

    // Nếu đang ở ConfirmQuit, vẽ nền trước (phase trước đó)
    if (current == GamePhase::ConfirmQuit) {
        GamePhase behind = state_->getPreviousPhase();

        if (behind == GamePhase::Menu) {
            sf::View menuView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
            window.setView(menuView);
            drawMainMenu(window);
        }
        else if (behind == GamePhase::Setup) {
            sf::View setupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
            window.setView(setupView);
            drawSelectMode(window);
        }
        else if (behind == GamePhase::PickLineup) {
            sf::View lineupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
            window.setView(lineupView);
            drawSelectLineup(window);
        }
        else if (current == GamePhase::Options) {
            sf::View optionsView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
            window.setView(optionsView);
            drawOptionsMenu(window);
        }
        else if (behind == GamePhase::Playing) {
            sf::View gameView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
            window.setView(gameView);
            drawField(window);
            drawGoals(window);
            drawBall(window);
            drawPieces(window);
            drawUI(window);
        }

        // SAU KHI VẼ XONG NỀN, VẼ MESSAGE BOX CHỒNG LÊN
        drawConfirmQuit(window);
        return; // THOÁT LUÔN, KHÔNG ĐI XUỐNG PHÍA DƯỚI
    }

    // Các phase bình thường (không phải ConfirmQuit)
    if (current == GamePhase::Menu) {
        sf::View menuView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(menuView);
        drawMainMenu(window);
        return;
    }
    else if (current == GamePhase::Setup) {
        sf::View setupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(setupView);
        drawSelectMode(window);
        return;
    }
    else if (current == GamePhase::PickTeam) {
        sf::View teamView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(teamView);
        drawSelectTeam(window);
        return;
    }
    else if (current == GamePhase::PickLineup) {
        sf::View lineupView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(lineupView);
        drawSelectLineup(window);
        return;
    }
    else if(current == GamePhase::Options) {
        sf::View optionsView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
        window.setView(optionsView);
        drawOptionsMenu(window);
        return;
	}

    // Phase Playing và GameOver
    sf::View gameView(sf::FloatRect({ 0.f, 0.f }, { FIELD_WIDTH, FIELD_HEIGHT }));
    window.setView(gameView);

    drawField(window);
    drawGoals(window);
    drawBall(window);
    drawPieces(window);


    if (dragActive_) {
        drawShotAiming(window);
        dragActive_ = false;
    }

    if (current == GamePhase::GameOver)
        drawGameOver(window);
    else
        drawUI(window);

    drawGoalAnimation(window);
}

// Thêm vào cuối Game_Render.cpp

void Game_Render::setSoundVolume(float vol) {
    soundVolume_ = vol;
    bgMusic_.setVolume(soundVolume_ * 100.f);
}

void Game_Render::setSFXVolume(float vol) {
    sfxVolume_ = vol;
    hitSoundKick_.setVolume(sfxVolume_ * 100.f);
    hitSoundCollide_.setVolume(sfxVolume_ * 100.f);
}

void Game_Render::updateSoundSliderVisual() {
    float bgWidth = 270.f;
    float bgStartX = (FIELD_WIDTH / 2.f) + 10.f - (bgWidth / 2.f);
    float realY = (FIELD_HEIGHT / 2.f) - 10.f;

    sf::Vector2u texSize = slideBarSoundTexture_.getSize();
    slideBarSoundSprite_.setOrigin({ 0.f, texSize.y / 2.f });
    slideBarSoundSprite_.setPosition({ bgStartX, realY });

    // --- CHỈNH PHẠM VI KÉO Ở ĐÂY ---
    float padding = 18.f; // Tăng lên nếu muốn thâu hẹp đường kéo lại
    float dragStartX = bgStartX + padding;
    float dragWidth = bgWidth - (2.f * padding);
    // -------------------------------

    float nodeX = dragStartX + (dragWidth * soundVolume_);
    slideNodeSoundSprite_.setPosition({ nodeX, realY });
}

void Game_Render::updateSFXSliderVisual() {
    float bgWidth = 270.f;
    float bgStartX = (FIELD_WIDTH / 2.f) + 10.f - (bgWidth / 2.f);
    float realY = (FIELD_HEIGHT / 2.f) + 80.f;

    sf::Vector2u texSize = slideBarSFXTexture_.getSize();
    slideBarSFXSprite_.setOrigin({ 0.f, texSize.y / 2.f });
    slideBarSFXSprite_.setPosition({ bgStartX, realY });

    // --- CHỈNH PHẠM VI KÉO Ở ĐÂY ---
    float padding = 18.f;
    float dragStartX = bgStartX + padding;
    float dragWidth = bgWidth - (2.f * padding);
    // -------------------------------

    float nodeX = dragStartX + (dragWidth * sfxVolume_);
    slideNodeSFXSprite_.setPosition({ nodeX, realY });
}

void Game_Render::updateVolumeFromMouse(sf::Vector2f mousePos) {
    float bgWidth = 270.f;
    float bgStartX = (FIELD_WIDTH / 2.f) + 10.f - (bgWidth / 2.f);

    // Áp dụng khoảng đệm y hệt như trên
    float padding = 18.f;
    float dragStartX = bgStartX + padding;
    float dragWidth = bgWidth - (2.f * padding);

    // Kẹp chuột vào giới hạn kéo mới
    float clampedX = mousePos.x;
    if (clampedX < dragStartX) clampedX = dragStartX;
    if (clampedX > dragStartX + dragWidth) clampedX = dragStartX + dragWidth;

    float newRatio = (clampedX - dragStartX) / dragWidth;

    if (isDraggingSound_) {
        setSoundVolume(newRatio);
        updateSoundSliderVisual();
    }
    else if (isDraggingSFX_) {
        setSFXVolume(newRatio);
        updateSFXSliderVisual();
    }
}

void Game_Render::handleEvent(const sf::Event& event, sf::RenderWindow& window, sf::Vector2f mousePos) {
    float bgWidth = 270.f;
    float bgStartX = (FIELD_WIDTH / 2.f) + 10.f - (bgWidth / 2.f);
    float padding = 18.f;
    float dragStartX = bgStartX + padding;
    float dragWidth = bgWidth - (2.f * padding);

    float soundY = (FIELD_HEIGHT / 2.f) - 10.f;
    float sfxY = (FIELD_HEIGHT / 2.f) + 80.f;

    // Vùng hit detection
    sf::FloatRect soundArea({ dragStartX - 20.f, soundY - 20.f }, { dragWidth + 40.f,  50.f });
    sf::FloatRect sfxArea({ dragStartX - 20.f, sfxY - 20.f }, { dragWidth + 40.f,  50.f });

    sf::FloatRect soundNodeArea({ slideNodeSoundSprite_.getPosition().x - 20.f, slideNodeSoundSprite_.getPosition().y - 20.f }, { 40.f, 40.f });
    sf::FloatRect sfxNodeArea({ slideNodeSFXSprite_.getPosition().x - 20.f, slideNodeSFXSprite_.getPosition().y - 20.f }, { 40.f, 40.f });

    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            if (soundNodeArea.contains(mousePos) || soundArea.contains(mousePos)) {
                isDraggingSound_ = true;
                isDraggingSFX_ = false;
                updateVolumeFromMouse(mousePos);
            }
            else if (sfxNodeArea.contains(mousePos) || sfxArea.contains(mousePos)) {
                isDraggingSFX_ = true;
                isDraggingSound_ = false;
                updateVolumeFromMouse(mousePos);
            }
        }
    }

    if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mbr->button == sf::Mouse::Button::Left) {
            isDraggingSound_ = false;
            isDraggingSFX_ = false;
        }
    }

    if (event.is<sf::Event::MouseMoved>()) {
        if (isDraggingSound_ || isDraggingSFX_) {
            updateVolumeFromMouse(mousePos);
        }
    }
}


void Game_Render::playHitSound() {
    hitSoundKick_.play();
}

void Game_Render::playCollideSound() {
    hitSoundCollide_.play();
}

void Game_Render::playWhistleSound() {
    whistleSound_.play();
}

void Game_Render::playGoalSound() {
    goalScoreSound_.setVolume(sfxVolume_ * 100.f);
    goalScoreSound_.play();
}

void Game_Render::playGoalMusic() {
    goalMusicSound_.setVolume(sfxVolume_ * 70.f);
	goalMusicSound_.play();
}

void Game_Render::startGoalAnimation() {
    goalAnimState_ = 1;
    goalAnimTimer_ = 0.f;
    if (state_->isFoul()) {
        playWhistleSound(); // Lỗi chỉ tuýt còi
    }
    else {
        playGoalSound();    // Vào thì hô GOAL và phát nhạc
        playGoalMusic();
    }
}

void Game_Render::updateGoalAnimation(float dt) {
    if (goalAnimState_ == 0) return;

    if (goalAnimState_ == 1) { // GIAI ĐOẠN 1: TRƯỢT VÀO TỪ 2 BÊN
        goalAnimTimer_ += dt;
        float progress = std::min(goalAnimTimer_ / GOAL_ANIM_DURATION, 1.0f);
        // Hiệu ứng ease-out (Trượt nhanh rồi chậm dần lúc vào giữa)
        float ease = 1.f - std::pow(1.f - progress, 3.f);

        leftGoalBanner_.setPosition({ (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });
        rightGoalBanner_.setPosition({ FIELD_WIDTH - (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });

        if (progress >= 1.0f) goalAnimState_ = 2; // Xong thì chuyển sang chờ
    }
    else if (goalAnimState_ == 2) { // GIAI ĐOẠN 2: CHỜ ÂM THANH TẮT
        bool soundDone = false;
        if (state_->isFoul()) {
            soundDone = (whistleSound_.getStatus() != sf::Sound::Status::Playing);
        }
        else {
            soundDone = (goalMusicSound_.getStatus() != sf::Sound::Status::Playing);
        }

        if (soundDone) {
            goalAnimState_ = 3;
            goalAnimTimer_ = 0.f;
        }
    }
    else if (goalAnimState_ == 3) { // GIAI ĐOẠN 3: TRƯỢT NGƯỢC RA LẠI
        goalAnimTimer_ += dt;
        float progress = std::min(goalAnimTimer_ / GOAL_ANIM_DURATION, 1.0f);
        // Hiệu ứng ease-in
        float ease = std::pow(progress, 3.f);

        leftGoalBanner_.setPosition({ (FIELD_WIDTH / 2.f) - (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });
        rightGoalBanner_.setPosition({ FIELD_WIDTH / 2.f + (FIELD_WIDTH / 2.f) * ease, FIELD_HEIGHT / 2.f });

        if (progress >= 1.0f) goalAnimState_ = 0; // KẾT THÚC TOÀN BỘ
    }
}

bool Game_Render::isGoalAnimationDone() const {
    return goalAnimState_ == 0;
}

void Game_Render::drawGoalAnimation(sf::RenderWindow& window) {
    if (goalAnimState_ == 0) return;

    window.draw(leftGoalBanner_);
    window.draw(rightGoalBanner_);

    // ====== ĐÃ SỬA: ĐỔI MÀU NẾU LÀ FOUL ======
    bool isFoul = state_->isFoul();
    float lineThick = 4.f;
    sf::Color borderColor = isFoul ? sf::Color(255, 30, 30) : sf::Color(255, 220, 0); // Lỗi Đỏ, Ghi bàn Vàng

    // 1. Viền trên và dưới cho thanh BÊN TRÁI
    sf::RectangleShape topL({ FIELD_WIDTH / 2.f, lineThick });
    topL.setFillColor(borderColor); // <-- dùng borderColor
    topL.setOrigin({ FIELD_WIDTH / 2.f, 0.f });
    topL.setPosition({ leftGoalBanner_.getPosition().x, leftGoalBanner_.getPosition().y - 80.f });

    sf::RectangleShape botL({ FIELD_WIDTH / 2.f, lineThick });
    botL.setFillColor(borderColor); // <-- dùng borderColor
    botL.setOrigin({ FIELD_WIDTH / 2.f, lineThick });
    botL.setPosition({ leftGoalBanner_.getPosition().x, leftGoalBanner_.getPosition().y + 80.f });

    // 2. Viền trên và dưới cho thanh BÊN PHẢI
    sf::RectangleShape topR({ FIELD_WIDTH / 2.f, lineThick });
    topR.setFillColor(borderColor); // <-- dùng borderColor
    topR.setOrigin({ 0.f, 0.f });
    topR.setPosition({ rightGoalBanner_.getPosition().x, rightGoalBanner_.getPosition().y - 80.f });

    sf::RectangleShape botR({ FIELD_WIDTH / 2.f, lineThick });
    botR.setFillColor(borderColor); // <-- dùng borderColor
    botR.setOrigin({ 0.f, lineThick });
    botR.setPosition({ rightGoalBanner_.getPosition().x, rightGoalBanner_.getPosition().y + 80.f });

    // Vẽ 4 dải viền
    window.draw(topL); window.draw(botL);
    window.draw(topR); window.draw(botR);

    // Chữ GOAL hoặc FOUL
    if (goalAnimState_ >= 1) {
        float alpha = 255.f;
        if (goalAnimState_ == 1) alpha = (goalAnimTimer_ / GOAL_ANIM_DURATION) * 255.f;
        if (goalAnimState_ == 3) alpha = 255.f - (goalAnimTimer_ / GOAL_ANIM_DURATION) * 255.f;

        if (isFoul) {
            goalText_.setString("FOUL!");
            goalText_.setFillColor(sf::Color(255, 30, 30, static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, alpha)))));
        }
        else {
            goalText_.setString("GOAL!");
            goalText_.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, alpha)))));
        }

        goalText_.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, alpha)))));

        // Đoạn này CỰC KỲ QUAN TRỌNG: Căn lại tâm vì chữ FOUL ngắn hơn chữ GOAL
        sf::FloatRect textBounds = goalText_.getLocalBounds();
        goalText_.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                              textBounds.position.y + textBounds.size.y / 2.f });

        window.draw(goalText_);
    }
}

} // namespace SoccerPool
