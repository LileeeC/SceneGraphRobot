#define GLFW_INCLUDE_NONE  
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>         
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#include "ShaderSources.h"
#include "SceneNode.h"
#include "Robot.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 檢查 Shader 編譯狀態
void checkCompileErrors(unsigned int shader, string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            cerr << "SHADER COMPILATION ERROR [" << type << "]:\n" << infoLog << endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            cerr << "SHADER PROGRAM LINKING ERROR:\n" << infoLog << endl;
        }
    }
}

// camera 參數
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.5f, 5.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.5f, 0.0f);
glm::vec3 cameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);

const float CAM_MOVE_SPEED   = 2.5f;
const float CAM_STRAFE_SPEED = 2.0f;

// 鍵盤輸入
void processInput(GLFWwindow* window, float deltaTime, Robot& robot) {
    // 靜態變數記錄上一幀的按鍵狀態，防止重複觸發
    static int prevKey1 = GLFW_RELEASE;
    static int prevKey2 = GLFW_RELEASE;
    static int prevKey3 = GLFW_RELEASE;

    int k1 = glfwGetKey(window, GLFW_KEY_1);
    int k2 = glfwGetKey(window, GLFW_KEY_2);
    int k3 = glfwGetKey(window, GLFW_KEY_3);

    float now = (float)glfwGetTime();
    if (k1 == GLFW_PRESS && prevKey1 == GLFW_RELEASE) robot.setMode(RobotMode::IDLE, now);
    if (k2 == GLFW_PRESS && prevKey2 == GLFW_RELEASE) robot.setMode(RobotMode::WALK, now);
    if (k3 == GLFW_PRESS && prevKey3 == GLFW_RELEASE) robot.setMode(RobotMode::KICK, now);

    prevKey1 = k1; prevKey2 = k2; prevKey3 = k3;

    // 方向感知 Camera 控制
    glm::vec3 forward = glm::normalize(cameraTarget - cameraPos);
    glm::vec3 right   = glm::normalize(glm::cross(forward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) cameraPos += forward * CAM_MOVE_SPEED * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) cameraPos -= forward * CAM_MOVE_SPEED * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) cameraPos -= right   * CAM_STRAFE_SPEED * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraPos += right   * CAM_STRAFE_SPEED * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "HW3 - Blinn-Phong Scene Graph Robot", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 

    // compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SRC, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    // compile Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_SRC, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    // connect Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 6 floats/vertex VAO/VBO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    // a_Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE, (void*)0);
    glEnableVertexAttribArray(0);

    // a_Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CUBE_STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 世界靜態平行光源方向
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.2f, 2.0f, 1.5f));
    glUseProgram(shaderProgram);
    glUniform3fv(glGetUniformLocation(shaderProgram, "u_LightDir"), 1, glm::value_ptr(lightDir));

    // init
    Robot robot;
    robot.initRobot(cubeVAO);

    float lastFrameTime = 0.0f;
    cout << "系統啟動" << endl;

    // main loop
    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastFrameTime;
        lastFrameTime     = currentTime;

        // 輸入監聽
        processInput(window, deltaTime, robot);

        // 清除畫布
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // 動態將攝影機世界座標送入 Shader，用於計算 Blinn-Phong 高光
        glUniform3fv(glGetUniformLocation(shaderProgram, "u_ViewPos"), 1, glm::value_ptr(cameraPos));

        // compute View Projection matrix
        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        float aspect = (winH > 0) ? (float)winW / (float)winH : 1.0f;

        glm::mat4 view       = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 VP         = projection * view;

        robot.updateAnimation(currentTime);
        robot.draw(shaderProgram, VP);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}