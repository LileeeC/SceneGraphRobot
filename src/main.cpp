#define GLFW_INCLUDE_NONE  
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>         
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

// ── 引入我們新蓋好的場景圖類別 ──
#include "SceneNode.h"
#include "Robot.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void checkCompileErrors(unsigned int shader, string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            cerr << "❌ SHADER 錯誤 [" << type << "]:\n" << infoLog << endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            cerr << "❌ SHADER PROGRAM 連結錯誤:\n" << infoLog << endl;
        }
    }
}

// Vertex Shader
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 u_MVP;
    void main() {
        gl_Position = u_MVP * vec4(aPos, 1.0);
    }
)";

// Fragment Shader
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        // 給機器人一個經典的高科技青色線條
        FragColor = vec4(0.0f, 1.0f, 0.7f, 1.0f); 
    }
)";

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "HW3 - Scene Graph Robot Initialized", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST); 

    // 保持線框模式，方便我們看清機器人的關節組合
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 編譯著色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 基礎正立方體頂點資料
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ── [核心步驟 1]：初始化場景圖機器人 ──
    Robot robot;
    robot.initRobot(VAO); // 傳入我們的方塊肉體

    // 設置稍微拉遠的 Camera，讓我們能看清機器人全身
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
    // 攝影機放在 (0, 0, 4) 稍微往上一點看著原點
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 1.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewProjection = projection * view;

    cout << "🌲 場景圖機器人組裝完畢，主循環啟動！" << endl;

    // 主循環
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // ── [核心步驟 2]：套用非線性時間動畫 ──
        float t = (float)glfwGetTime();
        
        // 讓整隻機器人（Root）在世界中緩慢自轉，方便檢查 3D 結構
        robot.root->localTransform = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        // 計算關節擺動角度 (使用正弦波，模擬走路大腿前後擺動 30 度)
        float swingAngle = glm::radians(30.0f * std::sin(t * 3.0f));

        // 驅動左大腿關節：先維持它原本在左Hip的位置 (-0.18, -0.55, 0)，再乘上旋轉
        robot.leftUpperLeg->localTransform = 
            glm::translate(glm::mat4(1.0f), glm::vec3(-0.18f, -0.55f, 0.0f)) * glm::rotate(glm::mat4(1.0f), swingAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        // 驅動右大腿關節：動作與左大腿相反（加個負號），形成前進交叉步
        robot.rightUpperLeg->localTransform = 
            glm::translate(glm::mat4(1.0f), glm::vec3(0.18f, -0.55f, 0.0f)) * glm::rotate(glm::mat4(1.0f), -swingAngle, glm::vec3(1.0f, 0.0f, 0.0f));

        // ── [核心步驟 3]：場景圖走訪與繪製 ──
        robot.update();                         // 遞迴計算整棵樹的世界矩陣
        robot.draw(shaderProgram, viewProjection); // 深度優先遍歷繪製

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}