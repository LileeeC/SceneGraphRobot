cmake_minimum_required(VERSION 3.14)
project(SceneGraphRobot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# download and setup dependencies
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# download and setup GLFW
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)
find_package(OpenGL REQUIRED)
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/glad.c
    src/SceneNode.cpp
    src/Robot.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    src
)

# link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw
    glm
    OpenGL::GL
)