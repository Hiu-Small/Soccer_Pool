#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "GameState.h"
#include "Game_Render.h"
#include "Game_Controller.h"
#include "Constants.h"
#include <optional>

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 1000, 600 }), "Soccer Pool - MVC");
    window.setFramerateLimit(60);

    SoccerPool::GameState state;
    SoccerPool::Game_Render view;
    SoccerPool::Game_Controller controller(state, view);

    view.setState(&state);
    controller.setViewSize(1000, 600);

    sf::Clock clock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else
                controller.handleEvent(*event, window);
        }

        float dt = clock.restart().asSeconds();
        controller.update(dt);

        window.clear(sf::Color(20, 20, 30));
        controller.draw(window);
        window.display();
    }
    return 0;
}
