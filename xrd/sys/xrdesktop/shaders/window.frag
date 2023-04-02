/*
 * gulkan
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#version 460
#extension GL_ARB_separate_shader_objects : enable

const float shininess = 16.0f;
const float ambient = 0.5f;

layout (location = 0) in vec4 world_position;
layout (location = 1) in vec4 view_position;
layout (location = 2) in vec2 uv;

layout (binding = 0) uniform Transformation {
  mat4 mvp;
  mat4 mv;
  mat4 m;
  bool receive_light;
} transformation;

layout (binding = 1) uniform sampler2D image;
layout (binding = 2) uniform Window {
  vec4 color;
  bool flip_y;
} window;

struct Light {
  vec4 position;
  vec3 color;
  float radius;
};

layout (binding = 3) uniform Lights
{
  Light lights[2];
  int active_lights;
} lights;

layout (location = 0) out vec4 out_color;

const float intensity = 2.0f;
const vec3 light_color_max = vec3(1.0f, 1.0f, 1.0f);

// "Lighten only" blending
vec3 lighten (vec3 a, vec3 b)
{
  vec3 c;
  c.r = max (a.r, b.r);
  c.g = max (a.g, b.g);
  c.b = max (a.b, b.b);
  return c;
}

void main ()
{
  vec2 uv_b = window.flip_y ? vec2 (uv.x, 1.0f - uv.y) : uv;
  vec4 texture_color = texture (image, uv_b);

  if (!transformation.receive_light)
    {
      out_color = texture_color * window.color;
      return;
    }

  vec4 diffuse = mix (texture_color * window.color, texture_color, 0.5f);

  vec3 lit = vec3 (0);

  float view_distance = length (view_position.xyz);

  for (int i = 0; i < lights.active_lights; i++)
    {
      vec3 L = lights.lights[i].position.xyz - world_position.xyz;
      float d = length (L);

      float radius = lights.lights[i].radius * view_distance;

      float atten = intensity / ((d / radius) + 1.0);
      vec3 light_gradient = mix (lights.lights[i].color.xyz,
                                 light_color_max,
                                 atten * 0.5f);
      lit += light_gradient * diffuse.rgb * atten;
    }

  out_color = vec4 (lighten (lit, diffuse.rgb), 1.0f);
}

