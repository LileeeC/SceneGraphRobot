#include "Robot.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

// ============================================================
//  Robot.cpp  --  Animation state machine implementation
// ============================================================

// ------------------------------------------------------------
//  Internal helpers
// ------------------------------------------------------------

static SceneNode* makeNode(
    unsigned int     cubeVAO,
    const glm::vec3& translation,
    const glm::vec3& scale,
    const glm::vec3& color)
{
    SceneNode* n      = new SceneNode();
    n->vao            = cubeVAO;
    n->localTransform = glm::translate(glm::mat4(1.0f), translation);
    n->meshScale      = glm::scale    (glm::mat4(1.0f), scale);
    n->nodeColor      = color;
    return n;
}

// smoothstep curve: f(x) = 3x^2 - 2x^3, x clamped to [0,1]
// Produces slow-in / slow-out easing without any external library.
static float smoothstep01(float x) {
    x = glm::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

static float remap(float x, float lo, float hi) {
    return smoothstep01((x - lo) / (hi - lo));
}

// ------------------------------------------------------------
//  setLocalTransform  (safe every-frame rebuild)
//
//  Formula: localTransform = T(offset) * Rx(ax) * Ry(ay) * Rz(az)
//
//  GLM column-major execution order (right-to-left):
//    step 1  Rz  roll
//    step 2  Ry  yaw
//    step 3  Rx  pitch (forward/back swing)
//    step 4  T   translate to rest position in parent space
//
//  Starting from glm::mat4(1.0f) every frame guarantees
//  angles are instantaneous values, never cumulative.
// ------------------------------------------------------------
void Robot::setLocalTransform(
    SceneNode*       node,
    const glm::vec3& offset,
    float            ax,
    float            ay,
    float            az)
{
    const glm::vec3 X(1, 0, 0);
    const glm::vec3 Y(0, 1, 0);
    const glm::vec3 Z(0, 0, 1);

    glm::mat4 T  = glm::translate(glm::mat4(1.0f), offset);
    glm::mat4 Rx = glm::rotate   (glm::mat4(1.0f), ax, X);
    glm::mat4 Ry = glm::rotate   (glm::mat4(1.0f), ay, Y);
    glm::mat4 Rz = glm::rotate   (glm::mat4(1.0f), az, Z);

    node->localTransform = T * Rx * Ry * Rz;
}

// ------------------------------------------------------------
//  Destructor
// ------------------------------------------------------------
Robot::~Robot() {
    if (!root) return;
    std::vector<SceneNode*> stack { root };
    while (!stack.empty()) {
        SceneNode* cur = stack.back(); stack.pop_back();
        for (SceneNode* c : cur->children) stack.push_back(c);
        delete cur;
    }
    root = nullptr;
}

// ------------------------------------------------------------
//  initRobot -- build hierarchy and assign colors
//
//  Color palette (sci-fi robot):
//    body/limbs  : dark steel  #3A3F4B  (0.23, 0.25, 0.29)
//    head        : amber       #F5A623  (0.96, 0.65, 0.14)
//    upper arms  : mid steel   #4E5562  (0.31, 0.33, 0.38)
//    lower arms  : light steel #7B8494  (0.48, 0.52, 0.58)
//    upper legs  : dark steel  #3A3F4B
//    lower legs  : mid steel   #4E5562
//    feet        : accent red  #D9534F  (0.85, 0.33, 0.31)
//    joints      : pale silver #C8CDD6  (0.78, 0.80, 0.84)
// ------------------------------------------------------------
void Robot::initRobot(unsigned int cubeVAO) {

    const glm::vec3 COL_DARK  (0.23f, 0.25f, 0.29f);
    const glm::vec3 COL_MID   (0.31f, 0.33f, 0.38f);
    const glm::vec3 COL_LIGHT (0.48f, 0.52f, 0.58f);
    const glm::vec3 COL_SILVER(0.78f, 0.80f, 0.84f);
    const glm::vec3 COL_AMBER (0.96f, 0.65f, 0.14f);
    const glm::vec3 COL_RED   (0.85f, 0.33f, 0.31f);

    // Layer 1: root (logical anchor, never drawn)
    root = new SceneNode();
    root->vao            = 0;
    root->localTransform = glm::mat4(1.0f);
    root->meshScale      = glm::mat4(1.0f);

    // Layer 2: body
    body = makeNode(cubeVAO, rest.bodyOffset, glm::vec3(0.60f, 0.80f, 0.30f), COL_DARK);
    root->addChild(body);

    // Layer 3: head
    head = makeNode(cubeVAO, rest.headOffset, glm::vec3(0.35f, 0.35f, 0.35f), COL_AMBER);
    body->addChild(head);

    // Layer 3/4: arms
    leftUpperArm  = makeNode(cubeVAO, rest.lUpperArmOffset, glm::vec3(0.18f, 0.45f, 0.18f), COL_MID);
    body->addChild(leftUpperArm);
    leftLowerArm  = makeNode(cubeVAO, rest.lLowerArmOffset, glm::vec3(0.15f, 0.40f, 0.15f), COL_LIGHT);
    leftUpperArm->addChild(leftLowerArm);

    rightUpperArm = makeNode(cubeVAO, rest.rUpperArmOffset, glm::vec3(0.18f, 0.45f, 0.18f), COL_MID);
    body->addChild(rightUpperArm);
    rightLowerArm = makeNode(cubeVAO, rest.rLowerArmOffset, glm::vec3(0.15f, 0.40f, 0.15f), COL_LIGHT);
    rightUpperArm->addChild(rightLowerArm);

    // Layer 3/4/5: left leg
    leftUpperLeg  = makeNode(cubeVAO, rest.lUpperLegOffset, glm::vec3(0.20f, 0.50f, 0.20f), COL_DARK);
    body->addChild(leftUpperLeg);
    leftLowerLeg  = makeNode(cubeVAO, rest.lLowerLegOffset, glm::vec3(0.17f, 0.45f, 0.17f), COL_MID);
    leftUpperLeg->addChild(leftLowerLeg);
    leftFoot      = makeNode(cubeVAO, rest.lFootOffset,     glm::vec3(0.20f, 0.10f, 0.35f), COL_RED);
    leftLowerLeg->addChild(leftFoot);

    // Layer 3/4/5: right leg
    rightUpperLeg = makeNode(cubeVAO, rest.rUpperLegOffset, glm::vec3(0.20f, 0.50f, 0.20f), COL_DARK);
    body->addChild(rightUpperLeg);
    rightLowerLeg = makeNode(cubeVAO, rest.rLowerLegOffset, glm::vec3(0.17f, 0.45f, 0.17f), COL_MID);
    rightUpperLeg->addChild(rightLowerLeg);
    rightFoot     = makeNode(cubeVAO, rest.rFootOffset,     glm::vec3(0.20f, 0.10f, 0.35f), COL_RED);
    rightLowerLeg->addChild(rightFoot);
}

// ------------------------------------------------------------
//  setMode
// ------------------------------------------------------------
void Robot::setMode(RobotMode mode, float currentTime) {
    if (currentMode == mode) return;
    currentMode   = mode;
    modeStartTime = currentTime;
}

// ------------------------------------------------------------
//  updateAnimation
// ------------------------------------------------------------
void Robot::updateAnimation(float time) {
    float localTime = time - modeStartTime;

    switch (currentMode) {
        case RobotMode::IDLE: animateIdle(localTime);            break;
        case RobotMode::WALK: animateWalk(time, localTime);      break;
        case RobotMode::KICK: animateKick(localTime);            break;
    }

    root->update(glm::mat4(1.0f));
}

// ============================================================
//  IDLE -- breathing
//
//  body: gentle Y-axis bob  amplitude=0.015  freq=1.2 Hz
//  arms: small X-axis sway  in phase with breath
//  head: slight Z-axis tilt for a "alive" feeling
//  Everything else: rest pose (zero rotation)
// ============================================================
void Robot::animateIdle(float t) {

    const float TWO_PI     = 2.0f * glm::pi<float>();
    const float FREQ       = 1.2f;
    const float BOB_AMP    = 0.015f;
    const float ARM_AMP    = glm::radians(6.0f);
    const float HEAD_AMP   = glm::radians(3.0f);

    float breath = sinf(t * FREQ * TWO_PI);

    // body: translate only (Y bob)
    body->localTransform = glm::translate(
        glm::mat4(1.0f),
        rest.bodyOffset + glm::vec3(0.0f, breath * BOB_AMP, 0.0f)
    );

    setLocalTransform(head,          rest.headOffset,       0.0f, 0.0f, breath * HEAD_AMP);
    setLocalTransform(leftUpperArm,  rest.lUpperArmOffset,  breath * ARM_AMP);
    setLocalTransform(rightUpperArm, rest.rUpperArmOffset,  breath * ARM_AMP);
    setLocalTransform(leftLowerArm,  rest.lLowerArmOffset,  0.0f);
    setLocalTransform(rightLowerArm, rest.rLowerArmOffset,  0.0f);
    setLocalTransform(leftUpperLeg,  rest.lUpperLegOffset,  0.0f);
    setLocalTransform(leftLowerLeg,  rest.lLowerLegOffset,  0.0f);
    setLocalTransform(leftFoot,      rest.lFootOffset,      0.0f);
    setLocalTransform(rightUpperLeg, rest.rUpperLegOffset,  0.0f);
    setLocalTransform(rightLowerLeg, rest.rLowerLegOffset,  0.0f);
    setLocalTransform(rightFoot,     rest.rFootOffset,      0.0f);
}

// ============================================================
//  WALK -- circle orbit + knee coupling + centripetal lean
//
//  Orbit:
//    The robot travels on a circle of radius R = 1.8 around
//    the world origin.  At time t, the position angle is:
//      orbitAngle = ORBIT_SPEED * globalTime
//    Position:  (R*cos(a), 0, R*sin(a))
//    Facing direction tangent to the circle = orbitAngle + 90 deg
//    So the root's localTransform = T(pos) * Ry(facing)
//
//  Centripetal lean:
//    A runner leaning into a curve tilts their body toward the
//    center of the circle (inward).  In the robot's local frame,
//    the inward direction is its local +X axis (to the left when
//    facing forward).
//    We apply a small Rz rotation on the body node (after the
//    root has already established the facing direction):
//      leanAngle = LEAN_DEG  (positive = tilt toward center)
//    This is a constant offset while walking, not animated.
//
//  Knee coupling (physics-based):
//    Lower leg angle = clamp(-upperLegAngle, 0, MAX) * SCALE
//    Only the swing leg (moving backward) bends the knee.
//    The stance leg stays straight -- matches real biomechanics.
// ============================================================
void Robot::animateWalk(float globalTime, float localTime) {

    const float TWO_PI      = 2.0f * glm::pi<float>();

    // -- Orbit parameters ------------------------------------
    const float RADIUS      = 1.8f;
    const float ORBIT_SPEED = 0.55f;   // rad/s, full lap ~11.4 s

    // -- Step parameters -------------------------------------
    const float WALK_FREQ   = 1.6f;    // steps per second
    const float A_LEG       = glm::radians(32.0f);
    const float A_ARM       = glm::radians(22.0f);
    const float KNEE_SCALE  = 0.75f;
    const float KNEE_MAX    = glm::radians(40.0f);

    // -- Centripetal lean ------------------------------------
    const float LEAN_ANGLE  = glm::radians(4.0f);  // ~4 deg inward

    // ---- 1. Orbit: compute root localTransform -------------
    float orbitAngle   = ORBIT_SPEED * globalTime;
    float posX         =  RADIUS * cosf(orbitAngle);
    float posZ         =  RADIUS * sinf(orbitAngle);

    // Facing angle: tangent direction = orbitAngle + PI/2
    // Ry rotation in GLM rotates CCW around Y, so a positive
    // facing angle turns the robot to face the direction of travel.
    float facingAngle  = -orbitAngle;

    glm::mat4 T_orbit  = glm::translate(glm::mat4(1.0f), glm::vec3(posX, 0.0f, posZ));
    glm::mat4 R_facing = glm::rotate   (glm::mat4(1.0f), facingAngle, glm::vec3(0, 1, 0));
    root->localTransform = T_orbit * R_facing;

    // ---- 2. Centripetal lean on body (local Z-axis tilt) ---
    // In the robot's local frame (after facing rotation), the
    // inward-to-center direction maps to its local +X axis.
    // A positive Rz rotation tilts the top of the body toward +X,
    // which is inward -- exactly the centripetal lean we want.
    setLocalTransform(body, rest.bodyOffset, 0.0f, 0.0f, LEAN_ANGLE);

    // ---- 3. Limb swing animation ---------------------------
    float phase    = localTime * WALK_FREQ * TWO_PI;
    float lLegUp   =  sinf(phase) * A_LEG;
    float rLegUp   = -sinf(phase) * A_LEG;
    float lLegLo   =  glm::clamp(-lLegUp, 0.0f, KNEE_MAX) * KNEE_SCALE;
    float rLegLo   =  glm::clamp(-rLegUp, 0.0f, KNEE_MAX) * KNEE_SCALE;
    float lArmAng  = -sinf(phase) * A_ARM;
    float rArmAng  =  sinf(phase) * A_ARM;

    setLocalTransform(head,          rest.headOffset,       0.0f);
    setLocalTransform(leftUpperArm,  rest.lUpperArmOffset,  lArmAng);
    setLocalTransform(leftLowerArm,  rest.lLowerArmOffset,  0.0f);
    setLocalTransform(rightUpperArm, rest.rUpperArmOffset,  rArmAng);
    setLocalTransform(rightLowerArm, rest.rLowerArmOffset,  0.0f);

    setLocalTransform(leftUpperLeg,  rest.lUpperLegOffset,  lLegUp);
    setLocalTransform(leftLowerLeg,  rest.lLowerLegOffset,  lLegLo);
    setLocalTransform(leftFoot,      rest.lFootOffset,      0.0f);
    setLocalTransform(rightUpperLeg, rest.rUpperLegOffset,  rLegUp);
    setLocalTransform(rightLowerLeg, rest.rLowerLegOffset,  rLegLo);
    setLocalTransform(rightFoot,     rest.rFootOffset,      0.0f);
}

// ============================================================
//  KICK -- nonlinear segmented timeline (right leg)
//
//  One period = 2.0 s, looping via fmod.
//
//  Segment    t-range    Action
//  --------   --------   ------------------------------------------
//  Wind-up    0 - 0.25   Right thigh swings back; knee deeply bent
//  Strike     0.25-0.45  Thigh whips forward (fast smoothstep)
//  Snap       0.45-0.55  Lower leg snaps straight at apex
//  Recover    0.55-1.00  Whole leg returns to rest (slow ease-out)
//
//  smoothstep gives each segment a slow-in/slow-out profile,
//  creating realistic acceleration without any animation library.
// ============================================================
void Robot::animateKick(float localTime) {

    const float PERIOD       = 2.0f;
    const float UPPER_WINDUP = glm::radians(45.0f);
    const float UPPER_KICK   = glm::radians(-70.0f);
    const float LOWER_WINDUP = glm::radians(85.0f);
    const float LOWER_SNAP   = glm::radians(-5.0f);

    float t = fmodf(localTime, PERIOD) / PERIOD;  // normalized [0, 1)

    float rUp = 0.0f, rLo = 0.0f;

    if (t < 0.25f) {
        float p = remap(t, 0.0f,  0.25f);
        rUp = glm::mix(0.0f,        UPPER_WINDUP, p);
        rLo = glm::mix(0.0f,        LOWER_WINDUP, p);
    } else if (t < 0.45f) {
        float p = remap(t, 0.25f, 0.45f);
        rUp = glm::mix(UPPER_WINDUP, UPPER_KICK,   p);
        rLo = glm::mix(LOWER_WINDUP, 0.0f,         p * 0.6f);
    } else if (t < 0.55f) {
        float p = remap(t, 0.45f, 0.55f);
        rUp = UPPER_KICK;
        rLo = glm::mix(0.0f,        LOWER_SNAP,   p);
    } else {
        float p = remap(t, 0.55f, 1.0f);
        rUp = glm::mix(UPPER_KICK,  0.0f,          p);
        rLo = glm::mix(LOWER_SNAP,  0.0f,          p);
    }

    // Support leg: slight backward lean for balance
    float lUp    = sinf(t * glm::pi<float>()) * glm::radians(-8.0f);

    // Torso: leans forward during wind-up and strike
    float tilt   = 0.0f;
    if      (t < 0.45f) tilt = remap(t, 0.0f,  0.45f) * glm::radians(-8.0f);
    else if (t < 1.0f)  tilt = (1.0f - remap(t, 0.55f, 1.0f)) * glm::radians(-8.0f);

    setLocalTransform(body,          rest.bodyOffset,       tilt);
    setLocalTransform(head,          rest.headOffset,       0.0f);
    setLocalTransform(leftUpperArm,  rest.lUpperArmOffset,  rUp * 0.4f);
    setLocalTransform(leftLowerArm,  rest.lLowerArmOffset,  0.0f);
    setLocalTransform(rightUpperArm, rest.rUpperArmOffset, -rUp * 0.4f);
    setLocalTransform(rightLowerArm, rest.rLowerArmOffset,  0.0f);
    setLocalTransform(leftUpperLeg,  rest.lUpperLegOffset,  lUp);
    setLocalTransform(leftLowerLeg,  rest.lLowerLegOffset,  0.0f);
    setLocalTransform(leftFoot,      rest.lFootOffset,      0.0f);
    setLocalTransform(rightUpperLeg, rest.rUpperLegOffset,  rUp);
    setLocalTransform(rightLowerLeg, rest.rLowerLegOffset,  rLo);
    setLocalTransform(rightFoot,     rest.rFootOffset,      0.0f);
}

// ============================================================
//  draw / drawNode
// ============================================================
void Robot::draw(unsigned int shaderProgram, const glm::mat4& viewProjection) {
    drawNode(root, shaderProgram, viewProjection);
}

// ------------------------------------------------------------
//  drawNode  (DFS)
//
//  MVP formula:
//    MVP = viewProjection * globalTransform * meshScale
//
//  Execution order (right-to-left):
//    step 1  meshScale       scale unit cube to limb shape
//    step 2  globalTransform rotate + translate into world space
//    step 3  viewProjection  project to clip space
//
//  Normal matrix (for lighting correctness):
//    normalMatrix = transpose(inverse(globalTransform))
//    Needed because non-uniform scale distorts normals.
//    meshScale is NOT included -- it is only a shape helper
//    and is excluded from the normal transform.
// ------------------------------------------------------------
void Robot::drawNode(SceneNode* node,
                    unsigned int shaderProgram,
                    const glm::mat4& viewProjection)
{
    if (!node) return;

    if (node->vao != 0) {
        glm::mat4 MVP = viewProjection * node->globalTransform * node->meshScale;
        glm::mat3 NM  = glm::mat3(glm::transpose(glm::inverse(node->globalTransform * node->meshScale)));

        GLint mvpLoc    = glGetUniformLocation(shaderProgram, "u_MVP");
        GLint nmLoc     = glGetUniformLocation(shaderProgram, "u_NormalMatrix");
        GLint colorLoc  = glGetUniformLocation(shaderProgram, "u_Color");
        GLint modelLoc  = glGetUniformLocation(shaderProgram, "u_Model");

        glUniformMatrix4fv(mvpLoc,   1, GL_FALSE, glm::value_ptr(MVP));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(node->globalTransform * node->meshScale));
        glUniformMatrix3fv(nmLoc,    1, GL_FALSE, glm::value_ptr(NM));
        glUniform3fv      (colorLoc, 1,            glm::value_ptr(node->nodeColor));

        glBindVertexArray(node->vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    for (SceneNode* child : node->children)
        drawNode(child, shaderProgram, viewProjection);
}