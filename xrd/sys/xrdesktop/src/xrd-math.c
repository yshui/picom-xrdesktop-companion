#include "xrd-math.h"

#include <inttypes.h>

#include <gxr.h>

#include "graphene-ext.h"

#define PI 3.1415926535f
#define DEG_TO_RAD(x) ( (x) * 2.0f * PI / 360.0f )
#define RAD_TO_DEG(x) ( (x) * 360.0f / ( 2.0f * PI ) )

float
xrd_math_point_matrix_distance (graphene_point3d_t *intersection_point,
                                graphene_matrix_t  *pose)
{
  graphene_vec3_t intersection_vec;
  graphene_point3d_to_vec3 (intersection_point, &intersection_vec);

  graphene_vec3_t pose_translation;
  graphene_ext_matrix_get_translation_vec3 (pose, &pose_translation);

  graphene_vec3_t distance_vec;
  graphene_vec3_subtract (&pose_translation,
                          &intersection_vec,
                          &distance_vec);

  return graphene_vec3_length (&distance_vec);
}

/**
 * xrd_math_get_rotation_angles:
 * @direction: a direction vector
 * @azimuth: clockwise angle in deg around the y axis, starting at -z:
 * "left-right" component.
 * @inclination: upwards angle in deg from the xz plane: "up-down" component.
 *
 * Calculate spherical angles from a direction vector with the direction
 * vector's origin as the origin of the sphere.
 *
 * Note that the distance (sphere radius) is not calculated. For converting
 * between spherical and 3D coordinates, the caller needs to keep track of it.
 */
void
xrd_math_get_rotation_angles (graphene_vec3_t *direction,
                              float *azimuth,
                              float *inclination)
{
  /* y axis = 90° up. angle diff to y axis when looking up = 0°: 90°-0°=90°
   * Looking up, angle to y axis shrinks to 0° -> 90°-0°=90° inclination.
   * Looking down, angle to y axis grows to -90° -> 90°--90°=-90° incl. */
  graphene_vec3_t y_axis;
  graphene_vec3_init_from_vec3 (&y_axis, graphene_vec3_y_axis ());
  graphene_vec3_t cross;
  graphene_vec3_cross (&y_axis, direction, &cross);
  float mag = graphene_vec3_length (&cross);
  float dot = graphene_vec3_dot (&y_axis, direction);
  *inclination =  (90 - RAD_TO_DEG (atan2f (mag, dot)));

  /* rotation around y axis, "left-right".
   * Negate z axis because z = -1 is forward */
  *azimuth =
      RAD_TO_DEG (atan2f (graphene_vec3_get_x (direction),
                          - graphene_vec3_get_z (direction)));
}

void
xrd_math_matrix_set_translation_point (graphene_matrix_t  *matrix,
                                       graphene_point3d_t *point)
{
  float m[16];
  graphene_matrix_to_float (matrix, m);

  m[12] = point->x;
  m[13] = point->y;
  m[14] = point->z;
  graphene_matrix_init_from_float (matrix, m);
}

void
xrd_math_matrix_set_translation_vec (graphene_matrix_t  *matrix,
                                     graphene_vec3_t *vec)
{
  float m[16];
  graphene_matrix_to_float (matrix, m);

  m[12] = graphene_vec3_get_x (vec);
  m[13] = graphene_vec3_get_y (vec);
  m[14] = graphene_vec3_get_z (vec);
  graphene_matrix_init_from_float (matrix, m);
}

/**
 * xrd_math_intersect_lines_2d:
 * @p0: The first point of the first line.
 * @p1: The second point of the first line.
 * @p2: The first point of the second line.
 * @p3: The second point of the second line.
 * @intersection: The resulting intersection point, if the lines intersect.
 *
 * Returns: TRUE if the lines intersect, else FALSE.
 *
 * 2 lines are given by 2 consecutive (x,y) points each.
 * Based on an algorithm in Andre LeMothe's
 * "Tricks of the Windows Game Programming Gurus".
 * Implementation from https://stackoverflow.com/a/1968345
 */
gboolean
xrd_math_intersect_lines_2d (graphene_point_t *p0, graphene_point_t *p1,
                             graphene_point_t *p2, graphene_point_t *p3,
                             graphene_point_t *intersection)
{
  graphene_point_t s1 = {
    .x = p1->x - p0->x,
    .y = p1->y - p0->y
  };

  graphene_point_t s2 = {
    .x = p3->x - p2->x,
    .y = p3->y - p2->y
  };

  float s, t;
  s = (-s1.y * (p0->x - p2->x) + s1.x * (p0->y - p2->y)) /
      (-s2.x * s1.y + s1.x * s2.y);
  t = ( s2.x * (p0->y - p2->y) - s2.y * (p0->x - p2->x)) /
      (-s2.x * s1.y + s1.x * s2.y);

  if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
      // Collision detected
      intersection->x = p0->x + (t * s1.x);
      intersection->y = p0->y + (t * s1.y);
      return TRUE;
    }
  return FALSE; // No collision
}

/**
 * xrd_math_clamp_towards_zero_2d:
 * @min: The (x,y) limit at the bottom left.
 * @max: The (x,y) limit at the top right.
 * @point: An (x,y) point, will be clamped if outside the min, max limits.
 * @clamped: The clamped point, if the point was outside the limits.
 *
 * Returns: TRUE if the point was clamped, else FALSE.
 *
 * The bottom left "min" and top right "max" limits define a rectangle.
 * Clamp a value to the borders of this rectangle such that both x and y go
 * towards zero, until a rectangle border is hit.
 */
gboolean
xrd_math_clamp_towards_zero_2d (graphene_point_t *min,
                                graphene_point_t *max,
                                graphene_point_t *point,
                                graphene_point_t *clamped)
{
  graphene_point_t zero = { .x = 0, .y = 0};

  graphene_point_t bottom_left = { min->x, min->y };
  graphene_point_t top_left = { min->x, max->y };
  graphene_point_t top_right = { max->x, max->y };
  graphene_point_t bottom_right = { max->x, min->y };

  /* left */
  if (xrd_math_intersect_lines_2d (&zero, point,
                                   &bottom_left, &top_left,
                                   clamped))
    return TRUE;

  /* right */
  if (xrd_math_intersect_lines_2d (&zero, point,
                                   &bottom_right, &top_right,
                                   clamped))
    return TRUE;

  /* top */
  if (xrd_math_intersect_lines_2d (&zero, point,
                                   &top_left, &top_right,
                                   clamped))
    return TRUE;

  /* bottom */
  if (xrd_math_intersect_lines_2d (&zero, point,
                                   &bottom_left, &bottom_right,
                                   clamped))
    return TRUE;


  return FALSE;
}

/**
 * xrd_math_sphere_to_3d_coords:
 * @azimuth: rotation around y axis, starting at -z. "left-right" component.
 * @inclination: rotation upwards from xz plane. "up-down" component".
 * @distance: the radius of the sphere
 * @point: the resulting point in 3D space on the surface of a sphere around
 * (0,0,0) with @distance.
 */
void
xrd_math_sphere_to_3d_coords (float azimuth,
                              float inclination,
                              float distance,
                              graphene_point3d_t *point)
{

  float dist_2d = distance * cosf (DEG_TO_RAD (inclination));
  graphene_point3d_init (point,
                         dist_2d * sinf (DEG_TO_RAD (azimuth)),
                         distance * sinf (DEG_TO_RAD (inclination)),
                         - dist_2d * cosf (DEG_TO_RAD (azimuth)));
}

