cmake_minimum_required(VERSION 3.15)
project(ArtGallery)

# Asegúrate de que esta ruta apunte a la carpeta donde está 'SFMLConfig.cmake'
set(SFML_DIR "C:/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit/SFML-3.0.2/lib/cmake/SFML")

# En SFML 3, los componentes se buscan así:
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

find_package(OpenGL REQUIRED)


# add_executable(main_app src/tappa00.cpp)
# add_executable(Tappa01 src/tappa01.cpp)
# add_executable(Tappa02 src/tappa02.cpp)
# add_executable(Tappa03 src/tappa03.cpp)
# add_executable(Tappa04 src/tappa04.cpp)
add_executable(Tappa05 src/tappa05.cpp)

target_link_libraries(Tappa05 
    PRIVATE
    SFML::Graphics
    SFML::Window 
    SFML::System 
    OpenGL::GL
    )
