#pragma once

#include "SceneNode.h"
#include <glm/glm.hpp>

// ============================================================
//  Robot.h  —  機器人階層結構 + 動畫狀態機
// ============================================================

// ------------------------------------------------------------
//  動畫模式列舉
// ------------------------------------------------------------
enum class RobotMode {
    IDLE,   // 原地呼吸：身體浮動 + 雙臂微擺
    WALK,   // 走路：手腳交叉擺動 + 膝蓋連動彎曲
    KICK    // 踢球：右腿蓄力 -> 踢出 -> 收回
};

// ============================================================
//  Robot 類別
// ============================================================
class Robot {
public:
    Robot()  = default;
    ~Robot();

    // --------------------------------------------------------
    //  初始化
    // --------------------------------------------------------
    void initRobot(unsigned int cubeVAO);

    // --------------------------------------------------------
    //  狀態機控制
    // --------------------------------------------------------

    /// 切換動畫模式。
    /// 記錄切換時刻的 time 作為新動畫的時間基點，
    /// 確保 localTime 從 0 重新開始，避免關節跳變。
    void setMode(RobotMode mode, float currentTime);

    RobotMode currentMode { RobotMode::IDLE };

    // --------------------------------------------------------
    //  每幀更新（取代舊版 update()）
    // --------------------------------------------------------

    /// 根據 currentMode 與全域時間計算各關節 localTransform，
    /// 再呼叫 root->update() 刷新整棵樹的 globalTransform。
    void updateAnimation(float time);

    // --------------------------------------------------------
    //  繪製
    // --------------------------------------------------------
    void draw(unsigned int shaderProgram,
            const glm::mat4& viewProjection);

    // --------------------------------------------------------
    //  節點指標（公開，方便 main.cpp 額外控制）
    // --------------------------------------------------------
    SceneNode* root          { nullptr };
    SceneNode* body          { nullptr };
    SceneNode* head          { nullptr };

    SceneNode* leftUpperArm  { nullptr };
    SceneNode* leftLowerArm  { nullptr };
    SceneNode* rightUpperArm { nullptr };
    SceneNode* rightLowerArm { nullptr };

    SceneNode* leftUpperLeg  { nullptr };
    SceneNode* leftLowerLeg  { nullptr };
    SceneNode* leftFoot      { nullptr };

    SceneNode* rightUpperLeg { nullptr };
    SceneNode* rightLowerLeg { nullptr };
    SceneNode* rightFoot     { nullptr };

private:
    // --------------------------------------------------------
    //  各部位「靜止時」相對父節點的平移偏移
    //  updateAnimation 每幀重建 localTransform 時使用
    // --------------------------------------------------------
    struct RestPose {
        glm::vec3 bodyOffset       { 0.00f,  0.00f, 0.00f };
        glm::vec3 headOffset       { 0.00f,  0.60f, 0.00f };

        glm::vec3 lUpperArmOffset  {-0.45f,  0.25f, 0.00f };
        glm::vec3 lLowerArmOffset  { 0.00f, -0.45f, 0.00f };
        glm::vec3 rUpperArmOffset  { 0.45f,  0.25f, 0.00f };
        glm::vec3 rLowerArmOffset  { 0.00f, -0.45f, 0.00f };

        glm::vec3 lUpperLegOffset  {-0.18f, -0.55f, 0.00f };
        glm::vec3 lLowerLegOffset  { 0.00f, -0.50f, 0.00f };
        glm::vec3 lFootOffset      { 0.00f, -0.28f, 0.05f };

        glm::vec3 rUpperLegOffset  { 0.18f, -0.55f, 0.00f };
        glm::vec3 rLowerLegOffset  { 0.00f, -0.50f, 0.00f };
        glm::vec3 rFootOffset      { 0.00f, -0.28f, 0.05f };
    } rest;

    // 記錄切換模式時的時間基點
    float modeStartTime { 0.0f };

    // --------------------------------------------------------
    //  各模式動畫（私有，由 updateAnimation 分派）
    // --------------------------------------------------------
    void animateIdle(float localTime);
    void animateWalk(float localTime);
    void animateKick(float localTime);

    // --------------------------------------------------------
    //  工具：安全重建節點的 localTransform
    //
    //  公式：localTransform = T(offset) x Rx x Ry x Rz
    //
    //  執行順序（GLM Column-Major，由右至左）：
    //    1. Rz  繞 Z 軸旋轉
    //    2. Ry  繞 Y 軸旋轉
    //    3. Rx  繞 X 軸旋轉
    //    4. T   平移到靜止位置
    //  每幀從單位矩陣重新計算，絕不累加，角度不爆炸。
    // --------------------------------------------------------
    static void setLocalTransform(
        SceneNode*       node,
        const glm::vec3& offset,
        float            angleX,
        float            angleY = 0.0f,
        float            angleZ = 0.0f
    );

    // DFS 繪製輔助
    void drawNode(SceneNode* node,
                unsigned int shaderProgram,
                const glm::mat4& viewProjection);
};
