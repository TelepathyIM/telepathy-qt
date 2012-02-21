/*
 * call-content.h - header for an example content
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

#ifndef EXAMPLE_CALL_CONTENT_H
#define EXAMPLE_CALL_CONTENT_H

#include <glib-object.h>

#include <telepathy-glib/telepathy-glib.h>

#include "call-stream.h"

G_BEGIN_DECLS

typedef struct _ExampleCallContent ExampleCallContent;
typedef struct _ExampleCallContentPrivate
    ExampleCallContentPrivate;

typedef struct _ExampleCallContentClass
    ExampleCallContentClass;
typedef struct _ExampleCallContentClassPrivate
    ExampleCallContentClassPrivate;

GType example_call_content_get_type (void);

#define EXAMPLE_TYPE_CALL_CONTENT \
  (example_call_content_get_type ())
#define EXAMPLE_CALL_CONTENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CALL_CONTENT, \
                               ExampleCallContent))
#define EXAMPLE_CALL_CONTENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CALL_CONTENT, \
                            ExampleCallContentClass))
#define EXAMPLE_IS_CALL_CONTENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CALL_CONTENT))
#define EXAMPLE_IS_CALL_CONTENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CALL_CONTENT))
#define EXAMPLE_CALL_CONTENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALL_CONTENT, \
                              ExampleCallContentClass))

struct _ExampleCallContentClass {
    TpBaseMediaCallContentClass parent_class;

    ExampleCallContentClassPrivate *priv;
};

struct _ExampleCallContent {
    TpBaseMediaCallContent parent;

    ExampleCallContentPrivate *priv;
};

/* In this example, each content can only have one stream. */
ExampleCallStream *example_call_content_get_stream (ExampleCallContent *self);

void example_call_content_add_stream (ExampleCallContent *self,
    ExampleCallStream *stream);

void example_call_content_remove_stream (ExampleCallContent *self);

G_END_DECLS

#endif
