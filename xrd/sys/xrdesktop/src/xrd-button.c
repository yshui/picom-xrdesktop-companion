/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-button.h"
#include "xrd-math.h"
#include "xrd-render-lock.h"

#include <gdk/gdk.h>

static GdkPixbuf *
_load_pixbuf (const gchar* name)
{
  GError * error = NULL;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource (name, &error);

  if (error != NULL)
    {
      g_printerr ("Unable to read file: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  return pixbuf;
}

static void
_draw_background (cairo_t *cr,
                  uint32_t width,
                  uint32_t height)
{
  double r0;
  if (width < height)
    r0 = (double) width / 3.0;
  else
    r0 = (double) height / 3.0;

  double radius = r0 * 4.0;
  double r1 = r0 * 5.0;

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  double cx0 = center_x - r0 / 2.0;
  double cy0 = center_y - r0;
  double cx1 = center_x - r0;
  double cy1 = center_y - r0;

  cairo_pattern_t *pat = cairo_pattern_create_radial (cx0, cy0, r0,
                                                      cx1, cy1, r1);
  cairo_pattern_add_color_stop_rgba (pat, 0, .3, .3, .3, 1);
  cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
  cairo_set_source (cr, pat);
  cairo_arc (cr, center_x, center_y, radius, 0, 2 * M_PI);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
}

static void
_draw_icon (cairo_t *cr,
            GdkPixbuf *pixbuf,
            uint32_t width,
            int padding)
{
  /* Draw icon with padding */
  int icon_w = gdk_pixbuf_get_width (pixbuf);
  double scale = width / (double) (icon_w + 2 * padding);
  cairo_scale (cr, scale, scale);
  gdk_cairo_set_source_pixbuf (cr, pixbuf, padding, padding);
  cairo_paint (cr);
}

static cairo_surface_t*
_create_surface_icon (unsigned char *image, uint32_t width,
                      uint32_t height, const gchar *icon_url)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         (int) width, (int) height,
                                         (int) width * 4);

  cairo_t *cr = cairo_create (surface);
  _draw_background (cr, width, height);

  GdkPixbuf *icon_pixbuf = _load_pixbuf (icon_url);
  if (icon_pixbuf == NULL)
    {
      g_printerr ("Could not load icon %s.\n", icon_url);
      return NULL;
    }
  _draw_icon (cr, icon_pixbuf, width, 100);
  cairo_destroy (cr);
  g_object_unref (icon_pixbuf);

  return surface;
}

static void
_draw_text (cairo_t *cr, int lines,
            gchar *const *text,
            uint32_t width,
            uint32_t height)
{
  cairo_select_font_face (cr, "cairo :monospace",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);

  uint64_t longest_line = 0;
  for (int i = 0; i < lines; i++)
    {
      if (strlen (text[i]) > longest_line)
        longest_line = strlen (text[i]);
    }

  double font_size = 42;
  cairo_set_font_size (cr, font_size);

  double center_x = (double) width / 2.0;
  double center_y = (double) height / 2.0;

  for (int i = 0; i < lines; i++)
    {
      cairo_text_extents_t extents;
      cairo_text_extents (cr, text[i], &extents);

      /* horizontally centered*/
      double x = center_x - (double) extents.width / 2.0;

      double line_spacing = 0.25 * font_size;

      double y;
      if (lines == 1)
        y = .25 * font_size + center_y;
      else if (lines == 2)
        {
          if (i == 0)
            y = .25 * font_size + center_y - .5 * font_size - line_spacing / 2.;
          else
            y = .25 * font_size + center_y + .5 * font_size + line_spacing / 2.;
        }
      else
        /* TODO: better placement for more than 2 lines */
        y = font_size + line_spacing + i * font_size + i * line_spacing;

      cairo_move_to (cr, x, y);
      cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
      cairo_show_text (cr, text[i]);
    }
}

static cairo_surface_t*
_create_surface_text (unsigned char *image, uint32_t width,
                      uint32_t height, int lines,
                      gchar *const *text)
{
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (image,
                                         CAIRO_FORMAT_ARGB32,
                                         (int) width, (int) height,
                                         (int) width * 4);

  cairo_t *cr = cairo_create (surface);

  _draw_background (cr,  width, height);
  _draw_text (cr, lines, text, width, height);

  cairo_destroy (cr);

  return surface;
}

static VkExtent2D
_get_texture_size (XrdWindow *window)
{
  float width_meter = xrd_window_get_current_width_meters (window);
  float height_meter = xrd_window_get_current_height_meters (window);
  float ppm = xrd_window_get_current_ppm (window);

  // TODO: Global default ppm setting for interal textures
  if (ppm == 0)
    ppm = 450;

  VkExtent2D size = {
    .width = (uint32_t) (width_meter * ppm),
    .height = (uint32_t) (height_meter * ppm)
  };

  return size;
}

static void
_submit_cairo_surface (XrdWindow    *button,
                       GulkanClient *client,
                       VkImageLayout upload_layout,
                       cairo_surface_t* surface)
{
  GulkanTexture *texture =
    gulkan_texture_new_from_cairo_surface (client, surface,
                                           VK_FORMAT_R8G8B8A8_SRGB,
                                           upload_layout);

  if (!texture)
    {
      g_printerr ("Could not create texture from cairo surface.\n");
      xrd_render_unlock ();
      return;
    }

  xrd_window_set_and_submit_texture (button, texture);
}

void
xrd_button_set_text (XrdWindow    *button,
                     GulkanClient *client,
                     VkImageLayout upload_layout,
                     int           label_count,
                     gchar       **label)
{
  VkExtent2D dim = _get_texture_size (button);

  gsize size = sizeof(unsigned char) * 4 * dim.width * dim.height;
  unsigned char* image = g_malloc (size);

  cairo_surface_t* surface =
    _create_surface_text (image, dim.width, dim.height, label_count, label);

  if (!surface)
    {
      g_printerr ("Could not create cairo surface.\n");
      return;
    }

  _submit_cairo_surface (button, client, upload_layout, surface);

  g_free (image);

  cairo_surface_destroy (surface);
}

void
xrd_button_set_icon (XrdWindow    *button,
                     GulkanClient *client,
                     VkImageLayout upload_layout,
                     const gchar  *url)
{

  VkExtent2D dim = _get_texture_size (button);

  gsize size = sizeof(unsigned char) * 4 * dim.width * dim.height;
  unsigned char* image = g_malloc (size);

  cairo_surface_t* surface =
    _create_surface_icon (image, dim.width, dim.height, url);

  if (!surface)
    {
      g_printerr ("Could not create cairo surface.\n");
      return;
    }

  _submit_cairo_surface (button, client, upload_layout, surface);

  g_free (image);

  cairo_surface_destroy (surface);
}
