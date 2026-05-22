#pragma once

#include "SceneNode.h"
#include <glm/glm.hpp>

// ============================================================
//  Robot.h  --  Hierarchical robot with animation state machine
//
//  Scene tree depth >= 5 layers:
//    root(1) -> body(2) -> upperLeg(3) -> lowerLeg(4) -> foot(5)
// ============================================================

enum class RobotMode {
    IDLE,   // Breathing idle: body bob + arm sway
    WALK,   // Walk in circle: cross-limb swing + knee coupling
    KICK    // Kick: wind-up -> strike -> follow-through -> recover
};

class Robot {
public:
    Robot()  = default;
    ~Robot();

    void initRobot(unsigned int cubeVAO);

    // Switch mode; pass current glfwGetTime() to reset the
    // local clock and prevent joint angle jump-cuts.
    void setMode(RobotMode mode, float currentTime);

    RobotMode currentMode { RobotMode::IDLE };

    // Compute all localTransforms for this frame, then
    // call root->update() to refresh all globalTransforms.
    void updateAnimation(float time);

    // DFS draw traversal
    void draw(unsigned int shaderProgram,
            const glm::mat4& viewProjection);

    // --- Public node pointers (for external animation control) ---
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
    // Rest-pose offsets (translation only, relative to parent).
    // Stored here so updateAnimation() can rebuild localTransform
    // from scratch every frame without accumulating angles.
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

    float modeStartTime { 0.0f };

    void animateIdle(float localTime);
    void animateWalk(float globalTime, float localTime);
    void animateKick(float localTime);

    // Safely rebuild localTransform = T(offset) * Rx * Ry * Rz.
    // Always starts from identity -- angles never accumulate.
    static void setLocalTransform(
        SceneNode*       node,
        const glm::vec3& offset,
        float            angleX,
        float            angleY = 0.0f,
        float            angleZ = 0.0f
    );

    void drawNode(SceneNode* node,
                unsigned int shaderProgram,
                const glm::mat4& viewProjection);
};