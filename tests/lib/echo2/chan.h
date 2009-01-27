/*
 * chan.h - header for an example channel
 *
 * Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_ECHO_MESSAGE_PARTS_CHAN_H
#define EXAMPLE_ECHO_MESSAGE_PARTS_CHAN_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/message-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleEcho2Channel ExampleEcho2Channel;
typedef struct _ExampleEcho2ChannelClass ExampleEcho2ChannelClass;
typedef struct _ExampleEcho2ChannelPrivate ExampleEcho2ChannelPrivate;

GType example_echo_2_channel_get_type (void);

#define EXAMPLE_TYPE_ECHO_2_CHANNEL \
  (example_echo_2_channel_get_type ())
#define EXAMPLE_ECHO_2_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_ECHO_2_CHANNEL, \
                               ExampleEcho2Channel))
#define EXAMPLE_ECHO_2_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_ECHO_2_CHANNEL, \
                            ExampleEcho2ChannelClass))
#define EXAMPLE_IS_ECHO_2_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_ECHO_2_CHANNEL))
#define EXAMPLE_IS_ECHO_2_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_ECHO_2_CHANNEL))
#define EXAMPLE_ECHO_2_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_2_CHANNEL, \
                              ExampleEcho2ChannelClass))

struct _ExampleEcho2ChannelClass {
    GObjectClass parent_class;

    TpDBusPropertiesMixinClass dbus_properties_class;
};

struct _ExampleEcho2Channel {
    GObject parent;
    TpMessageMixin text;

    ExampleEcho2ChannelPrivate *priv;
};

G_END_DECLS

#endif
