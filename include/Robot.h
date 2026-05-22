#pragma once

#include "SceneNode.h"
#include <glm/glm.hpp>
#include <vector>

// ============================================================
//  Robot.h  —  機器人階層結構定義
//
//  場景樹深度 ≥ 3 層（從 root 到 foot 共 5 層）：
//
//  root  (世界錨點，僅做整體平移/旋轉)
//   └── body        (軀幹)
//        ├── head        (頭部)
//        ├── leftUpperArm
//        │    └── leftLowerArm
//        ├── rightUpperArm
//        │    └── rightLowerArm
//        ├── leftUpperLeg     ← 深度示範主線
//        │    └── leftLowerLeg
//        │         └── leftFoot   ← 第 5 層（深度 ≥ 3 從 root 算起）
//        └── rightUpperLeg
//             └── rightLowerLeg
//                  └── rightFoot
// ============================================================

class Robot {
public:
    Robot()  = default;
    ~Robot(); // 釋放所有以 new 建立的節點

    // --------------------------------------------------------
    //  初始化：建立階層、設定初始位置與形狀
    // --------------------------------------------------------
    void initRobot(unsigned int cubeVAO);

    // --------------------------------------------------------
    //  每幀呼叫：更新動畫、繪製整棵樹
    // --------------------------------------------------------

    /// 更新整棵場景樹的 globalTransform（動畫時修改各節點
    /// 的 localTransform 後呼叫此函式即可）。
    void update();

    /// DFS 遍歷場景樹並繪製每個節點。
    ///
    /// MVP 計算：
    ///   MVP = viewProjection × globalTransform × meshScale
    ///
    ///   · meshScale      : 最先執行，把單位方塊縮放成各部位形狀
    ///   · globalTransform: 再執行，把方塊放到世界座標中的正確位置
    ///   · viewProjection : 最後執行，套用攝影機投影
    void draw(unsigned int shaderProgram, const glm::mat4& viewProjection);

    // 公開節點指標（方便 main.cpp 做動畫控制）
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
    /// DFS 繪製輔助函式（遞迴）
    void drawNode(SceneNode* node, unsigned int shaderProgram, const glm::mat4& viewProjection);
};