////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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
layout (location = 3) in vec2 aTexCoord;

out vec3 vertexColor;
out vec3 FragPos; // Position of the fragment in world space
out vec3 Normal; // Normal of the fragment in world space
out vec2 TexCoord; // Texture coordinates

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    FragPos = vec3(model * vec4(aPos, 1.0)); // Calculate the fragment position in world space

    Normal = mat3(transpose(inverse(model))) * aNormal; // Transform the normal to world space

    TexCoord = aTexCoord; // Pass the texture coordinates to the fragment shader
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos; // Position of the light source
uniform vec3 lightColor; // Color of the light source
uniform sampler2D ourTexture; // Texture sampler

void main()
{
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec4 texColor = texture(ourTexture, TexCoord); // Sample the texture color

    vec3 finalColor = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(finalColor, 1.0);
}
)";

struct Painting {
    GLuint texture;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationY;
};

struct Scene {
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint modelLoc, viewLoc, projectionLoc;
    GLuint lightPosLoc, lightColorLoc;

    GLuint floorTexture;
    GLuint backWallTexture;
    GLuint sideWallTexture;
    std::vector<Painting> paintings;

    GLuint paintingVAO, paintingVBO, paintingEBO;

    GLuint loadTexture(const char* path) {
        GLuint textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;

        stbi_set_flip_vertically_on_load(true); // Flip the texture vertically

        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) 
                format = GL_RGB;
            else if (nrComponents == 4) 
                format = GL_RGBA;
       

            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "Texture loaded: " << path << std::endl;
        }
        else {
            std::cerr << "Failed to load texture: " << path << std::endl;
            stbi_image_free(data);
        }
        return textureID;
    }

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
            -5.0f, -1.0f, -5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f, // Left Behind
             5.0f, -1.0f, -5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Right Behind
             5.0f, -1.0f,  5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // Right Front
            -5.0f, -1.0f,  5.0f,   0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   // Left Front

            // == Back wall (Dark blue of exhibition) ==
            -5.0f, -1.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
             5.0f, -1.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
             5.0f,  3.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
            -5.0f,  3.0f, -5.0f,   0.15f, 0.2f, 0.3f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

            // == Left wall (White) ==
            -5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
            -5.0f, -1.0f, -5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
            -5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
            -5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

            // == Right wall (White) ==
             5.0f, -1.0f, -5.0f,   0.75f, 0.75f, 0.7f,   -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
             5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,   -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
             5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
             5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,   -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

             // == Ceiling (reutiliza textura de yeso de las paredes) ==
            -5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
             5.0f,  3.0f, -5.0f,   0.75f, 0.75f, 0.7f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
             5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
            -5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
        };

        unsigned int indices[] = {
            0, 1, 2,     2, 3, 0,     // Floor
            4, 5, 6,     6, 7, 4,     // Back wall
            8, 9, 10,    10, 11, 8,   // Left wall
            12, 13, 14,  14, 15, 12,   // Right wall
            16, 17, 18,  18, 19, 16   // Ceiling
        };

        float paintingVertices[] = {
            //Posicion         // Colors          // Normals        // Texture Coords
            -0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Top Left
             0.5f, -0.5f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // Top Right
             0.5f, 0.0f,  0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // Bottom Right
            -0.5f, 0.0f,  0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,  // Bottom Left 
        };

        unsigned int paintingIndices[] = {
            0, 1, 2,     2, 3, 0
        };

        // 1. CONFIGURAR SALA (VAO)
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Atributos de la Sala
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);


        // 2. CONFIGURAR CUADRO (paintingVAO) COMPLETAMENTE SEPARADO
        glGenVertexArrays(1, &paintingVAO);
        glGenBuffers(1, &paintingVBO);
        glGenBuffers(1, &paintingEBO);

        glBindVertexArray(paintingVAO);
        glBindBuffer(GL_ARRAY_BUFFER, paintingVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(paintingVertices), paintingVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, paintingEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(paintingIndices), paintingIndices, GL_STATIC_DRAW);

        // Atributos del Cuadro (se aplican al paintingVAO activo)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        floorTexture = loadTexture("../src/textures/WoodFloor053_1K-JPG_Color.jpg");
        backWallTexture = loadTexture("../src/textures/Fabric023_1K-JPG_Color.jpg");
        sideWallTexture = loadTexture("../src/textures/Plaster002_1K-JPG_Color.jpg");

        GLuint texture1 = loadTexture("../src/textures/painting1.jpg");
        GLuint texture2 = loadTexture("../src/textures/painting2.jpg");
        GLuint texture3 = loadTexture("../src/textures/painting3.jpg");

        paintings.push_back({ texture1, glm::vec3(0.0f, 1.7f, -4.9f), glm::vec3(2.76f, 3.5f, 1.0f), 0.0f });
        paintings.push_back({ texture2, glm::vec3(-4.9f, 1.7f, 0.0f), glm::vec3(1.39f, 3.2f, 1.0f), 90.0f });
        paintings.push_back({ texture3, glm::vec3(4.9f, 1.7f, 0.0f), glm::vec3(1.31f, 3.52f, 1.0f), -90.0f });
    }

    void draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {

        glUseProgram(shaderProgram);

        glUniform3f(lightPosLoc, 0.0f, 2.5f, -1.0f); // Position of the light source
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Color of the light source  
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0); 
        glActiveTexture(GL_TEXTURE0); 

        // --- PASO 1: DIBUJAR LA SALA ---
        glBindVertexArray(VAO);

        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));

        glBindTexture(GL_TEXTURE_2D, backWallTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(unsigned int)));

        glBindTexture(GL_TEXTURE_2D, sideWallTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(unsigned int)));

        glBindTexture(GL_TEXTURE_2D, sideWallTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(18 * sizeof(unsigned int)));

        glBindTexture(GL_TEXTURE_2D, sideWallTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(24 * sizeof(unsigned int)));

        // --- PASO 2: DIBUJAR EL CUADRO ---
        glBindVertexArray(paintingVAO);

        for (const auto& painting : paintings)
        {
            glm::mat4 paintingModel = model;
            paintingModel = glm::translate(paintingModel, painting.position);

            if (painting.rotationY != 0.0f)
                paintingModel = glm::rotate(paintingModel, glm::radians(painting.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

            paintingModel = glm::scale(paintingModel, painting.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(paintingModel));

            glBindTexture(GL_TEXTURE_2D, painting.texture);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteTextures(1, &floorTexture);
        glDeleteTextures(1, &backWallTexture);
        glDeleteTextures(1, &sideWallTexture);
        glDeleteTextures(static_cast<GLsizei>(paintings.size()), reinterpret_cast<const GLuint*>(paintings.data()));
        glDeleteVertexArrays(1, &VAO);
        glDeleteVertexArrays(1, &paintingVAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &paintingVBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &paintingEBO);
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
    fcg::RawMouse raw_mouse;

    
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

        glm::vec3 moveForward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        glm::vec3 moveRight = glm::normalize(glm::cross(moveForward, cameraUp));

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            cameraPos += cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            cameraPos -= cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            cameraPos -= moveRight * cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            cameraPos += moveRight * cameraSpeed;

        cameraPos.y = 1.00f; // Keep the camera at a fixed height (y=1) to simulate walking on the floor

        const float wallMargin = 0.3; // Margin to prevent the camera from going through walls
        cameraPos.x = glm::clamp(cameraPos.x, -5.0f + wallMargin, 5.0f - wallMargin);
        cameraPos.z = glm::clamp(cameraPos.z, -5.0f + wallMargin, 5.0f - wallMargin);


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
    State state(800, 600, "3D Virtual Art Gallery - Tappa 9");

    while (state.isRunning) // main loop
    {
        state.handleEvents();
        state.update();
        state.render();
    }

    return 0;
}
