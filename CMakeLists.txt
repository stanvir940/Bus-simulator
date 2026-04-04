cmake_minimum_required(VERSION 3.10)
project(BusStandSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(BusStandSimulator
    main.cpp
    scene.cpp
    bus.cpp
    camera.cpp
    texture_util.cpp
    shader_program.cpp
    lighting.cpp
    renderer.cpp
)

target_compile_definitions(BusStandSimulator PRIVATE
    "SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\""
)

target_include_directories(BusStandSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(BusStandSimulator PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
)

if(APPLE)
    target_compile_options(BusStandSimulator PRIVATE -Wno-deprecated-declarations)
endif()
