#include "SceneNode.h"

// ============================================================
//  SceneNode.cpp  —  場景圖節點實作
// ============================================================

void SceneNode::addChild(SceneNode* child) {
    child->parent = this;          // 建立向上的父指標
    children.push_back(child);     // 建立向下的子列表
}

void SceneNode::update(const glm::mat4& parentTransform) {
    // ----------------------------------------------------------
    //  【核心公式】
    //    globalTransform = parentTransform × localTransform
    //
    //    · parentTransform：父節點的世界矩陣（從外部傳入）
    //    · localTransform ：本節點相對父節點的平移 + 旋轉
    //
    //  注意乘法順序（Column-Major / OpenGL 慣例）：
    //    矩陣 A × 矩陣 B  →  先做 B，再做 A
    //    所以 parentTransform × localTransform
    //    代表「先做 localTransform，再做 parentTransform」，
    //    等同於：在父節點的座標系中做局部變換，正確。
    //
    //  ⚠️  meshScale 刻意「不」納入此計算：
    //    它只在 draw() 時乘在最右側（最先執行），
    //    僅影響當前節點的幾何形狀，不傳遞給子節點。
    // ----------------------------------------------------------
    globalTransform = parentTransform * localTransform;

    // 遞迴更新所有子節點，將自己的世界矩陣向下傳遞
    for (SceneNode* child : children) {
        child->update(globalTransform);
    }
}
