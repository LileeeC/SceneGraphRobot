#pragma once

#include <vector>
#include <glm/glm.hpp>

// ============================================================
//  SceneNode.h  --  Scene Graph Node
//
//  Matrix convention: Column-Major (GLM default)
//  Multiplication order: T * R * S  (right-to-left execution)
//  Child world matrix = parent.globalTransform * self.localTransform
// ============================================================

class SceneNode {
public:
    // --- Transforms -----------------------------------------

    // Relative transform to parent (translation + rotation, NO meshScale)
    glm::mat4 localTransform  { glm::mat4(1.0f) };

    // Final world-space transform, auto-computed by update()
    glm::mat4 globalTransform { glm::mat4(1.0f) };

    // Shape scale for THIS node's mesh only.
    // Kept separate so it does NOT propagate to children.
    glm::mat4 meshScale       { glm::mat4(1.0f) };

    // --- Per-node appearance --------------------------------

    // Diffuse color sent to the fragment shader as u_Color
    glm::vec3 nodeColor { 0.8f, 0.8f, 0.8f };

    // --- Graph structure ------------------------------------

    SceneNode* parent { nullptr };
    std::vector<SceneNode*> children;

    // OpenGL VAO handle (0 = logical anchor, not drawn)
    unsigned int vao { 0 };

    // --- Lifecycle ------------------------------------------

    SceneNode()  = default;
    ~SceneNode() = default;

    // Attach child and set child->parent pointer
    void addChild(SceneNode* child);

    // Recursively recompute globalTransform for this subtree.
    // Call as: root->update(glm::mat4(1.0f))
    //
    // Formula: globalTransform = parentTransform * localTransform
    //   GLM A*B means "apply B first, then A", so:
    //   the local transform is applied in parent space -- correct.
    //
    // meshScale is intentionally excluded here; it is applied
    // only during draw() and must NOT affect children.
    void update(const glm::mat4& parentTransform);
};