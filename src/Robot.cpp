#include "Robot.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
//  Robot.cpp  —  機器人階層結構實作
// ============================================================

// ------------------------------------------------------------
//  工具：建立一個新節點，設定 VAO、localTransform、meshScale
// ------------------------------------------------------------
static SceneNode* makeNode(
    unsigned int  cubeVAO,
    const glm::vec3& localTranslation,   // 相對父節點的平移
    const glm::vec3& scale               // 方塊的顯示大小（僅影響自身）
) {
    SceneNode* node = new SceneNode();
    node->vao = cubeVAO;

    // localTransform = 平移矩陣（旋轉動畫請在 main 迴圈中疊加）
    node->localTransform = glm::translate(glm::mat4(1.0f), localTranslation);

    // meshScale 純縮放，不傳遞給子節點
    node->meshScale      = glm::scale(glm::mat4(1.0f), scale);

    return node;
}

// ============================================================
//  析構函式：釋放所有動態節點
// ============================================================
Robot::~Robot() {
    // 使用簡單的 BFS/DFS 刪除所有節點
    // （因節點由 Robot 全權管理，不怕重複釋放）
    if (!root) return;

    std::vector<SceneNode*> stack { root };
    while (!stack.empty()) {
        SceneNode* cur = stack.back();
        stack.pop_back();
        for (SceneNode* child : cur->children)
            stack.push_back(child);
        delete cur;
    }
    root = nullptr;
}

// ============================================================
//  initRobot：建立階層樹並設定初始變換
//
//  座標系約定：
//    · Y 軸朝上
//    · 每個節點的 localTranslation 是「關節錨點」相對父關節的偏移
//    · 先平移到正確位置，再在 draw() 乘上 meshScale 決定形狀
// ============================================================
void Robot::initRobot(unsigned int cubeVAO) {

    // ── 第 1 層：root（世界錨點，不繪製方塊，僅做整體平移） ──
    root = new SceneNode();
    root->vao = 0;   // 0 表示本節點不繪製
    root->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    root->meshScale      = glm::mat4(1.0f);

    // ── 第 2 層：body（軀幹，Y+0 相對 root） ──
    body = makeNode(cubeVAO,
        glm::vec3(0.0f, 0.0f, 0.0f),   // 與 root 同位置
        glm::vec3(0.6f, 0.8f, 0.3f)    // 寬 × 高 × 深
    );
    root->addChild(body);

    // ── 第 3 層：head（頭，在 body 頂部上方） ──
    //    body 高 0.8 → 頭關節在 body 頂端 y = 0.8/2 = 0.4，
    //    再加上頭半高 0.2，localTranslation.y = 0.6
    head = makeNode(cubeVAO,
        glm::vec3(0.0f, 0.6f, 0.0f),
        glm::vec3(0.35f, 0.35f, 0.35f)
    );
    body->addChild(head);

    // ── 第 3 層：左上臂（body 左肩） ──
    leftUpperArm = makeNode(cubeVAO,
        glm::vec3(-0.45f, 0.25f, 0.0f),
        glm::vec3(0.18f, 0.45f, 0.18f)
    );
    body->addChild(leftUpperArm);

    // ── 第 4 層：左下臂（肘關節，在上臂底端） ──
    leftLowerArm = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.45f, 0.0f),
        glm::vec3(0.15f, 0.40f, 0.15f)
    );
    leftUpperArm->addChild(leftLowerArm);

    // ── 第 3 層：右上臂 ──
    rightUpperArm = makeNode(cubeVAO,
        glm::vec3(0.45f, 0.25f, 0.0f),
        glm::vec3(0.18f, 0.45f, 0.18f)
    );
    body->addChild(rightUpperArm);

    // ── 第 4 層：右下臂 ──
    rightLowerArm = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.45f, 0.0f),
        glm::vec3(0.15f, 0.40f, 0.15f)
    );
    rightUpperArm->addChild(rightLowerArm);

    // ────────────────────────────────────────────────────────
    //  主線示範（深度 ≥ 3）：root → body → leftUpperLeg → leftLowerLeg → leftFoot
    // ────────────────────────────────────────────────────────

    // ── 第 3 層：左大腿（body 左臀，body 底端 y = -0.4） ──
    leftUpperLeg = makeNode(cubeVAO,
        glm::vec3(-0.18f, -0.55f, 0.0f),
        glm::vec3(0.20f, 0.50f, 0.20f)
    );
    body->addChild(leftUpperLeg);

    // ── 第 4 層：左小腿（膝關節，在大腿底端） ──
    leftLowerLeg = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.50f, 0.0f),
        glm::vec3(0.17f, 0.45f, 0.17f)
    );
    leftUpperLeg->addChild(leftLowerLeg);

    // ── 第 5 層：左腳（踝關節，在小腿底端，往前凸出） ──
    leftFoot = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.28f, 0.05f),
        glm::vec3(0.20f, 0.10f, 0.35f)  // 扁平且較長
    );
    leftLowerLeg->addChild(leftFoot);

    // ── 第 3 層：右大腿 ──
    rightUpperLeg = makeNode(cubeVAO,
        glm::vec3(0.18f, -0.55f, 0.0f),
        glm::vec3(0.20f, 0.50f, 0.20f)
    );
    body->addChild(rightUpperLeg);

    // ── 第 4 層：右小腿 ──
    rightLowerLeg = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.50f, 0.0f),
        glm::vec3(0.17f, 0.45f, 0.17f)
    );
    rightUpperLeg->addChild(rightLowerLeg);

    // ── 第 5 層：右腳 ──
    rightFoot = makeNode(cubeVAO,
        glm::vec3(0.0f, -0.28f, 0.05f),
        glm::vec3(0.20f, 0.10f, 0.35f)
    );
    rightLowerLeg->addChild(rightFoot);
}

// ============================================================
//  update：從根節點遞迴更新整棵樹的 globalTransform
// ============================================================
void Robot::update() {
    // 根節點的父變換為單位矩陣（無父節點）
    root->update(glm::mat4(1.0f));
}

// ============================================================
//  draw：呼叫 DFS 遍歷並繪製
// ============================================================
void Robot::draw(unsigned int shaderProgram, const glm::mat4& viewProjection) {
    drawNode(root, shaderProgram, viewProjection);
}

// ============================================================
//  drawNode：DFS 遞迴繪製單一節點
//
//  MVP 乘法順序（由右至左執行）：
//
//    MVP = viewProjection  × globalTransform × meshScale
//           ↑ 最後執行         ↑ 中間執行       ↑ 最先執行
//    步驟①：meshScale      —— 把單位方塊縮放成各部位的形狀
//    步驟②：globalTransform —— 把縮放後的方塊旋轉並平移到世界空間
//    步驟③：viewProjection —— 套用攝影機視圖與投影，得到裁剪空間座標
// ============================================================
void Robot::drawNode(SceneNode* node,
                     unsigned int shaderProgram,
                     const glm::mat4& viewProjection)
{
    if (!node) return;

    // vao == 0 代表此節點為「邏輯錨點」，只做變換、不繪製
    if (node->vao != 0) {

        // ── 計算最終 MVP 矩陣 ──────────────────────────────
        //
        //  注意：GLM 乘法 A * B 代表「先做 B，再做 A」
        //  因此以下寫法的執行順序為：
        //    1. meshScale       (頂點縮放)
        //    2. globalTransform (世界定位)
        //    3. viewProjection  (投影)
        //
        glm::mat4 MVP = viewProjection          // ③ Projection × View
                      * node->globalTransform   // ② Model（位置 + 旋轉）
                      * node->meshScale;        // ① 形狀縮放（不傳子節點）

        // ── 上傳 MVP 到 Shader ─────────────────────────────
        GLint mvpLoc = glGetUniformLocation(shaderProgram, "u_MVP");
        // GL_FALSE：GLM 已是 Column-Major，不需轉置
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        // ── 繪製方塊 ───────────────────────────────────────
        glBindVertexArray(node->vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);  // 6 面 × 2 三角形 × 3 頂點
        glBindVertexArray(0);
    }

    // ── 遞迴遍歷所有子節點（DFS）──────────────────────────
    for (SceneNode* child : node->children) {
        drawNode(child, shaderProgram, viewProjection);
    }
}
