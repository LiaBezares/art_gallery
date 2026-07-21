////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <SFML/OpenGL.hpp>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "rawmouse.hh"

#include <vector>
#include <string>
#include <iostream>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertexColor;
out vec3 FragPos; // Position of the fragment in world space
out vec3 Normal; // Normal of the fragment in world space

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    FragPos = vec3(model * vec4(aPos, 1.0)); // Calculate the fragment position in world space

    Normal = mat3(transpose(inverse(model))) * aNormal; // Transform the normal to world space

    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos; // Position of the light source
uniform vec3 lightColor; // Color of the light source

void main()
{
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 finalColor = (ambient + diffuse) * vertexColor;
    FragColor = vec4(finalColor, 1.0);
}
)";

struct Scene {
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint modelLoc, viewLoc, projectionLoc;
    GLuint lightPosLoc, lightColorLoc;

    void init() {

        glEnable(GL_DEPTH_TEST);

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

        modelLoc = glGetUniformLocation(shaderProgram, "model");
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        
        lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        // Set up vertex data and buffers
        float vertices[] = {
            // == Floor (Grease) ==
            // Posicion         // Colors
            -5.0f, -1.0f, -5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f, // Left Behind
             5.0f, -1.0f, -5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f, // Right Behind
             5.0f, -1.0f,  5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f, // Right Front
            -5.0f, -1.0f,  5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,  // Left Front

            // == Back wall (Dark blue of exhibition) ==
            -5.0f, -1.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,
             5.0f, -1.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,
             5.0f,  3.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,
            -5.0f,  3.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,

            // == Left wall (White) ==
            -5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,
            -5.0f, -1.0f, -5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,
            -5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,
            -5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,

            // == Right wall (White) ==
             5.0f, -1.0f, -5.0f,   0.75f, 0.75f, 0.7f,   1.0f, 0.0f, 0.0f,
             5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,   1.0f, 0.0f, 0.0f,
             5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   1.0f, 0.0f, 0.0f,
             5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,   1.0f, 0.0f, 0.0f,
        };

        unsigned int indices[] = {
            0, 1, 2,     2, 3, 0,     // Floor
            4, 5, 6,     6, 7, 4,     // Back wall
            8, 9, 10,    10, 11, 8,   // Left wall
            12, 13, 14,  14, 15, 12   // Right wall
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Normal attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {

        glUseProgram(shaderProgram);

        glUniform3f(lightPosLoc, 0.0f, 2.5f, -1.0f); // Position of the light source
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Color of the light source  
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
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

    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cameraSpeed;

    float yaw; //Horizontal rotation
    float pitch; //Vertical rotation
    float sensitivity; //Mouse sensitivity
    bool isMouseCaptured; //Flag to check if mouse is captured for camera control
    fcg::RawMouse raw_mouse; // Raw mouse input handler

    
    State(unsigned w, unsigned h, std::string title)
        : isRunning(true),
            cameraPos(glm::vec3(0.0f, 0.0f, 3.0f)),
            cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
            cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
            cameraSpeed(0.05f),
            yaw(-90.0f),
            pitch(0.0f),
            sensitivity(0.1f),
            isMouseCaptured(true)


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

        window.setMouseCursorVisible(false); // Hide the mouse cursor
        window.setMouseCursorGrabbed(true); // Capture the mouse cursor

        sf::Mouse::setPosition(sf::Vector2i(w / 2, h / 2), window); //Center the mouse cursor

        scene.init();


    }

    void handleEvents()
    {
        while (const std::optional event = window.pollEvent())
        {
            if (window.hasFocus())
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
                {
                    window.close();
                    isRunning = false;
                }
            }

            if (const auto* mouse_moved_raw = event->getIf<sf::Event::MouseMovedRaw>())
                raw_mouse.event(*mouse_moved_raw);
        }
    

    if (window.hasFocus())
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            cameraPos += cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            cameraPos -= cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        // Capture mouse movement for camera control
        if (isMouseCaptured)
        {
            sf::Vector2f offset = raw_mouse.delta();
            float xOffset = offset.x;
            float yOffset = offset.y;

            if (xOffset != 0.0f || yOffset != 0.0f)
            {
                xOffset *= sensitivity;
                yOffset *= sensitivity;

                yaw += xOffset;
                pitch += yOffset;

                // Constrain the pitch angle to prevent flipping
                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;

                // Update camera front vector based on yaw and pitch
                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cameraFront = glm::normalize(front);

            }
        }
    }   
}
///
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Callback functions
void update() {}

void render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    sf::Vector2u windowSize = window.getSize();
    float aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    scene.draw(model, view, projection);

    window.display();
}

    ~State() 
        {
            scene.cleanup();
        }
};


int main()
{
    State state(800, 600, "3D Virtual Art Gallery - Tappa 05");

    while (state.isRunning) // main loop
    {
        state.handleEvents();
        state.update();
        state.render();
    }

    return 0;
}
