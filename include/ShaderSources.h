#pragma once

// ============================================================
//  ShaderSources.h
//
//  Inline GLSL source strings for OpenGL 3.3 Core Profile.
//  Include this header in main.cpp and pass these C-strings
//  to your existing shader compilation utility.
//
//  Lighting model: Blinn-Phong directional light
//    - Diffuse  : saturate(dot(N, L))
//    - Ambient  : small constant fill light
//    - Specular : Blinn halfway-vector highlight
// ============================================================

// ------------------------------------------------------------
//  Vertex Shader
//
//  Inputs (location-bound):
//    location 0  vec3  a_Position   object-space vertex position
//    location 1  vec3  a_Normal     object-space vertex normal
//
//  Uniforms:
//    u_MVP          mat4  model-view-projection matrix
//    u_Model        mat4  model matrix (globalTransform * meshScale)
//    u_NormalMatrix mat3  transpose(inverse(u_Model)) -- corrects normals
//                         when non-uniform scale is present
//
//  Outputs to fragment shader:
//    v_Normal       vec3  world-space normal (normalized)
//    v_FragPos      vec3  world-space fragment position
// ------------------------------------------------------------
static const char* VERTEX_SHADER_SRC = R"GLSL(
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;

out vec3 v_Normal;
out vec3 v_FragPos;

void main() {
    gl_Position = u_MVP * vec4(a_Position, 1.0);

    // Transform position into world space for specular calc
    v_FragPos = vec3(u_Model * vec4(a_Position, 1.0));

    // NormalMatrix corrects normal direction under non-uniform scale.
    // Without it, normals would be stretched/skewed with the mesh.
    v_Normal  = normalize(u_NormalMatrix * a_Normal);
}
)GLSL";

// ------------------------------------------------------------
//  Fragment Shader
//
//  Uniforms:
//    u_Color       vec3  per-node base diffuse color (set in drawNode)
//    u_LightDir    vec3  world-space light direction (normalized, toward light)
//    u_ViewPos     vec3  camera position in world space (for specular)
//
//  Lighting equation:
//    ambient  = 0.18 * color
//    diffuse  = max(dot(N, L), 0.0) * color
//    specular = pow(max(dot(N, H), 0.0), 32) * 0.35 * vec3(1.0)
//               where H = normalize(L + V)  -- Blinn halfway vector
//    finalColor = ambient + diffuse + specular
//
//  The ambient term ensures the dark side of each cube is still
//  visible and not completely black.
// ------------------------------------------------------------
static const char* FRAGMENT_SHADER_SRC = R"GLSL(
#version 330 core

in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_Color;
uniform vec3 u_LightDir;   // direction FROM fragment TOWARD light (normalized)
uniform vec3 u_ViewPos;    // camera world-space position

out vec4 FragColor;

void main() {
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(u_LightDir);
    vec3 V = normalize(u_ViewPos - v_FragPos);

    // Blinn-Phong halfway vector
    vec3 H = normalize(L + V);

    // Lighting components
    float ambientStrength  = 0.18;
    float diffuseStrength  = max(dot(N, L), 0.0);
    float specularStrength = pow(max(dot(N, H), 0.0), 32.0) * 0.35;

    vec3 ambient  = ambientStrength  * u_Color;
    vec3 diffuse  = diffuseStrength  * u_Color;
    vec3 specular = specularStrength * vec3(1.0, 1.0, 1.0);

    vec3 result = ambient + diffuse + specular;

    // Tone-map to avoid blown-out highlights on bright colors
    result = result / (result + vec3(0.6));
    result = pow(result, vec3(1.0 / 2.2));  // gamma correction

    FragColor = vec4(result, 1.0);
}
)GLSL";

// ============================================================
//  Cube vertex data  --  36 vertices (6 faces x 2 tris x 3 verts)
//
//  Layout per vertex: [ px, py, pz,  nx, ny, nz ]
//                       position(3)  normal(3)
//
//  The cube spans [-0.5, +0.5] on all axes.
//  meshScale will stretch it to each limb's final shape.
//
//  VAO setup:
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0)
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)))
// ============================================================
static const float CUBE_VERTICES[] = {
    // ---- +X face  (right)   normal = ( 1,  0,  0) ----
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,

    // ---- -X face  (left)    normal = (-1,  0,  0) ----
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

    // ---- +Y face  (top)     normal = ( 0,  1,  0) ----
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,

    // ---- -Y face  (bottom)  normal = ( 0, -1,  0) ----
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,

    // ---- +Z face  (front)   normal = ( 0,  0,  1) ----
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,

    // ---- -Z face  (back)    normal = ( 0,  0, -1) ----
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
};

// Number of vertices
static const int CUBE_VERTEX_COUNT = 36;

// Stride in bytes between consecutive vertices
static const int CUBE_STRIDE = 6 * sizeof(float);
