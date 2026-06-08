cmake_minimum_required(VERSION 3.15)
project(ArtGallery)

# Asegúrate de que esta ruta apunte a la carpeta donde está 'SFMLConfig.cmake'
set(SFML_DIR "C:/SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit/SFML-3.0.2/lib/cmake/SFML")

# En SFML 3, los componentes se buscan así:
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(main_app src/main.cpp)
# add_executable(main_app src/04b_hits.cpp)

# Enlazamos usando el espacio de nombres SFML::
target_link_libraries(main_app SFML::Graphics SFML::Window SFML::System)
