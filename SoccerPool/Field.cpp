#include "Field.h"
#include "Constants.h"
#include <algorithm>
#include <cmath>

namespace SoccerPool {

Field::Field() {}

// Ghi bàn khi mép bóng (hoặc tâm) đã qua vạch khung thành
bool Field::isInGoal1(sf::Vector2f pos) const {
    if (pos.x > FIELD_MARGIN_X + BALL_RADIUS) return false;
    float gy = GOAL_Y_OFFSET;
	//printf("Checking Goal1: pos=(%.1f, %.1f), gy=%.1f, gy+height=%.1f\n", pos.x, pos.y, gy, gy + GOAL_HEIGHT);
    return pos.y >= gy && pos.y <= gy + GOAL_HEIGHT;
}

bool Field::isInGoal2(sf::Vector2f pos) const {
    if (pos.x < (FIELD_WIDTH - FIELD_MARGIN_X) - BALL_RADIUS) return false;
    float gy = GOAL_Y_OFFSET;
	//printf("Checking Goal2: pos=(%.1f, %.1f), gy=%.1f, gy+height=%.1f\n", pos.x, pos.y, gy, gy + GOAL_HEIGHT);
    return pos.y >= gy && pos.y <= gy + GOAL_HEIGHT;
}


//ham nay hien tai dang ko dung den
void Field::clampToField(sf::Vector2f& pos, float radius) const {
    //printf("Check pos ball: pos=(%.1f, %.1f)", pos.x, pos.y );
    // Trục Y: toàn bộ chiều cao
    pos.y = std::max(FIELD_MARGIN_Y + radius, std::min(FIELD_HEIGHT - FIELD_MARGIN_Y_BOTTOM - radius, pos.y));
    // Trục X: trừ vùng khung thành (chỉ cho phép trong "thủ môn" về phía sân)
    float leftLimit = 0.f;
    float rightLimit = FIELD_WIDTH;
    // Cầu môn trái: từ 0 đến GOAL_WIDTH là vùng cấm (trừ khi trong khung thành)
    if (pos.x < 0.f) {
        pos.x = 0.f;
        return;
    }
    if (pos.x > FIELD_WIDTH) {
        pos.x = FIELD_WIDTH;
        return;
    }
    float margin = radius + 2.f;
    if (pos.x < margin) pos.x = margin;
    if (pos.x > FIELD_WIDTH - margin) pos.x = FIELD_WIDTH - margin;
}

} // namespace SoccerPool
