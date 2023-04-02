/*
 * xrdesktop
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "xrd-settings.h"

#define SCHEMA_ID "org.xrdesktop"

static GSettings *instance = NULL;

gboolean
xrd_settings_is_schema_installed ()
{
  GSettingsSchemaSource *source =
    g_settings_schema_source_get_default ();
  GSettingsSchema *schema =
    g_settings_schema_source_lookup (source, SCHEMA_ID, TRUE);

  gboolean installed = schema != NULL;

  if (schema)
    g_settings_schema_unref (schema);

  return installed;
}

GSettings *
xrd_settings_get_instance ()
{
  if (!instance)
    instance = g_settings_new (SCHEMA_ID);
  return instance;
}

void
xrd_settings_destroy_instance ()
{
  if (instance)
    {
      // shouldn't happen, but better be safe
      gboolean has_unapplied;
      g_object_get (instance, "has-unapplied", &has_unapplied, NULL);
      if (has_unapplied)
        g_settings_apply (instance);
      g_object_unref (instance);
    }
  instance = NULL;
}

typedef void (*settings_callback) (GSettings *settings,
                                   gchar     *key,
                                   gpointer   user_data);

/**
 * xrd_settings_connect_and_apply:
 * @callback: A function that will be called with the given
 * @key and @data 1) immediately and 2) when the value for the given key is
 * updated.
 * @key: The settings key
 * @data: A pointer that will be passed to the update callback.
 *
 * Use this convenience function when you don't want to initially read a config
 * value from the settings, and then connect a callback to when the value
 * changes.
 *
 * Instead write only one callback that handles initially setting the value, as
 * well as any updates to this value.
 */
void
xrd_settings_connect_and_apply (GCallback callback, gchar *key, gpointer data)
{
  GSettings *settings = xrd_settings_get_instance ();

  settings_callback cb = (settings_callback) callback;
  cb (settings, key, data);

  GString *detailed_signal = g_string_new ("changed::");
  g_string_append (detailed_signal, key);

  g_signal_connect (settings, detailed_signal->str, callback, data);

  g_string_free (detailed_signal, TRUE);
}

/**
 * xrd_settings_update_double_val:
 * @settings: The gsettings
 * @key: The key
 * @val: Pointer to a double value to be updated
 *
 * Convencience callback that can be registered when no action is required on
 * an update of a double value.
 * */
void
xrd_settings_update_double_val (GSettings *settings,
                                gchar *key,
                                double *val)
{
  *val = g_settings_get_double (settings, key);
}

/**
 * xrd_settings_update_int_val:
 * @settings: The gsettings
 * @key: The key
 * @val: Pointer to an int value to be updated
 *
 * Convencience callback that can be registered when no action is required on
 * an update of a int value.
 * */
void
xrd_settings_update_int_val (GSettings *settings,
                             gchar *key,
                             int *val)
{
  *val = g_settings_get_int (settings, key);
}

/**
 * xrd_settings_update_gboolean_val:
 * @settings: The gsettings
 * @key: The key
 * @val: Pointer to an gboolean value to be updated
 *
 * Convencience callback that can be registered when no action is required on
 * an update of a gboolean value.
 * */
void
xrd_settings_update_gboolean_val (GSettings *settings,
                             gchar *key,
                             gboolean *val)
{
  *val = g_settings_get_boolean (settings, key);
}

