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
#include <telepathy-glib/svc-call.h>

G_DEFINE_TYPE (ExampleCallContent,
    example_call_content,
    TP_TYPE_BASE_MEDIA_CALL_CONTENT)

struct _ExampleCallContentPrivate
{
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
dispose (GObject *object)
{
  ExampleCallContent *self = EXAMPLE_CALL_CONTENT (object);

  g_clear_object (&self->priv->stream);

  ((GObjectClass *) example_call_content_parent_class)->dispose (object);
}

static void
example_call_content_class_init (ExampleCallContentClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  g_type_class_add_private (klass,
      sizeof (ExampleCallContentPrivate));

  object_class->dispose = dispose;
}

ExampleCallStream *
example_call_content_get_stream (ExampleCallContent *self)
{
  g_return_val_if_fail (EXAMPLE_IS_CALL_CONTENT (self), NULL);

  return self->priv->stream;
}

void
example_call_content_add_stream (ExampleCallContent *self,
    ExampleCallStream *stream)
{
  g_return_if_fail (EXAMPLE_IS_CALL_CONTENT (self));
  g_return_if_fail (EXAMPLE_IS_CALL_STREAM (stream));
  g_return_if_fail (self->priv->stream == NULL);

  self->priv->stream = g_object_ref (stream);

  tp_base_call_content_add_stream ((TpBaseCallContent *) self,
      (TpBaseCallStream *) stream);
}

void
example_call_content_remove_stream (ExampleCallContent *self)
{
  TpBaseCallStream *stream;

  g_return_if_fail (EXAMPLE_IS_CALL_CONTENT (self));
  g_return_if_fail (self->priv->stream != NULL);

  stream = (TpBaseCallStream *) self->priv->stream;
  self->priv->stream = NULL;

  tp_base_call_content_remove_stream ((TpBaseCallContent *) self, stream,
      0, TP_CALL_STATE_CHANGE_REASON_UNKNOWN, "", "");

  g_object_unref (stream);
}
