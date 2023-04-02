/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;

layout (binding = 1) uniform sampler2D image;

layout (location = 0) out vec4 out_color;

void main ()
{
  out_color = texture (image, uv);
}

