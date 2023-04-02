/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-time.h"

guint32
gxr_time_age_secs_to_monotonic_msecs (float age)
{
  gint64 mono_time = g_get_monotonic_time ();
  gint64 event_age = (gint64) (age * SEC_IN_USEC_D);
  gint64 time_usec = mono_time - event_age;

  return (guint32) (((double)time_usec / SEC_IN_USEC_D) * SEC_IN_MSEC_D);
}

