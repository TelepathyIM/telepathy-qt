/*
 * media-stream.h - header for an example stream
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

#ifndef __EXAMPLE_CALLABLE_MEDIA_STREAM_H__
#define __EXAMPLE_CALLABLE_MEDIA_STREAM_H__

#include <glib-object.h>

#include <telepathy-glib/enums.h>

G_BEGIN_DECLS

typedef struct _ExampleCallableMediaStream ExampleCallableMediaStream;
typedef struct _ExampleCallableMediaStreamPrivate
    ExampleCallableMediaStreamPrivate;

typedef struct _ExampleCallableMediaStreamClass
    ExampleCallableMediaStreamClass;
typedef struct _ExampleCallableMediaStreamClassPrivate
    ExampleCallableMediaStreamClassPrivate;

GType example_callable_media_stream_get_type (void);

#define EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM \
  (example_callable_media_stream_get_type ())
#define EXAMPLE_CALLABLE_MEDIA_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM, \
                               ExampleCallableMediaStream))
#define EXAMPLE_CALLABLE_MEDIA_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM, \
                            ExampleCallableMediaStreamClass))
#define EXAMPLE_IS_CALLABLE_MEDIA_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM))
#define EXAMPLE_IS_CALLABLE_MEDIA_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM))
#define EXAMPLE_CALLABLE_MEDIA_STREAM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_STREAM, \
                              ExampleCallableMediaStreamClass))

struct _ExampleCallableMediaStreamClass {
    GObjectClass parent_class;

    ExampleCallableMediaStreamClassPrivate *priv;
};

struct _ExampleCallableMediaStream {
    GObject parent;

    ExampleCallableMediaStreamPrivate *priv;
};

void example_callable_media_stream_close (ExampleCallableMediaStream *self);
gboolean example_callable_media_stream_change_direction (
    ExampleCallableMediaStream *self, TpMediaStreamDirection direction,
    GError **error);
void example_callable_media_stream_accept_proposed_direction (
    ExampleCallableMediaStream *self);
void example_callable_media_stream_connect (ExampleCallableMediaStream *self);

/* This controls receiving emulated network events, so it wouldn't exist in
 * a real connection manager */
void example_callable_media_stream_simulate_contact_agreed_to_send (
    ExampleCallableMediaStream *self);

void example_callable_media_stream_receive_direction_request (
    ExampleCallableMediaStream *self, TpMediaStreamDirection direction);

G_END_DECLS

#endif
