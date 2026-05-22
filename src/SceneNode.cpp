#include "SceneNode.h"

void SceneNode::addChild(SceneNode* child) {
    child->parent = this;
    children.push_back(child);
}

void SceneNode::update(const glm::mat4& parentTransform) {
    // globalTransform = parentTransform * localTransform
    //
    // Execution order (right-to-left in GLM column-major):
    //   1. localTransform  -- position/rotate in parent space
    //   2. parentTransform -- carry into world space
    //
    // meshScale is NOT multiplied here; it only affects the
    // draw call for this node and must not cascade to children.
    globalTransform = parentTransform * localTransform;

    for (SceneNode* child : children)
        child->update(globalTransform);
}