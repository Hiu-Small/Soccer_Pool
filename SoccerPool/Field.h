#pragma once

#include <SFML/System/Vector2.hpp>
#include "Constants.h"

namespace SoccerPool {

// Biên sân và vùng khung thành (để kiểm tra bàn thắng / va chạm tường)
class Field {
public:
    Field();

    // Khoảng di chuyển hợp lệ (hình chữ nhật, trừ khung thành)
    float getLeft() const { return 0.f; }
    float getRight() const { return FIELD_WIDTH; }
    float getTop() const { return 0.f; }
    float getBottom() const { return FIELD_HEIGHT; }

    // Khung thành Team1 bên trái (x < ...), Team2 bên phải (x > ...)
    float getGoal1Left() const { return 0.f; }
    float getGoal1Right() const { return 0.f; }  // chỉ là đường cầu môn
    float getGoal2Left() const { return FIELD_WIDTH; }
    float getGoal2Right() const { return FIELD_WIDTH; }

    // Vùng vào lưới (trục x và y)
    bool isInGoal1(sf::Vector2f pos) const;
    bool isInGoal2(sf::Vector2f pos) const;

    // Clamp vị trí vào trong sân (trừ ô cầu môn)
    void clampToField(sf::Vector2f& pos, float radius) const;
};

} // namespace SoccerPool
