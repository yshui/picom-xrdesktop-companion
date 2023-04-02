/*
 * Graphene Extensions
 * Copyright 2019 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_BUTTON_EXT_H_
#define XRD_BUTTON_EXT_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <cairo.h>

#include "xrd-window.h"

G_BEGIN_DECLS

void
xrd_button_set_text (XrdWindow    *button,
                     GulkanClient *client,
                     VkImageLayout upload_layout,
                     int           label_count,
                     gchar       **label);

void
xrd_button_set_icon (XrdWindow    *button,
                     GulkanClient *client,
                     VkImageLayout upload_layout,
                     const gchar  *url);

G_END_DECLS

#endif /* XRD_BUTTON_EXT_H_ */
