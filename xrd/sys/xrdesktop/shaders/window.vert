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
  mat4 mv;
  mat4 m;
  bool receive_light;
} transformation;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec4 out_world_position;
layout (location = 1) out vec4 out_view_position;
layout (location = 2) out vec2 out_uv;


out gl_PerVertex {
  vec4 gl_Position;
};

void main() {
  gl_Position = transformation.mvp * vec4 (position, 1.0f);
  gl_Position.y = -gl_Position.y;
  out_uv = uv;

  if (!transformation.receive_light)
    return;

  out_world_position = transformation.m * vec4 (position, 1.0f);
  out_view_position = transformation.mv * vec4 (position, 1.0f);
}
