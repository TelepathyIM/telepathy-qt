/*
 * call-content.c - a content in a call.
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "call-content.h"

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/telepathy-glib.h>

#include "extensions/extensions.h"

G_DEFINE_TYPE_WITH_CODE (ExampleCallContent,
    example_call_content,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    /* no methods, so no vtable needed */
    G_IMPLEMENT_INTERFACE (FUTURE_TYPE_SVC_CALL_CONTENT, NULL))

enum
{
  PROP_OBJECT_PATH = 1,
  PROP_CONNECTION,
  PROP_NAME,
  PROP_TYPE,
  PROP_CREATOR,
  PROP_DISPOSITION,
  PROP_STREAM_PATHS,
  N_PROPS
};

struct _ExampleCallContentPrivate
{
  gchar *object_path;
  TpBaseConnection *conn;
  gchar *name;
  TpMediaStreamType type;
  TpHandle creator;
  FutureCallContentDisposition disposition;
  ExampleCallStream *stream;
};

static void
example_call_content_init (ExampleCallContent *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CALL_CONTENT,
      ExampleCallContentPrivate);
}

static void
constructed (GObject *object)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_call_content_parent_class)->constructed;
  TpHandleRepoIface *contact_repo;

  if (chain_up != NULL)
    chain_up (object);

  tp_dbus_daemon_register_object (
      tp_base_connection_get_dbus_daemon (self->priv->conn),
      self->priv->object_path, self);

  contact_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_CONTACT);
  tp_handle_ref (contact_repo, self->priv->creator);
}

static void
get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, self->priv->object_path);
      break;

    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->priv->name);
      break;

    case PROP_CREATOR:
      g_value_set_uint (value, self->priv->creator);
      break;

    case PROP_TYPE:
      g_value_set_uint (value, self->priv->type);
      break;

    case PROP_DISPOSITION:
      g_value_set_uint (value, self->priv->disposition);
      break;

    case PROP_STREAM_PATHS:
        {
          GPtrArray *paths = g_ptr_array_sized_new (1);

          if (self->priv->stream != NULL)
            {
              gchar *path;

              g_object_get (self->priv->stream,
                  "object-path", &path,
                  NULL);

              g_ptr_array_add (paths, path);
            }

          g_value_take_boxed (value, paths);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);

  switch (property_id)
    {
    case PROP_OBJECT_PATH:
      g_assert (self->priv->object_path == NULL);   /* construct-only */
      self->priv->object_path = g_value_dup_string (value);
      break;

    case PROP_CONNECTION:
      g_assert (self->priv->conn == NULL);
      self->priv->conn = g_value_dup_object (value);
      break;

    case PROP_CREATOR:
      /* we don't ref it here because we don't necessarily have access to the
       * contact repo yet - instead we ref it in the constructor.
       */
      g_assert (self->priv->creator == 0);    /* construct-only */
      self->priv->creator = g_value_get_uint (value);
      break;

    case PROP_NAME:
      g_assert (self->priv->name == NULL);    /* construct-only */
      self->priv->name = g_value_dup_string (value);
      break;

    case PROP_TYPE:
      self->priv->type = g_value_get_uint (value);
      break;

    case PROP_DISPOSITION:
      self->priv->disposition = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dispose (GObject *object)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);

  if (self->priv->stream != NULL)
    {
      g_object_unref (self->priv->stream);
      self->priv->stream = NULL;
    }

  if (self->priv->conn != NULL)
    {
      TpHandleRepoIface *contact_handles = tp_base_connection_get_handles
          (self->priv->conn, TP_HANDLE_TYPE_CONTACT);

      tp_handle_unref (contact_handles, self->priv->creator);
      self->priv->creator = 0;

      g_object_unref (self->priv->conn);
      self->priv->conn = NULL;
    }

  ((GObjectClass *) example_call_content_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);
  void (*chain_up) (GObject *) =
    ((GObjectClass *) example_call_content_parent_class)->finalize;

  g_free (self->priv->object_path);
  g_free (self->priv->name);

  if (chain_up != NULL)
    chain_up (object);
}

static void
example_call_content_class_init (ExampleCallContentClass *klass)
{
  static TpDBusPropertiesMixinPropImpl content_props[] = {
      { "Name", "name", NULL },
      { "Type", "type", NULL },
      { "Creator", "creator", NULL },
      { "Disposition", "disposition", NULL },
      { "Streams", "stream-paths", NULL },
      { NULL }
  };
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { FUTURE_IFACE_CALL_CONTENT,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        content_props,
      },
      { NULL }
  };
  GObjectClass *object_class = (GObjectClass *) klass;
  GParamSpec *param_spec;

  g_type_class_add_private (klass,
      sizeof (ExampleCallContentPrivate));

  object_class->constructed = constructed;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->dispose = dispose;
  object_class->finalize = finalize;

  param_spec = g_param_spec_string ("object-path", "D-Bus object path",
      "The D-Bus object path used for this object on the bus.", NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_OBJECT_PATH, param_spec);

  param_spec = g_param_spec_object ("connection", "TpBaseConnection",
      "Connection that owns this content",
      TP_TYPE_BASE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_uint ("type", "TpMediaStreamType",
      "Media stream type",
      0, NUM_TP_MEDIA_STREAM_TYPES - 1, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TYPE, param_spec);

  param_spec = g_param_spec_uint ("disposition",
      "FutureCallContentDisposition",
      "Disposition of the content",
      0, NUM_FUTURE_CALL_CONTENT_DISPOSITIONS - 1,
      FUTURE_CALL_CONTENT_DISPOSITION_NONE,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DISPOSITION, param_spec);

  param_spec = g_param_spec_uint ("creator", "Creator's handle",
      "The contact who initiated this content",
      0, G_MAXUINT32, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CREATOR, param_spec);

  param_spec = g_param_spec_string ("name", "Content name",
      "The name of the content",
      NULL,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_NAME, param_spec);

  param_spec = g_param_spec_boxed ("stream-paths", "Stream paths",
      "Streams' object paths",
      TP_ARRAY_TYPE_OBJECT_PATH_LIST,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_STREAM_PATHS,
      param_spec);

  klass->dbus_properties_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleCallContentClass,
        dbus_properties_class));
}

ExampleCallStream *
example_call_content_get_stream (ExampleCallContent *self)
{
  g_return_val_if_fail (EXAMPLE_IS_CALL_CONTENT (self), NULL);
  return self->priv->stream;
}

static void
example_call_content_stream_removed_cb (ExampleCallContent *self,
    ExampleCallStream *stream)
{
  gchar *path;

  g_return_if_fail (EXAMPLE_IS_CALL_CONTENT (self));
  g_return_if_fail (EXAMPLE_IS_CALL_STREAM (stream));
  g_return_if_fail (self->priv->stream == stream);

  g_object_get (stream,
      "object-path", &path,
      NULL);
  future_svc_call_content_emit_stream_removed (self, path);
  g_free (path);

  g_object_unref (self->priv->stream);
  self->priv->stream = NULL;
}

void
example_call_content_add_stream (ExampleCallContent *self,
    ExampleCallStream *stream)
{
  gchar *path;

  g_return_if_fail (EXAMPLE_IS_CALL_CONTENT (self));
  g_return_if_fail (EXAMPLE_IS_CALL_STREAM (stream));
  g_return_if_fail (self->priv->stream == NULL);

  self->priv->stream = g_object_ref (stream);
  g_object_get (stream,
      "object-path", &path,
      NULL);
  future_svc_call_content_emit_stream_added (self, path);
  g_free (path);

  tp_g_signal_connect_object (stream, "removed",
      G_CALLBACK (example_call_content_stream_removed_cb), self,
      G_CONNECT_SWAPPED);
}
