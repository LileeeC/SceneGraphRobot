#include "Robot.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>     // sinf, cosf, fmodf
#include <vector>

// ============================================================
//  Robot.cpp  --  機器人階層結構 + 動畫狀態機實作
// ============================================================

// ============================================================
//  工具函式（模組內部）
// ============================================================

/// 建立節點並設定 VAO / localTransform / meshScale
static SceneNode* makeNode(
    unsigned int     cubeVAO,
    const glm::vec3& localTranslation,
    const glm::vec3& scale)
{
    SceneNode* node      = new SceneNode();
    node->vao            = cubeVAO;
    node->localTransform = glm::translate(glm::mat4(1.0f), localTranslation);
    node->meshScale      = glm::scale   (glm::mat4(1.0f), scale);
    return node;
}

// ============================================================
//  setLocalTransform（靜態工具，每幀安全重建）
//
//  公式：localTransform = T(offset) x Rx(ax) x Ry(ay) x Rz(az)
//
//  GLM 慣例：A * B 代表「先做 B，再做 A」
//  所以展開後執行順序為：
//    step 1  Rz  繞 Z 軸旋轉（側傾）
//    step 2  Ry  繞 Y 軸旋轉（左右轉）
//    step 3  Rx  繞 X 軸旋轉（前後擺）
//    step 4  T   平移到父座標系中的靜止位置
//
//  關鍵：每次都從 glm::mat4(1.0f) 重新計算，
//        角度永遠是「當前瞬間值」，不會無限累加。
// ============================================================
void Robot::setLocalTransform(
    SceneNode*       node,
    const glm::vec3& offset,
    float            angleX,
    float            angleY,
    float            angleZ)
{
    glm::mat4 T  = glm::translate(glm::mat4(1.0f), offset);
    glm::mat4 Rx = glm::rotate   (glm::mat4(1.0f), angleX, glm::vec3(1, 0, 0));
    glm::mat4 Ry = glm::rotate   (glm::mat4(1.0f), angleY, glm::vec3(0, 1, 0));
    glm::mat4 Rz = glm::rotate   (glm::mat4(1.0f), angleZ, glm::vec3(0, 0, 1));

    // T x Rx x Ry x Rz：先旋轉、再平移，符合場景圖慣例
    node->localTransform = T * Rx * Ry * Rz;
}

// ============================================================
//  析構函式
// ============================================================
Robot::~Robot() {
    if (!root) return;
    std::vector<SceneNode*> stack { root };
    while (!stack.empty()) {
        SceneNode* cur = stack.back(); stack.pop_back();
        for (SceneNode* child : cur->children)
            stack.push_back(child);
        delete cur;
    }
    root = nullptr;
}

// ============================================================
//  initRobot
// ============================================================
void Robot::initRobot(unsigned int cubeVAO) {

    // ── 第 1 層：root（邏輯錨點，不繪製） ──
    root = new SceneNode();
    root->vao            = 0;
    root->localTransform = glm::mat4(1.0f);
    root->meshScale      = glm::mat4(1.0f);

    // ── 第 2 層：body ──
    body = makeNode(cubeVAO, rest.bodyOffset,      glm::vec3(0.60f, 0.80f, 0.30f));
    root->addChild(body);

    // ── 第 3 層：head ──
    head = makeNode(cubeVAO, rest.headOffset,      glm::vec3(0.35f, 0.35f, 0.35f));
    body->addChild(head);

    // ── 第 3 層：左上臂 ──
    leftUpperArm  = makeNode(cubeVAO, rest.lUpperArmOffset, glm::vec3(0.18f, 0.45f, 0.18f));
    body->addChild(leftUpperArm);

    // ── 第 4 層：左下臂 ──
    leftLowerArm  = makeNode(cubeVAO, rest.lLowerArmOffset, glm::vec3(0.15f, 0.40f, 0.15f));
    leftUpperArm->addChild(leftLowerArm);

    // ── 第 3 層：右上臂 ──
    rightUpperArm = makeNode(cubeVAO, rest.rUpperArmOffset, glm::vec3(0.18f, 0.45f, 0.18f));
    body->addChild(rightUpperArm);

    // ── 第 4 層：右下臂 ──
    rightLowerArm = makeNode(cubeVAO, rest.rLowerArmOffset, glm::vec3(0.15f, 0.40f, 0.15f));
    rightUpperArm->addChild(rightLowerArm);

    // ── 第 3 層：左大腿 ──
    leftUpperLeg  = makeNode(cubeVAO, rest.lUpperLegOffset, glm::vec3(0.20f, 0.50f, 0.20f));
    body->addChild(leftUpperLeg);

    // ── 第 4 層：左小腿 ──
    leftLowerLeg  = makeNode(cubeVAO, rest.lLowerLegOffset, glm::vec3(0.17f, 0.45f, 0.17f));
    leftUpperLeg->addChild(leftLowerLeg);

    // ── 第 5 層：左腳 ──
    leftFoot = makeNode(cubeVAO, rest.lFootOffset, glm::vec3(0.20f, 0.10f, 0.35f));
    leftLowerLeg->addChild(leftFoot);

    // ── 第 3 層：右大腿 ──
    rightUpperLeg = makeNode(cubeVAO, rest.rUpperLegOffset, glm::vec3(0.20f, 0.50f, 0.20f));
    body->addChild(rightUpperLeg);

    // ── 第 4 層：右小腿 ──
    rightLowerLeg = makeNode(cubeVAO, rest.rLowerLegOffset, glm::vec3(0.17f, 0.45f, 0.17f));
    rightUpperLeg->addChild(rightLowerLeg);

    // ── 第 5 層：右腳 ──
    rightFoot = makeNode(cubeVAO, rest.rFootOffset, glm::vec3(0.20f, 0.10f, 0.35f));
    rightLowerLeg->addChild(rightFoot);
}

// ============================================================
//  setMode：切換模式並重置時間基點
// ============================================================
void Robot::setMode(RobotMode mode, float currentTime) {
    if (currentMode == mode) return; // 已在此模式，不重置
    currentMode = mode;
    modeStartTime  = currentTime; // 新動畫從 localTime = 0 開始
}

// ============================================================
//  updateAnimation：分派到各模式，再刷新場景樹
// ============================================================
void Robot::updateAnimation(float time) {
    float localTime = time - modeStartTime;   // 每個模式自己的時鐘

    switch (currentMode) {
        case RobotMode::IDLE: animateIdle(localTime); break;
        case RobotMode::WALK: animateWalk(localTime); break;
        case RobotMode::KICK: animateKick(localTime); break;
    }

    // 刷新整棵場景樹的 globalTransform
    root->update(glm::mat4(1.0f));
}

// ============================================================
//  animateIdle：原地呼吸
//
//  設計思路：
//    · body 做 Y 軸正弦浮動（模擬胸腔起伏）
//    · 雙臂小幅度繞 X 軸擺動，與呼吸同相位
//    · 頭部輕微繞 Z 軸側傾（增加生命感）
//    · 所有振幅刻意保持極小，避免看起來太誇張
// ============================================================
void Robot::animateIdle(float localTime) {

    const float BREATH_FREQ  = 1.2f;   // 每秒 1.2 個呼吸週期（慢速）
    const float BREATH_AMP   = 0.015f; // body Y 軸浮動幅度（單位）
    const float ARM_AMP      = glm::radians(6.0f);   // 手臂擺動幅度
    const float HEAD_AMP     = glm::radians(3.0f);   // 頭部側傾幅度

    float breath = sinf(localTime * BREATH_FREQ * 2.0f * glm::pi<float>());

    // body：Y 軸微浮（平移不旋轉）
    body->localTransform = glm::translate(
        glm::mat4(1.0f),
        rest.bodyOffset + glm::vec3(0.0f, breath * BREATH_AMP, 0.0f)
    );

    // 頭：隨呼吸輕微側傾（Z 軸）
    setLocalTransform(head, rest.headOffset,
        0.0f, 0.0f, breath * HEAD_AMP);

    // 雙臂：與呼吸同頻，繞 X 軸微擺（向前抬）
    setLocalTransform(leftUpperArm,  rest.lUpperArmOffset,  breath * ARM_AMP);
    setLocalTransform(rightUpperArm, rest.rUpperArmOffset,  breath * ARM_AMP);

    // 下臂 / 腿 / 腳：維持靜止（恢復純平移）
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
//  animateWalk：走路
//
//  【關節連動設計】
//
//  大腿角度 θ_upper（繞 X 軸，正值 = 向前邁出）：
//    θ_upper = A_leg * sin(ωt)
//
//  小腿角度 θ_lower（膝蓋彎曲，永遠是非負值）：
//    當大腿向後（θ_upper < 0）時，小腿向後彎曲，
//    仿造真實行走的「擺盪腿膝蓋彎曲」現象。
//    用 clamp(−θ_upper, 0, max) 取大腿後擺部份：
//
//    θ_lower = clamp(-θ_upper, 0, A_knee)   * KNEE_SCALE
//
//    · -θ_upper > 0 表示大腿正在後擺 → 膝蓋彎曲
//    · -θ_upper ≤ 0 表示大腿正在前邁 → 膝蓋伸直（= 0）
//    這樣膝蓋只在後擺阶段彎曲，前邁時自然伸直，非常逼真。
//
//  手臂與腿反相（手腳交叉自然擺動）：
//    左臂 ≈ 右腿相位（同步正弦）
//    右臂 ≈ 左腿相位（反相正弦）
// ============================================================
void Robot::animateWalk(float localTime) {

    const float WALK_FREQ   = 0.9f;                     // 步頻（Hz）
    const float A_LEG       = glm::radians(50.0f);      // 大腿擺動幅度
    const float A_ARM       = glm::radians(35.0f);      // 手臂擺動幅度
    const float KNEE_SCALE  = 0.85f;                    // 膝蓋彎曲跟隨比例
    const float A_KNEE_MAX  = glm::radians(55.0f);      // 膝蓋最大彎曲角

    float phase = localTime * WALK_FREQ * 2.0f * glm::pi<float>();

    // ── 大腿角度（左右反相） ──────────────────────────────
    float lLegUp =  sinf(phase) * A_LEG;   // 左大腿
    float rLegUp = -sinf(phase) * A_LEG;   // 右大腿（反相）

    // ── 小腿角度：僅在後擺時彎曲 ─────────────────────────
    //   當大腿後擺（angle < 0），-angle > 0 → 觸發彎曲
    //   glm::clamp 確保角度不超過最大值且不為負
    float lLegLo = glm::clamp(-lLegUp, 0.0f, A_KNEE_MAX) * KNEE_SCALE;
    float rLegLo = glm::clamp(-rLegUp, 0.0f, A_KNEE_MAX) * KNEE_SCALE;

    // ── 手臂角度（與對側腿同相，交叉擺動） ──────────────
    float lArmAng = -sinf(phase) * A_ARM;  // 左臂與右腿同相
    float rArmAng =  sinf(phase) * A_ARM;  // 右臂與左腿同相

    // ── 軀幹輕微左右搖晃（Y 軸旋轉，增加真實感） ────────
    float bodySwayY = sinf(phase) * glm::radians(3.0f);

    // ── 寫入各節點 localTransform ─────────────────────────
    setLocalTransform(body,         rest.bodyOffset,      0.0f, bodySwayY);
    setLocalTransform(head,         rest.headOffset,      0.0f);
    setLocalTransform(leftUpperArm, rest.lUpperArmOffset, lArmAng);
    setLocalTransform(leftLowerArm, rest.lLowerArmOffset, 0.0f);   // 下臂自然下垂
    setLocalTransform(rightUpperArm,rest.rUpperArmOffset, rArmAng);
    setLocalTransform(rightLowerArm,rest.rLowerArmOffset, 0.0f);

    // 左腿
    setLocalTransform(leftUpperLeg, rest.lUpperLegOffset, lLegUp);
    setLocalTransform(leftLowerLeg, rest.lLowerLegOffset, lLegLo);  // 膝蓋連動
    setLocalTransform(leftFoot,     rest.lFootOffset,     0.0f);

    // 右腿
    setLocalTransform(rightUpperLeg,rest.rUpperLegOffset, rLegUp);
    setLocalTransform(rightLowerLeg,rest.rLowerLegOffset, rLegLo);  // 膝蓋連動
    setLocalTransform(rightFoot,    rest.rFootOffset,     0.0f);
}

// ============================================================
//  animateKick：踢球（週期性右腿踢擊）
//
//  動作拆解（一個週期 T = 2.0 秒）：
//
//   Phase 0.00 ~ 0.25T  (0.0 ~ 0.5s)  蓄力：右腿向後抬，膝蓋大幅彎曲
//   Phase 0.25 ~ 0.45T  (0.5 ~ 0.9s)  踢出：大腿快速前甩
//   Phase 0.45 ~ 0.55T  (0.9 ~ 1.1s)  踢直：小腿在最高點迅速伸直
//   Phase 0.55 ~ 1.00T  (1.1 ~ 2.0s)  收回：整腿緩緩回到靜止
//
//  使用「分段線性插值 + 平滑步進」實現非線性時間曲線，
//  完全不依賴外部動畫庫。
//
//  smoothstep(a, b, x)：在 [a,b] 範圍內的三次方平滑插值，
//  產生「慢進慢出」效果，比線性插值逼真許多。
// ============================================================

// 輔助：把 x 限制在 [0,1] 並套用 smoothstep 曲線
static float smoothstep01(float x) {
    x = glm::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);   // 3t^2 - 2t^3
}

// 在 [lo, hi] 範圍內，將 x 映射到 [0,1] 並平滑
static float remap(float x, float lo, float hi) {
    return smoothstep01((x - lo) / (hi - lo));
}

void Robot::animateKick(float localTime) {

    const float KICK_PERIOD = 2.0f;   // 整個踢球週期（秒）

    // 週期化 localTime（讓動作循環播放）
    float t = fmodf(localTime, KICK_PERIOD) / KICK_PERIOD;  // t in [0, 1)

    // ── 關鍵影格角度定義（單位：弧度） ──────────────────

    // 右大腿：靜止(0) → 後擺蓄力(-45°) → 前踢(+70°) → 靜止(0)
    const float WINDUP_UPPER = glm::radians(45.0f);  // 蓄力時大腿後擺
    const float KICK_UPPER   = glm::radians(-70.0f);  // 踢出時大腿前甩

    // 右小腿：靜止(0) → 蓄力大彎(-90°) → 踢直(+5°) → 靜止(0)
    const float WINDUP_LOWER = glm::radians(85.0f);  // 蓄力時膝蓋深度彎曲
    const float KICK_LOWER   = glm::radians(-5.0f);  // 踢直時小腿略微過直

    float rUpperAngle = 0.0f;
    float rLowerAngle = 0.0f;

    if (t < 0.25f) {
        // ── 蓄力階段（0 → 0.25）慢慢向後蓄力 ──────────
        float p = remap(t, 0.0f, 0.25f);
        rUpperAngle = glm::mix(0.0f, WINDUP_UPPER, p);
        rLowerAngle = glm::mix(0.0f, WINDUP_LOWER, p);
    }
    else if (t < 0.45f) {
        // ── 踢出階段（0.25 → 0.45）大腿快速前甩 ────────
        // 使用較陡的 smoothstep 讓速度感更強
        float p = remap(t, 0.25f, 0.45f);
        rUpperAngle = glm::mix(WINDUP_UPPER, KICK_UPPER, p);
        // 小腿暫時仍彎曲（蓄力殘留），之後踢直
        rLowerAngle = glm::mix(WINDUP_LOWER, 0.0f, p * 0.6f);
    }
    else if (t < 0.55f) {
        // ── 踢直階段（0.45 → 0.55）小腿迅速伸直 ────────
        float p = remap(t, 0.45f, 0.55f);
        rUpperAngle = KICK_UPPER;   // 大腿維持最高點
        rLowerAngle = glm::mix(0.0f, KICK_LOWER, p);  // 小腿踢直（略過直）
    }
    else {
        // ── 收回階段（0.55 → 1.0）緩緩回到靜止 ─────────
        float p = remap(t, 0.55f, 1.0f);
        rUpperAngle = glm::mix(KICK_UPPER, 0.0f, p);
        rLowerAngle = glm::mix(KICK_LOWER, 0.0f, p);
    }

    // ── 左腿相位差（踢球時站立腿輕微向後撐） ────────────
    float lUpperAngle = sinf(t * glm::pi<float>()) * glm::radians(-8.0f);

    // ── 軀幹向前微傾（踢球蓄力感） ──────────────────────
    float bodyTilt = 0.0f;
    if (t < 0.45f)
        bodyTilt = remap(t, 0.0f, 0.45f) * glm::radians(-8.0f); // 向前傾
    else
        bodyTilt = (1.0f - remap(t, 0.55f, 1.0f)) * glm::radians(-8.0f);

    // ── 寫入各節點 ───────────────────────────────────────
    setLocalTransform(body,          rest.bodyOffset,      bodyTilt);
    setLocalTransform(head,          rest.headOffset,      0.0f);

    // 手臂：右臂隨右腿反向甩（踢球自然動作）
    setLocalTransform(leftUpperArm,  rest.lUpperArmOffset,  rUpperAngle * 0.4f);
    setLocalTransform(leftLowerArm,  rest.lLowerArmOffset,  0.0f);
    setLocalTransform(rightUpperArm, rest.rUpperArmOffset, -rUpperAngle * 0.4f);
    setLocalTransform(rightLowerArm, rest.rLowerArmOffset,  0.0f);

    // 左腿（支撐腿）
    setLocalTransform(leftUpperLeg,  rest.lUpperLegOffset,  lUpperAngle);
    setLocalTransform(leftLowerLeg,  rest.lLowerLegOffset,  0.0f);
    setLocalTransform(leftFoot,      rest.lFootOffset,      0.0f);

    // 右腿（踢擊腿）
    setLocalTransform(rightUpperLeg, rest.rUpperLegOffset,  rUpperAngle);
    setLocalTransform(rightLowerLeg, rest.rLowerLegOffset,  rLowerAngle);
    setLocalTransform(rightFoot,     rest.rFootOffset,      0.0f);
}

// ============================================================
//  draw / drawNode
// ============================================================
void Robot::draw(unsigned int shaderProgram, const glm::mat4& viewProjection) {
    drawNode(root, shaderProgram, viewProjection);
}

void Robot::drawNode(SceneNode* node,
                    unsigned int shaderProgram,
                    const glm::mat4& viewProjection)
{
    if (!node) return;

    if (node->vao != 0) {
        // MVP = viewProjection x globalTransform x meshScale
        //
        // 執行順序（由右至左）：
        //   step 1  meshScale       把單位方塊縮放成部位形狀
        //   step 2  globalTransform 把方塊旋轉並平移到世界座標
        //   step 3  viewProjection  套用攝影機投影
        glm::mat4 MVP = viewProjection
                      * node->globalTransform
                      * node->meshScale;

        GLint mvpLoc = glGetUniformLocation(shaderProgram, "u_MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        glBindVertexArray(node->vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    for (SceneNode* child : node->children)
        drawNode(child, shaderProgram, viewProjection);
}
