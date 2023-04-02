/*
 * xrdesktop
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef XRD_WINDOW_MANAGER_H_
#define XRD_WINDOW_MANAGER_H_

#if !defined (XRD_INSIDE) && !defined (XRD_COMPILATION)
#error "Only <xrd.h> can be included directly."
#endif

#include <glib-object.h>

#include <gxr.h>

#include "xrd-window.h"
#include "xrd-container.h"

G_BEGIN_DECLS

#define XRD_TYPE_WINDOW_MANAGER xrd_window_manager_get_type()
G_DECLARE_FINAL_TYPE (XrdWindowManager, xrd_window_manager, XRD,
                      WINDOW_MANAGER, GObject)

/**
 * XrdNoHoverEvent:
 * @controller: A #GxrController.
 *
 * An event that gets emitted when a window is not being hovered anymore.
 **/
typedef struct {
  GxrController *controller;
} XrdNoHoverEvent;

/**
 * XrdTransformTransition:
 * @window: A #XrdWindow.
 * @from: The initial #graphene_matrix_t of the transiton.
 * @to: The final #graphene_matrix_t of the transition.
 * @from_scaling: The initial scale of the transition.
 * @to_scaling: The final scale of the transition.
 * @interpolate: The current state in the range [0-1] of the transition.
 * @last_timestamp: The last timestamp the transition was updated.
 *
 * A transition between two #XrdWindow transformation states.
 **/
typedef struct {
  XrdWindow *window;
  graphene_matrix_t from;
  graphene_matrix_t to;
  float from_scaling;
  float to_scaling;
  float interpolate;
  gint64 last_timestamp;
} XrdTransformTransition;

/**
 * XrdWindowFlags:
 * @XRD_WINDOW_HOVERABLE: Set if hover events should be generated.
 * @XRD_WINDOW_DRAGGABLE: Set if the window should be draggable.
 * @XRD_WINDOW_MANAGED: Set if window should be manipulated by window manager auto alignment.
 * @XRD_WINDOW_DESTROY_WITH_PARENT: Set if window should be destroyed with the window manager.
 * @XRD_WINDOW_BUTTON: Set if window is a button.
 *
 * Flags for the window manager.
 *
 **/
typedef enum
{
  XRD_WINDOW_HOVERABLE           = 1 << 0,
  XRD_WINDOW_DRAGGABLE           = 1 << 1,
  XRD_WINDOW_MANAGED             = 1 << 2,
  XRD_WINDOW_DESTROY_WITH_PARENT = 1 << 3,
  XRD_WINDOW_BUTTON              = 1 << 4,
} XrdWindowFlags;

/**
 * XrdHoverMode:
 * @XRD_HOVER_MODE_EVERYTHING: Buttons and windows should receive events.
 * @XRD_HOVER_MODE_BUTTONS: Only buttons should receive events.
 *
 * A mode where input events can be ignored for certain widgets.
 *
 **/
typedef enum
{
  XRD_HOVER_MODE_EVERYTHING,
  XRD_HOVER_MODE_BUTTONS
} XrdHoverMode;

XrdWindowManager *xrd_window_manager_new (void);

void
xrd_window_manager_arrange_reset (XrdWindowManager *self);

gboolean
xrd_window_manager_arrange_sphere (XrdWindowManager *self,
                                   GxrContext       *context);

void
xrd_window_manager_add_container (XrdWindowManager *self,
                                  XrdContainer *container);

void
xrd_window_manager_remove_container (XrdWindowManager *self,
                                     XrdContainer *container);

void
xrd_window_manager_add_window (XrdWindowManager *self,
                               XrdWindow        *window,
                               XrdWindowFlags    flags);

void
xrd_window_manager_remove_window (XrdWindowManager *self,
                                  XrdWindow        *window);

void
xrd_window_manager_drag_start (XrdWindowManager *self,
                               GxrController *controller);

void
xrd_window_manager_scale (XrdWindowManager *self,
                          GxrGrabState *grab_state,
                          float factor,
                          float update_rate_ms);

void
xrd_window_manager_check_grab (XrdWindowManager *self,
                               GxrController *controller);

void
xrd_window_manager_check_release (XrdWindowManager *self,
                                 GxrController *controller);

void
xrd_window_manager_update_controller (XrdWindowManager *self,
                                      GxrController *controller);

void
xrd_window_manager_poll_window_events (XrdWindowManager *self,
                                       GxrContext       *context);

GxrGrabState *
xrd_window_manager_get_grab_state (XrdWindowManager *self,
                                   GxrController *controller);

GxrHoverState *
xrd_window_manager_get_hover_state (XrdWindowManager *self,
                                    GxrController *controller);

GSList *
xrd_window_manager_get_windows (XrdWindowManager *self);

GSList *
xrd_window_manager_get_buttons (XrdWindowManager *self);

void
xrd_window_manager_set_hover_mode (XrdWindowManager *self,
                                   XrdHoverMode mode);

XrdHoverMode
xrd_window_manager_get_hover_mode (XrdWindowManager *self);

G_END_DECLS

#endif /* XRD_WINDOW_MANAGER_H_ */
