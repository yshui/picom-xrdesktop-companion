/*
 * xrdesktop
 * Copyright 2020 Collabora Ltd.
 * Author: Chrtstoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-render-lock.h"

static GRecMutex render_mutex;
static volatile gint initialized = 0;

void
xrd_render_lock_init ()
{
  g_atomic_int_compare_and_exchange (&initialized, 0, 1);
}

void
xrd_render_lock_destroy ()
{
  g_atomic_int_compare_and_exchange (&initialized, 1, 0);
}

void
xrd_render_lock ()
{
  if (g_atomic_int_get (&initialized) == 1)
  {
    //g_print ("Lock!\n");
    g_rec_mutex_lock (&render_mutex);
  }
}

void
xrd_render_unlock ()
{
  if (g_atomic_int_get (&initialized) == 1)
  {
    //g_print ("Unlock!\n");
    g_rec_mutex_unlock (&render_mutex);
  }
}
