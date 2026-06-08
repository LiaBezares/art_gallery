////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <SFML/OpenGL.hpp>
#include "glm/glm.hpp"

#include <vector>
#include <string>
#include <iostream>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vertexColor;
void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)";

struct Scene {
    GLuint shaderProgram;
    GLuint VAO, VBO;

    void init() {
        // Compile shaders and link shader program
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);
        // Check for compilation errors...

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        // Check for compilation errors...

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // Check for linking errors...

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Set up vertex data and buffers
        float vertices[] = {
            // positions         // colors
             0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void render() {
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};

////////////////////////////////////////////////////////////
/// GUI State

struct State
{
    // General resources
    sf::RenderWindow window;
    bool isRunning;
    Scene scene;

    
    State(unsigned w, unsigned h, std::string title)
        : isRunning(true)


    {

        sf::ContextSettings settings;
        settings.depthBits = 24;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.majorVersion = 3;
        settings.minorVersion = 3;

        window.create(sf::VideoMode({w, h}), sf::String(title), sf::State::Windowed, settings);
        window.setFramerateLimit(60);

        (void)window.setActive(true);

        if (!gladLoadGL(sf::Context::getFunction))
        {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            isRunning = false;
            return;
        }

        scene.init();


    }

        ~State()
        {
            scene.cleanup();
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
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gs.scene.render();
    
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
