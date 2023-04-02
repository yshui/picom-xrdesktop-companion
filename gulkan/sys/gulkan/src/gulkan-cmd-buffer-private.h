/*
 * gulkan
 * Copyright 2020 Collabora Ltd.
 * Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GULKAN_CMD_BUFFER_PRIVATE_H_
#define GULKAN_CMD_BUFFER_PRIVATE_H_

#include "gulkan-cmd-buffer.h"
#include "gulkan-device.h"

G_BEGIN_DECLS

typedef struct _GulkanQueue GulkanQueue;

GulkanCmdBuffer *gulkan_cmd_buffer_new (GulkanDevice *device,
                                        GulkanQueue  *queue);

G_END_DECLS

#endif /* GULKAN_CMD_BUFFER_PRIVATE_H_ */
