/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform Transformation {
  mat4 mvp;
} ubo;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 out_normal;
layout (location = 1) out vec2 out_uv;

out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = ubo.mvp * vec4 (position, 1.0);
  gl_Position.y = -gl_Position.y;
  out_uv = uv;
  out_normal = normal;
}
