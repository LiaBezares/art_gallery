cmake_minimum_required(VERSION 3.15)
project(ArtGallery)

# Rutas y paquetes
set(SFML_DIR "C:/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit/SFML-3.0.2/lib/cmake/SFML")
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(OpenGL REQUIRED)

# --- Define la etapa en la que estás trabajando aquí ---
set(ACTIVE_TAPPA Tappa10)
set(ACTIVE_SRC src/tappa10.cpp)
# -------------------------------------------------------

add_executable(${ACTIVE_TAPPA} ${ACTIVE_SRC})

target_link_libraries(${ACTIVE_TAPPA}
    PRIVATE
    SFML::Graphics
    SFML::Window 
    SFML::System 
    OpenGL::GL
)

add_custom_command(TARGET ${ACTIVE_TAPPA} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${ACTIVE_TAPPA}>
        $<TARGET_FILE_DIR:${ACTIVE_TAPPA}>
    COMMAND_EXPAND_LISTS
)