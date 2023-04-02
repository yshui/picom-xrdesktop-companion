/*
 * gxr
 * Copyright 2019 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 */

#include "gxr-manifest.h"

#include <json-glib/json-glib.h>

struct _GxrManifest
{
  GObject parent;

  GSList *action_manifest_entries;

  /* list of GxrBindingManifest */
  GSList *bindings;

  GSList *binding_filenames;
};

G_DEFINE_TYPE (GxrManifest, gxr_manifest, G_TYPE_OBJECT)

static void
gxr_manifest_finalize (GObject *gobject);

static void
gxr_manifest_class_init (GxrManifestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gxr_manifest_finalize;
}

static void
gxr_manifest_init (GxrManifest *self)
{
  self->action_manifest_entries = NULL;
  self->bindings = NULL;
  self->binding_filenames = NULL;
}

static GxrBindingType
_get_binding_type (const gchar *type_string)
{
  if (g_str_equal (type_string, "boolean"))
    return GXR_BINDING_TYPE_BOOLEAN;
  if (g_str_equal (type_string, "vector1"))
    return GXR_BINDING_TYPE_FLOAT;
  if (g_str_equal (type_string, "vector2"))
    return GXR_BINDING_TYPE_VEC2;
  if (g_str_equal (type_string, "pose"))
    return GXR_BINDING_TYPE_POSE;
  if (g_str_equal (type_string, "vibration"))
    return GXR_BINDING_TYPE_HAPTIC;

  g_printerr ("Binding type %s is not known\n", type_string);
  return GXR_BINDING_TYPE_UNKNOWN;
}

static GxrBindingMode
_get_binding_mode (const gchar *mode_string)
{
  if (mode_string == NULL)
    return GXR_BINDING_MODE_NONE;
  if (g_str_equal (mode_string, "button"))
    return GXR_BINDING_MODE_BUTTON;
  if (g_str_equal (mode_string, "trackpad"))
    return GXR_BINDING_MODE_TRACKPAD;
  if (g_str_equal (mode_string, "joystick"))
    return GXR_BINDING_MODE_ANALOG_STICK;

  g_printerr ("Binding mode %s is not known\n", mode_string);
  return GXR_BINDING_MODE_UNKNOWN;
}

static GxrBindingComponent
_get_binding_component (const gchar *component_string)
{
  if (g_str_equal (component_string, "click"))
    return GXR_BINDING_COMPONENT_CLICK;
  if (g_str_equal (component_string, "pull"))
    return GXR_BINDING_COMPONENT_PULL;
  if (g_str_equal (component_string, "position"))
    return GXR_BINDING_COMPONENT_POSITION;
  if (g_str_equal (component_string, "touch"))
    return GXR_BINDING_COMPONENT_TOUCH;
  if (g_str_equal (component_string, "force"))
    return GXR_BINDING_COMPONENT_FORCE;

  g_printerr ("Binding component %s is not known\n", component_string);
  return GXR_BINDING_COMPONENT_UNKNOWN;
}

static gboolean
_parse_default_binding_filenames (GxrManifest *self, JsonObject *joroot)
{
  JsonArray *jobindings =
    json_object_get_array_member (joroot, "default_bindings");

  guint len = json_array_get_length (jobindings);
  g_debug ("parsing %d default binding filenames", len);

  for (guint i = 0; i < len; i++)
    {
      JsonObject *jobinding = json_array_get_object_element (jobindings, i);
      const gchar *controller_type =
        json_object_get_string_member (jobinding, "controller_type");
      const gchar *binding_url =
        json_object_get_string_member (jobinding, "binding_url");

      g_debug ("Parsed default binding filename %s: %s",
               controller_type, binding_url);

      self->binding_filenames =
        g_slist_append (self->binding_filenames, g_strdup (binding_url));
    }
  return TRUE;
}

static gboolean
_parse_actions (GxrManifest *self, GInputStream *stream)
{
  GError *error = NULL;

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_stream (parser, stream, NULL, &error);
  if (error)
    {
      g_print ("Unable to parse actions: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return FALSE;
    }

  JsonNode *jnroot = json_parser_get_root (parser);

  JsonObject *joroot;
  joroot = json_node_get_object (jnroot);

  _parse_default_binding_filenames (self, joroot);

  JsonArray *joactions = json_object_get_array_member (joroot, "actions");

  guint len = json_array_get_length (joactions);
  g_debug ("parsing %d actions", len);

  for (guint i = 0; i < len; i++)
    {
      JsonObject *joaction = json_array_get_object_element (joactions, i);
      const gchar *name = json_object_get_string_member (joaction, "name");
      const gchar *binding_type =
        json_object_get_string_member (joaction, "type");

      g_debug ("\tParsed action %s: %s", name, binding_type);

      GxrActionManifestEntry *action =
        g_malloc (sizeof (GxrActionManifestEntry));
      action->name = g_strdup (name);
      action->type = _get_binding_type (binding_type);

      self->action_manifest_entries =
        g_slist_append (self->action_manifest_entries, action);
    }
  g_object_unref (parser);

  return TRUE;
}

static GxrActionManifestEntry *
_find_action_manifest_entry (GxrManifest *self, const gchar *action_name)
{
  for (GSList *l = self->action_manifest_entries; l; l = l->next)
    {
      GxrActionManifestEntry *action = l->data;
      if (g_strcmp0 (action_name, action->name) == 0)
        {
          return action;
        }
    }
  return NULL;
}

static GxrBinding *
_find_binding_for_action (GxrBindingManifest     *bindings,
                          GxrActionManifestEntry *action)
{
  for (GSList *l = bindings->gxr_bindings; l; l = l->next)
    {
      GxrBinding *binding = l->data;
      if (binding->action == action)
        {
          return binding;
        }
    }
  return NULL;
}

static void
_add_input_path_or_binding (GxrManifest *self,
                            GxrBindingManifest *bindings,
                            const gchar *action_name,
                            GxrBindingMode mode,
                            GxrBindingComponent component,
                            const gchar *input_path)
{
  GxrActionManifestEntry *action =
    _find_action_manifest_entry (self, action_name);

  if (action == NULL)
    {
      g_printerr ("%s: Attempted to add input %s for action %s "
                  " which is not in action manifest!",
                  bindings->filename, input_path, action_name);
      return;
    }

  GxrBinding *binding = _find_binding_for_action (bindings, action);
  if (binding == NULL)
    {
      binding = g_malloc (sizeof (GxrBinding));
      binding->action = action;
      binding->input_paths = NULL;
      bindings->gxr_bindings = g_slist_append (bindings->gxr_bindings, binding);
    }

  GxrBindingPath *binding_path = g_malloc (sizeof (GxrBindingPath));
  binding_path->component = component;
  binding_path->path = g_strdup (input_path);
  binding_path->mode = mode;

  binding->input_paths = g_slist_append (binding->input_paths, binding_path);
}

static void
_parse_sources (GxrManifest        *self,
                JsonArray          *jasources,
                GxrBindingManifest *bindings)
{
  guint sources_len = json_array_get_length (jasources);
  for (guint i = 0; i < sources_len; i++)
    {
      JsonObject *josource = json_array_get_object_element (jasources, i);

      const gchar *path = json_object_get_string_member (josource, "path");
      const gchar *mode = NULL;
      if (json_object_has_member (josource, "mode"))
        json_object_get_string_member (josource, "mode");

      // g_debug ("\tParsed path %s with mode %s\n", path, mode);

      JsonObject *joinputs = json_object_get_object_member (josource, "inputs");

      GList *input_list = json_object_get_members (joinputs);
      for (GList *m = input_list; m != NULL; m = m->next)
        {
          const gchar *component = m->data;
          JsonObject *joinput =
            json_object_get_object_member (joinputs, component);
          const gchar *action_name =
            json_object_get_string_member (joinput, "output");
          g_debug ("\t%s: Parsed output %s for component %s",
                   path, action_name, component);

          _add_input_path_or_binding (self, bindings, (gchar*)action_name,
                                      _get_binding_mode(mode),
                                      _get_binding_component(component), path);
        }
      g_list_free (input_list);
    }
}

static void
_parse_haptics (GxrManifest        *self,
                JsonArray          *jahaptics,
                GxrBindingManifest *bindings)
{
  guint haptics_len = json_array_get_length (jahaptics);
  for (guint i = 0; i < haptics_len; i++)
    {
      JsonObject *johaptics = json_array_get_object_element (jahaptics, i);

      const gchar *path =
        json_object_get_string_member (johaptics, "path");
      const gchar *action_name =
        json_object_get_string_member (johaptics, "output");

      g_debug ("\t%s: Parsed haptics %s", path, action_name);

      _add_input_path_or_binding (self, bindings, (gchar*)action_name,
                                  GXR_BINDING_MODE_NONE,
                                  GXR_BINDING_COMPONENT_NONE, path);
    }
}

static void
_parse_poses (GxrManifest        *self,
              JsonArray          *japose,
              GxrBindingManifest *bindings)
{
  guint pose_len = json_array_get_length (japose);
  for (guint i = 0; i < pose_len; i++)
    {
      JsonObject *jopose = json_array_get_object_element (japose, i);

      const gchar *path =
        json_object_get_string_member (jopose, "path");
      const gchar *action_name =
        json_object_get_string_member (jopose, "output");

      g_debug ("\t%s: Parsed pose %s", path, action_name);

      _add_input_path_or_binding (self, bindings, (gchar*)action_name,
                                  GXR_BINDING_MODE_NONE,
                                  GXR_BINDING_COMPONENT_NONE, path);
    }
}

static gboolean
_parse_bindings (GxrManifest        *self,
                 GInputStream       *stream,
                 GxrBindingManifest *bindings)
{
  GError *error = NULL;

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_stream (parser, stream, NULL, &error);
  if (error)
    {
      g_printerr ("Unable to parse bindings: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return FALSE;
    }

  JsonNode *jnroot = json_parser_get_root (parser);

  JsonObject *joroot;
  joroot = json_node_get_object (jnroot);

  JsonObject *jobinding = json_object_get_object_member (joroot, "bindings");

  if (json_object_has_member (joroot, "interaction_profile"))
    bindings->interaction_profile =
      g_strdup (json_object_get_string_member (joroot, "interaction_profile"));

  GList *binding_list = json_object_get_members (jobinding);
  for (GList *l = binding_list; l != NULL; l = l->next)
    {
      const gchar *actionset = l->data;
      g_debug ("Parsing Action Set %s", actionset);

      JsonObject *joactionset =
        json_object_get_object_member (jobinding, actionset);

      if (json_object_has_member (joactionset, "sources"))
        {
          JsonArray *jasources =
            json_object_get_array_member (joactionset, "sources");
          _parse_sources (self, jasources, bindings);
        }

      if (json_object_has_member (joactionset, "haptics"))
        {
          JsonArray *jahaptics =
            json_object_get_array_member (joactionset, "haptics");
          _parse_haptics (self, jahaptics, bindings);
        }

      if (json_object_has_member (joactionset, "poses"))
        {
          JsonArray *japose =
            json_object_get_array_member (joactionset, "poses");
          _parse_poses (self, japose, bindings);
        }
    }
  g_list_free (binding_list);

  g_object_unref (parser);

  return TRUE;
}

gboolean
gxr_manifest_load_actions (GxrManifest *self,
                           GInputStream *action_stream)
{
  if (!_parse_actions (self, action_stream))
    return FALSE;

  /* actions manifest parsing and bindings parsing are separate because
   * for openvr we only need binding filenames from actions manifest. */

  return TRUE;
}

gboolean
gxr_manifest_load_bindings (GxrManifest *self, const char *resource_path)
{
  for (GSList *l = self->binding_filenames; l; l = l->next)
    {
      gchar *binding_filename = l->data;

      GError *error = NULL;

      GString *bindings_res_path = g_string_new ("");
      g_string_printf (bindings_res_path, "%s/%s",
                       resource_path, binding_filename);
      g_debug ("Parsing bindings file %s", bindings_res_path->str);

      GInputStream *bindings_res_input_stream =
        g_resources_open_stream (bindings_res_path->str,
                                 G_RESOURCE_LOOKUP_FLAGS_NONE,
                                &error);
      if (error)
        {
          g_printerr ("skipping %s: %s\n", binding_filename, error->message);
          g_error_free (error);
          g_string_free (bindings_res_path, TRUE);
          g_object_unref (bindings_res_input_stream);
          continue;
        }


      GxrBindingManifest *bindings = g_malloc (sizeof (GxrBindingManifest));
      bindings->gxr_bindings = NULL;
      bindings->interaction_profile = NULL;

      bindings->filename = g_strdup (binding_filename);

      if (!_parse_bindings (self, bindings_res_input_stream, bindings))
        {
          g_free (bindings);
          g_printerr ("Failed to parse bindings manifest %s\n",
                      bindings_res_path->str);

          g_string_free (bindings_res_path, TRUE);
          g_object_unref (bindings_res_input_stream);
          continue;
        }

      self->bindings = g_slist_append (self->bindings, bindings);

      g_string_free (bindings_res_path, TRUE);
      g_object_unref (bindings_res_input_stream);
    }
  return TRUE;
}

GSList *
gxr_manifest_get_binding_filenames (GxrManifest *self)
{
  return self->binding_filenames;
}

GxrManifest *
gxr_manifest_new (void)
{
  return (GxrManifest *) g_object_new (GXR_TYPE_MANIFEST, 0);
}

GSList *
gxr_manifest_get_binding_manifests (GxrManifest *self)
{
  return self->bindings;
}

static void
gxr_manifest_finalize (GObject *gobject)
{
  GxrManifest *self = GXR_MANIFEST (gobject);

  for (GSList *l = self->action_manifest_entries; l; l = l->next)
    {
      GxrActionManifestEntry *action = l->data;
      g_free (action->name);
    }
  g_slist_free_full (self->action_manifest_entries, g_free);

  g_slist_free_full (self->binding_filenames, g_free);

  for (GSList *l = self->bindings ; l; l = l->next)
    {
      GxrBindingManifest *binding_manifest = l->data;
      for (GSList *m = binding_manifest->gxr_bindings; m; m = m->next)
        {
          GxrBinding *binding = m->data;
          for (GSList *n = binding->input_paths; n; n = n->next)
            {
              GxrBindingPath *binding_path = n->data;
              g_free (binding_path->path);
            }
          g_slist_free_full (binding->input_paths, g_free);
        }
      g_slist_free_full (binding_manifest->gxr_bindings, g_free);
      g_free (binding_manifest->interaction_profile);
      g_free (binding_manifest->filename);
    }

  g_slist_free_full (self->bindings, g_free);
}
