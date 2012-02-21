/*
 * call-stream.h - header for an example stream
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

#ifndef EXAMPLE_CALL_STREAM_H
#define EXAMPLE_CALL_STREAM_H

#include <glib-object.h>

#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

typedef struct _ExampleCallStream ExampleCallStream;
typedef struct _ExampleCallStreamPrivate
    ExampleCallStreamPrivate;

typedef struct _ExampleCallStreamClass
    ExampleCallStreamClass;
typedef struct _ExampleCallStreamClassPrivate
    ExampleCallStreamClassPrivate;

GType example_call_stream_get_type (void);

#define EXAMPLE_TYPE_CALL_STREAM \
  (example_call_stream_get_type ())
#define EXAMPLE_CALL_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CALL_STREAM, \
                               ExampleCallStream))
#define EXAMPLE_CALL_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CALL_STREAM, \
                            ExampleCallStreamClass))
#define EXAMPLE_IS_CALL_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CALL_STREAM))
#define EXAMPLE_IS_CALL_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CALL_STREAM))
#define EXAMPLE_CALL_STREAM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALL_STREAM, \
                              ExampleCallStreamClass))

struct _ExampleCallStreamClass {
    TpBaseMediaCallStreamClass parent_class;

    ExampleCallStreamClassPrivate *priv;
};

struct _ExampleCallStream {
    TpBaseMediaCallStream parent;

    ExampleCallStreamPrivate *priv;
};

void example_call_stream_accept_proposed_direction (ExampleCallStream *self);
void example_call_stream_connect (ExampleCallStream *self);

/* This controls receiving emulated network events, so it wouldn't exist in
 * a real connection manager */
void example_call_stream_simulate_contact_agreed_to_send (
    ExampleCallStream *self);

G_END_DECLS

#endif
