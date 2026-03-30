 #pragma once

#include "GameState.h"
#include "Game_Render.h"
#include "AIPlayer.h"
#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>
#include <memory>
#include <optional>

namespace SoccerPool {

class Game_Controller {
public:
    Game_Controller(GameState& state, Game_Render& view);

    void setViewSize(unsigned width, unsigned height);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    //oid handleMouseInput(sf::Event& event, sf::RenderWindow& window);
    void handlePickLineup(sf::Vector2f mPos);

private:
    void startGameWithMode(int menuChoice);
    void onGoalScored(Team scoringTeam);
    void onGameOver(Team winner);
    void endTurn();
    void tryShoot(sf::Vector2f velocity);
    int getPieceIndexAt(sf::Vector2f worldPos) const;
    bool isCurrentPlayerHuman() const;
    void triggerAITurn();

    GameState& state_;
    Game_Render& view_;
    std::unique_ptr<AIPlayer> aiPlayer1_;
    std::unique_ptr<AIPlayer> aiPlayer2_;

    unsigned viewWidth_ = 1000;
    unsigned viewHeight_ = 600;

    bool dragging_ = false;
    int selectedPieceIndex_ = -1;
    sf::Vector2f dragStart_;
    sf::Vector2f dragCurrent_;
    sf::Vector2f lastMouseWorld_;

    float aiThinkTimer_ = 0.f;
    static const float AI_DELAY_SEC;

    std::optional<sf::Cursor> handCursor_;    // Dùng optional
    std::optional<sf::Cursor> defaultCursor_;
    bool isMouseOverInteractive(sf::Vector2f mousePos) const;
};

} // namespace SoccerPool
