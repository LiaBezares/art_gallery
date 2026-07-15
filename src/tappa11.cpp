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

#define NR_SPOTLIGHTS 3

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float cutOff;
    float outerCutOff;
};

uniform SpotLight spotLight[NR_SPOTLIGHTS];
uniform vec3 ambientColor;
uniform sampler2D ourTexture; // Texture sampler

void main()
{
    vec3 norm = normalize(Normal);
    vec4 texColor = texture(ourTexture, TexCoord); // Sample the texture color

    vec3 resultColor = ambientColor * texColor.rgb; // Start with ambient color

    for (int i = 0; i < NR_SPOTLIGHTS; ++i) {

        vec3 lightDir = normalize(spotLight[i].position - FragPos);

        float theta = dot(lightDir, normalize(-spotLight[i].direction));
        float epsilon = spotLight[i].cutOff - spotLight[i].outerCutOff;
        float intensity = clamp((theta - spotLight[i].outerCutOff) / epsilon, 0.0, 1.0);

        float diff = max(dot(norm, lightDir), 0.0);

        float distance = length(spotLight[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.045 *distance + 0.0075 * distance * distance); // Inverse square law

        vec3 diffuse = diff * spotLight[i].color * intensity * attenuation;
        resultColor += diffuse * texColor.rgb;
    
    }

    FragColor = vec4(resultColor, 1.0);
}
)";

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float cutOff;
    float outerCutOff;
};

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
    std::vector<SpotLight> spotLights;

    GLuint floorTexture;
    GLuint backWallTexture;
    GLuint sideWallTexture;
    GLuint cellingTexture;
    std::vector<Painting> paintings;

    GLuint paintingVAO, paintingVBO, paintingEBO;

    // Imprime en consola el error de compilacion de un shader, si lo hay
    void checkShaderCompileErrors(GLuint shader, const char* type) {
        GLint success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR: fallo al compilar el shader (" << type << ")\n" << infoLog << std::endl;
        }
    }

    // Imprime en consola el error de linkeo del programa, si lo hay
    void checkProgramLinkErrors(GLuint program) {
        GLint success;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            std::cerr << "ERROR: fallo al linkear el programa de shaders\n" << infoLog << std::endl;
        }
    }

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
        checkShaderCompileErrors(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderCompileErrors(fragmentShader, "FRAGMENT");

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        checkProgramLinkErrors(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        modelLoc = glGetUniformLocation(shaderProgram, "model");
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        
        spotLights.push_back(SpotLight{
            glm::vec3(0.0f, 2.9f, -3.9f), // Position cuadro 1
            glm::normalize(glm::vec3(0.0f, -1.2f, -1.0f)), // Direction
            glm::vec3(1.0f,0.95f,0.85f), // Color
            glm::cos(glm::radians(25.0f)), // CutOff
            glm::cos(glm::radians(35.0f))  // OuterCutOff
        });

        spotLights.push_back(SpotLight{
            glm::vec3(-3.9f, 2.9f, 0.0f), // Position cuadro 2 Left
            glm::normalize(glm::vec3(-1.0f, -1.2f, 0.0f)), // Direction
            glm::vec3(1.0f,0.95f,0.85f), // Color
            glm::cos(glm::radians(25.0f)), // CutOff
            glm::cos(glm::radians(35.0f))  // OuterCutOff
        });

        spotLights.push_back(SpotLight{
            glm::vec3(3.9f, 2.9f, 0.0f), // Position cuadro 3 Right
            glm::normalize(glm::vec3(1.0f, -1.2f, 0.0f)), // Direction
            glm::vec3(1.0f,0.95f,0.85f), // Color
            glm::cos(glm::radians(25.0f)), // CutOff
            glm::cos(glm::radians(35.0f))  // OuterCutOff
        });

        glUseProgram(shaderProgram);

        glUniform3f(glGetUniformLocation(shaderProgram, "ambientColor"), 0.6f, 0.6f, 0.6f); // Set ambient color

        for (size_t i = 0; i < spotLights.size(); ++i) {
            std::string indexStr = std::to_string(i);
            glUniform3fv(glGetUniformLocation(shaderProgram, ("spotLight[" + indexStr + "].position").c_str()), 1, glm::value_ptr(spotLights[i].position));
            glUniform3fv(glGetUniformLocation(shaderProgram, ("spotLight[" + indexStr + "].direction").c_str()), 1, glm::value_ptr(spotLights[i].direction));
            glUniform3fv(glGetUniformLocation(shaderProgram, ("spotLight[" + indexStr + "].color").c_str()), 1, glm::value_ptr(spotLights[i].color));
            glUniform1f(glGetUniformLocation(shaderProgram, ("spotLight[" + indexStr + "].cutOff").c_str()), spotLights[i].cutOff);
            glUniform1f(glGetUniformLocation(shaderProgram, ("spotLight[" + indexStr + "].outerCutOff").c_str()), spotLights[i].outerCutOff);
        }


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

            // == Front wall (Cuarta pared, cierra la sala en z=5) ==
            -5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
             5.0f, -1.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
             5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
            -5.0f,  3.0f,  5.0f,   0.75f, 0.75f, 0.7f,   0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
        };

        unsigned int indices[] = {
            0, 1, 2,     2, 3, 0,     // Floor
            4, 5, 6,     6, 7, 4,     // Back wall
            8, 9, 10,    10, 11, 8,   // Left wall
            12, 13, 14,  14, 15, 12,   // Right wall
            16, 17, 18,  18, 19, 16,  // Ceiling
            20, 21, 22,  22, 23, 20   // Front wall
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
        cellingTexture = loadTexture("../src/textures/Poliigon_TilesCeramicWhite_6956_BaseColor.jpg");


        GLuint texture1 = loadTexture("../src/textures/painting1.jpg");
        GLuint texture2 = loadTexture("../src/textures/painting2.jpg");
        GLuint texture3 = loadTexture("../src/textures/painting3.jpg");

        paintings.push_back({ texture1, glm::vec3(0.0f, 1.7f, -4.9f), glm::vec3(2.76f, 3.5f, 1.0f), 0.0f });
        paintings.push_back({ texture2, glm::vec3(-4.9f, 1.7f, 0.0f), glm::vec3(1.39f, 3.2f, 1.0f), 90.0f });
        paintings.push_back({ texture3, glm::vec3(4.9f, 1.7f, 0.0f), glm::vec3(1.31f, 3.52f, 1.0f), -90.0f });
    }

    void draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {

        glUseProgram(shaderProgram);

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

        glBindTexture(GL_TEXTURE_2D, cellingTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(24 * sizeof(unsigned int)));

        glBindTexture(GL_TEXTURE_2D, backWallTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(30 * sizeof(unsigned int)));

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
        glDeleteTextures(1, &cellingTexture);
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

        sf::Mouse::setPosition(sf::Vector2i(w / 2, h / 2), window); //Center the mouse cursor

        scene.init();


    }

    void handleEvents()
    {
        while (window.pollEvent())
        {
            if (window.hasFocus())
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
                {
                    window.close();
                    isRunning = false;
                }
            }
        }
    

    if (window.hasFocus())
    {

        glm::vec3 moveForward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        glm::vec3 moveRight = glm::normalize(glm::cross(moveForward, cameraUp));

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            cameraPos += cameraSpeed * moveForward;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            cameraPos -= cameraSpeed * moveForward;
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
            sf::Vector2u windowSize = window.getSize();
            sf::Vector2i centerPos(windowSize.x / 2, windowSize.y / 2);
            sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);

            float xOffset = static_cast<float>(currentMousePos.x - centerPos.x);
            float yOffset = static_cast<float>(centerPos.y - currentMousePos.y);

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

                // Reset mouse position to the center of the window
                sf::Mouse::setPosition(centerPos, window);
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
    State state(800, 600, "3D Virtual Art Gallery - Tappa 11");

    while (state.isRunning) // main loop
    {
        state.handleEvents();
        state.update();
        state.render();
    }

    return 0;
}