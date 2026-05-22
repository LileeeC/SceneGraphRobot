cmake_minimum_required(VERSION 3.14)
project(SceneGraphRobot LANGUAGES C CXX)

# 設定 C++ 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 自動下載並設定 GLM (數學庫) ---
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8 # 或你需要的版本
)
FetchContent_MakeAvailable(glm)

# --- 2. 自動下載並設定 GLFW (視窗庫) ---
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# --- 3. 尋找系統內建的 OpenGL ---
find_package(OpenGL REQUIRED)

# --- 4. 定義你的執行檔與原始碼 ---
# 包含你自己寫的 src/main.cpp 以及 glad.c
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/glad.c
    src/SceneNode.cpp
    src/Robot.cpp
)

# --- 5. 設定標頭檔搜尋路徑 (Include Directories) ---
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    src
)

# --- 6. 連結所有函式庫 (Link Libraries) ---
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw
    glm
    OpenGL::GL
)