/*
 * room.h - header for an example chatroom channel
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_CSH_ROOM_H
#define EXAMPLE_CSH_ROOM_H

#include <glib-object.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>
#include <telepathy-glib/text-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleCSHRoomChannel ExampleCSHRoomChannel;
typedef struct _ExampleCSHRoomChannelClass ExampleCSHRoomChannelClass;
typedef struct _ExampleCSHRoomChannelPrivate ExampleCSHRoomChannelPrivate;

GType example_csh_room_channel_get_type (void);

#define EXAMPLE_TYPE_CSH_ROOM_CHANNEL \
  (example_csh_room_channel_get_type ())
#define EXAMPLE_CSH_ROOM_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CSH_ROOM_CHANNEL, \
                               ExampleCSHRoomChannel))
#define EXAMPLE_CSH_ROOM_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CSH_ROOM_CHANNEL, \
                            ExampleCSHRoomChannelClass))
#define EXAMPLE_IS_CSH_ROOM_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CSH_ROOM_CHANNEL))
#define EXAMPLE_IS_CSH_ROOM_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CSH_ROOM_CHANNEL))
#define EXAMPLE_CSH_ROOM_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CSH_ROOM_CHANNEL, \
                              ExampleCSHRoomChannelClass))

struct _ExampleCSHRoomChannelClass {
    GObjectClass parent_class;

    TpTextMixinClass text_class;
    TpGroupMixinClass group_class;
    TpDBusPropertiesMixinClass dbus_properties_class;
};

struct _ExampleCSHRoomChannel {
    GObject parent;

    TpTextMixin text;
    TpGroupMixin group;

    ExampleCSHRoomChannelPrivate *priv;
};

G_END_DECLS

#endif
