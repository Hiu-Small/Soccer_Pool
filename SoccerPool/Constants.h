#pragma once

#include <cstddef>
#include <vector>
#include <SFML/Graphics/Color.hpp>

namespace SoccerPool {

// Kích thước sân (pixel)
const float FIELD_WIDTH = 1000.f;
const float FIELD_HEIGHT = 500.f;

// --- THÊM 2 DÒNG NÀY ---
const float FIELD_MARGIN_X = 156.f; // Khoảng cách từ mép ảnh đến vạch vôi dọc
const float FIELD_MARGIN_Y = 85.f; // Khoảng cách từ mép ảnh đến vạch vôi ngang
const float FIELD_MARGIN_Y_BOTTOM = 20.f; // Khoảng cách từ mép ảnh đến vạch vôi ngang

// Khung thành
const float GOAL_WIDTH = 120.f;
const float GOAL_HEIGHT = 105.f;
const float GOAL_Y_OFFSET = (FIELD_HEIGHT - GOAL_HEIGHT) / 2.f + 35.f; //Giúp đặt khung thành nằm chính giữa theo chiều dọc của sân.
// Vạch sân (khu cấm địa, khu thành)
const float PENALTY_AREA_DEPTH = 90.f; //Độ sâu của vòng cấm địa (dùng để vẽ trang trí vạch vôi trên sân).
const float GOAL_AREA_DEPTH = 32.f; //Độ sâu của vòng cấm địa (dùng để vẽ trang trí vạch vôi trên sân).
const float LINE_WIDTH = 3.f; //Độ dày của các đường kẻ vạch trắng trên sân (3 pixel).

// Vật thể
const float BALL_RADIUS = 16.f;
const float PIECE_RADIUS = 25.f; //Bán kính quân cờ (cầu thủ).

// Vật lý (tăng tốc: lực sút mạnh hơn, ma sát nhẹ hơn)
const float FRICTION = 0.987f;
const float MIN_VELOCITY_PIECE = 5.0f;
const float MIN_VELOCITY_BALL =  10.5f;
const float WALL_BOUNCE = 0.5f; //Độ nảy của tường.
const float BALL_MASS = 1.0f; //Khối lượng.
const float PIECE_MASS = 10.f; //Khối lượng.
const float MAX_SHOOT_POWER = 600.f; //Lực sút tối đa
const float DRAG_POWER_FACTOR = 3.0f; //Độ nhạy khi kéo chuột
const float MAX_VISUAL_DRAG = 90.f; //Giới hạn độ dài hiển thị của đường kéo (vòng tròn max lực)

// Hệ số giữ lại vận tốc sau va chạm (0.0 đến 1.0)
// 1.0 là không mất gì, 0.7 là mất 30% vận tốc ngay lập tức
const float COLLISION_RESISTANCE_PIECE = 0.85f; // Cản khi cầu thủ đâm nhau
const float COLLISION_RESISTANCE_BALL = 0.92f;  // Cản khi chạm bóng

// Số quân / bàn thắng
const int MIN_PIECES = 2;
const int MAX_PIECES = 5;
const int DEFAULT_GOALS_TO_WIN = 2;
const int MIN_GOALS_TO_WIN = 1;
const int MAX_GOALS_TO_WIN = 5;

// Các sơ đồ chiến thuật: mỗi phần tử = số quân ở hàng (từ khung thành ra)
// Ví dụ: {1, 2, 2} = 1 thủ môn, 2 hàng 2
using Formation = std::vector<int>; //Đây là hàm giúp sắp xếp đội hình

inline Formation getFormation(int lineUp) {
    switch (lineUp) {
        //case 2: return {1, 1};
        //case 3: return {1, 2};
        case 1: return {1, 1, 3};
        case 0: return {1, 2, 2};
        case 2: return {1,2,1,1 };//eagle
        case 3: return {1, 3, 1 };//psycho
        case 4: return { 0, 3, 2 };//moon
		case 5: return { 0, 2, 3 };//wave
        case 6: return { 1,1,1,2 };//edge
        default: return { 1, 2, 2 };
    }
}

// Màu đội
const sf::Color TEAM1_COLOR = sf::Color(0, 0, 255); // Xanh dương
const sf::Color TEAM2_COLOR = sf::Color(255, 0, 0);  // Đỏ

// Thời gian mỗi lượt (giây), 0 = không giới hạn
const float TURN_TIME_LIMIT = 30.f;



// Constants.h

// Vị trí bảng tỉ số
const float SB_ANCHOR_Y = 40.f;
const float SB_SCALE_FACTOR = 0.35f; // Tỉ lệ thu nhỏ (tương ứng targetWidth = 350px)

// Khoảng cách từ TÂM bảng tỉ số đến các ô trống (tính bằng pixel ảnh gốc sau khi scale)
// Bạn có thể điều chỉnh các số này một chút nếu chữ chưa nằm chính giữa ô
const sf::Vector2f SB_OFFSET_SCORE1 = { -152.f, 4.f }; // Ô tròn trái
const sf::Vector2f SB_OFFSET_SCORE2 = { 150.f, 3.f }; // Ô tròn phải
const sf::Vector2f SB_OFFSET_TIME = { 0.f,   2.f }; // Ô chữ nhật giữa

const unsigned int SB_FONT_SIZE_SCORE = 22;
const unsigned int SB_FONT_SIZE_TIME = 18;

// THÊM CÁC ENUM QUẢN LÝ MÀN HÌNH
//enum class GameMode { PvP, PvAI, AIvsAI };
//enum class GamePhase { MainMenu, SelectMode, Playing, GoalScored, GameOver };
//enum class AIDifficulty { Easy, Medium, Hard };


} // namespace SoccerPool
