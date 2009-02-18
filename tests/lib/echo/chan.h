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

#ifndef __EXAMPLE_CHAN_H__
#define __EXAMPLE_CHAN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/text-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleEchoChannel ExampleEchoChannel;
typedef struct _ExampleEchoChannelClass ExampleEchoChannelClass;
typedef struct _ExampleEchoChannelPrivate ExampleEchoChannelPrivate;

GType example_echo_channel_get_type (void);

#define EXAMPLE_TYPE_ECHO_CHANNEL \
  (example_echo_channel_get_type ())
#define EXAMPLE_ECHO_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_ECHO_CHANNEL, \
                               ExampleEchoChannel))
#define EXAMPLE_ECHO_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_ECHO_CHANNEL, \
                            ExampleEchoChannelClass))
#define EXAMPLE_IS_ECHO_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_ECHO_CHANNEL))
#define EXAMPLE_IS_ECHO_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_ECHO_CHANNEL))
#define EXAMPLE_ECHO_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_CHANNEL, \
                              ExampleEchoChannelClass))

struct _ExampleEchoChannelClass {
    GObjectClass parent_class;
    TpTextMixinClass text_class;
    TpDBusPropertiesMixinClass dbus_properties_class;
};

struct _ExampleEchoChannel {
    GObject parent;
    TpTextMixin text;

    ExampleEchoChannelPrivate *priv;
};

G_END_DECLS

#endif /* #ifndef __EXAMPLE_CHAN_H__ */
