cmake_minimum_required(VERSION 3.21) # 3.21+ necesario para $<TARGET_RUNTIME_DLLS:...>

# Algunas dependencias internas de SFML (como freetype) declaran un
# cmake_minimum_required muy viejo; esto evita que fallen con CMake 4+.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for all subprojects")

project(ArtGallery LANGUAGES CXX)

# Fuerza a MinGW a empaquetar la librería estándar de C y C++ dentro del propio .exe
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++ -static)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# =========================================================
# SFML (via FetchContent: se compila desde el codigo fuente,
# igual en Windows, Linux y Mac -- no depende de una instalacion
# local en una ruta fija)
# =========================================================

# Forzar SFML como libreria COMPARTIDA (DLL/.so), no estatica.
# Si se compila estatica, la copia interna de stb_image que usa
# SFML (para sf::Image) queda embebida en el .exe junto con la
# nuestra propia, y el linker choca por simbolos duplicados.
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

set(SFML_BUILD_AUDIO OFF CACHE INTERNAL "")
set(SFML_BUILD_NETWORK OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Le dice al compilador desde CMake que trate stb_image como estático.
# ¡Cero cambios en tus archivos .cpp!
add_compile_definitions(STB_IMAGE_STATIC)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# =========================================================
# GLM
# =========================================================
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
    SYSTEM
)
FetchContent_MakeAvailable(glm)

# =========================================================
# IMGUI + IMGUI-SFML (solo lo usa Tappa10)
# =========================================================
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1
    GIT_SHALLOW ON
    SYSTEM
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui core headers" FORCE)

FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/eliasdaler/imgui-sfml.git
    GIT_TAG v3.0
    GIT_SHALLOW ON
    SYSTEM
)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_USE_SYSTEM_IMGUI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(imgui-sfml)

find_package(OpenGL REQUIRED)

# =========================================================
# Funcion auxiliar: crea el ejecutable de una etapa, la linkea
# con SFML + OpenGL + glad + glm, y copia automaticamente las
# DLLs de runtime necesarias (SFML, etc.) al lado del .exe tras
# cada build, para no tener que copiarlas a mano.
# =========================================================
function(add_gallery_stage NAME SOURCE)
    if(APPLE)
        add_executable(${NAME} MACOSX_BUNDLE ${SOURCE})
    else()
        add_executable(${NAME} ${SOURCE})
    endif()

    target_include_directories(${NAME} PRIVATE src)

    target_link_libraries(${NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        glm::glm
        OpenGL::GL
    )

    #if(WIN32)
    #    add_custom_command(TARGET ${NAME} POST_BUILD
    #        COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #            $<TARGET_RUNTIME_DLLS:${NAME}>
    #            $<TARGET_FILE_DIR:${NAME}>
    #        COMMAND_EXPAND_LISTS
    #    )
    #endif()

    if(APPLE)
        target_link_libraries(${NAME} PRIVATE
            "-framework Cocoa"
            "-framework OpenGL"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    endif()
endfunction()

# =========================================================
# Etapas del proyecto
# =========================================================
add_gallery_stage(Tappa00 src/tappa00.cpp)
add_gallery_stage(Tappa01 src/tappa01.cpp)
add_gallery_stage(Tappa02 src/tappa02.cpp)
add_gallery_stage(Tappa03 src/tappa03.cpp)
add_gallery_stage(Tappa04 src/tappa04.cpp)
add_gallery_stage(Tappa05 src/tappa05.cpp)
add_gallery_stage(Tappa06 src/tappa06.cpp)
add_gallery_stage(Tappa07 src/tappa07.cpp)
add_gallery_stage(Tappa08 src/tappa08.cpp)
add_gallery_stage(Tappa09 src/tappa09.cpp)
add_gallery_stage(Tappa10 src/tappa10.cpp)
add_gallery_stage(Tappa11 src/tappa11.cpp)
add_gallery_stage(Tappa12 src/tappa12.cpp)

# Tappa10 usa ademas la interfaz grafica ImGui
target_link_libraries(Tappa10 PRIVATE ImGui-SFML::ImGui-SFML)

# =========================================================
# TEXTURAS
# (si tambien usas una carpeta "resources" con fuentes como
#  tuffy.ttf, añade aqui otra linea file(COPY resources ...) )
# =========================================================
# Copia las texturas dentro de build/src para que la ruta relativa ../src/textures funcione siempre
file(COPY ${CMAKE_SOURCE_DIR}/src/textures DESTINATION ${CMAKE_BINARY_DIR}/src)
