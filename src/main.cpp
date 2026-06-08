////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>

#include <vector>
#include <string>
#include <iostream>

////////////////////////////////////////////////////////////
/// GUI State

struct State
{
    // General resources
    sf::RenderWindow window;
    bool isRunning;

    
    State(unsigned w, unsigned h, std::string title)
        : window(sf::VideoMode({w, h}), title),
            isRunning(true)

    {
        window.setFramerateLimit(60);

    }

};
///
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Callback functions
void handle(const sf::Event::Closed &, State &gs)
{
    gs.window.close();
    gs.isRunning = false;
}

void handle(const sf::Event::KeyPressed& keyPressed, State &gs)
{
    if (keyPressed.code == sf::Keyboard::Key::Escape)
    {
        gs.window.close();
        gs.isRunning = false;
    }
}

template <typename T>
void handle(const T &, State &gs){}

///
////////////////////////////////////////////////////////////
/// Graphics
void doGraphics(State &gs)
{
    gs.window.clear(sf::Color::Black);
    
    gs.window.display();
}


int main()
{
    State gs(800, 600, "3D Virtual Art Gallery - Tappa 01");

    while (gs.window.isOpen()) // main loop
    {
        // event loop and handler through callbacks
        gs.window.handleEvents([&](const auto &event)
                               { handle(event, gs); });
                    

        doGraphics(gs);
    }

    return 0;
}
