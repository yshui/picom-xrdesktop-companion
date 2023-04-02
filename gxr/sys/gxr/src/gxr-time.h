/*
 * gxr
 * Copyright 2018 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GXR_TIME_H_
#define GXR_TIME_H_

#if !defined (GXR_INSIDE) && !defined (GXR_COMPILATION)
#error "Only <gxr.h> can be included directly."
#endif

#include <time.h>
#include <glib.h>

#define SEC_IN_MSEC_D 1000.0
#define SEC_IN_USEC_D 1000000.0
#define SEC_IN_NSEC_L 1000000000L
#define SEC_IN_NSEC_D 1000000000.0

guint32
gxr_time_age_secs_to_monotonic_msecs (float age);

#endif /* GXR_TIME_H_ */
