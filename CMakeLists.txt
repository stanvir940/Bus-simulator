cmake_minimum_required(VERSION 3.10)
project(BusStandSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(BusStandSimulator
    main.cpp
    scene.cpp
    bus.cpp
    camera.cpp
)

if(APPLE)
    target_compile_options(BusStandSimulator PRIVATE -Wno-deprecated-declarations)
    target_link_libraries(BusStandSimulator
        "-framework OpenGL"
        "-framework GLUT"
    )
else()
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    target_include_directories(BusStandSimulator PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
    target_link_libraries(BusStandSimulator ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
endif()
