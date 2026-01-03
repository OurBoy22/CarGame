
cmake_minimum_required(VERSION 3.10)

add_definitions(-DPROJECT_ROOT="${CMAKE_SOURCE_DIR}")

project(my_app)

add_executable(my_app
    src/main.c
    src/io/file.c
)

# OpenGL (system)
find_package(OpenGL REQUIRED)

# vcpkg-provided libraries
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)


target_link_libraries(my_app
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        glad::glad
        # glut::glut
)