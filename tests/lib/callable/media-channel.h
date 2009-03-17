/*
 * media-channel.h - header for an example channel
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

#ifndef __EXAMPLE_CALLABLE_MEDIA_CHANNEL_H__
#define __EXAMPLE_CALLABLE_MEDIA_CHANNEL_H__

#include <glib-object.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleCallableMediaChannel ExampleCallableMediaChannel;
typedef struct _ExampleCallableMediaChannelPrivate
    ExampleCallableMediaChannelPrivate;

typedef struct _ExampleCallableMediaChannelClass
    ExampleCallableMediaChannelClass;
typedef struct _ExampleCallableMediaChannelClassPrivate
    ExampleCallableMediaChannelClassPrivate;

GType example_callable_media_channel_get_type (void);

#define EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL \
  (example_callable_media_channel_get_type ())
#define EXAMPLE_CALLABLE_MEDIA_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL, \
                               ExampleCallableMediaChannel))
#define EXAMPLE_CALLABLE_MEDIA_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL, \
                            ExampleCallableMediaChannelClass))
#define EXAMPLE_IS_CALLABLE_MEDIA_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL))
#define EXAMPLE_IS_CALLABLE_MEDIA_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL))
#define EXAMPLE_CALLABLE_MEDIA_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL, \
                              ExampleCallableMediaChannelClass))

struct _ExampleCallableMediaChannelClass {
    GObjectClass parent_class;
    TpGroupMixinClass group_class;
    TpDBusPropertiesMixinClass dbus_properties_class;

    ExampleCallableMediaChannelClassPrivate *priv;
};

struct _ExampleCallableMediaChannel {
    GObject parent;
    TpGroupMixin group;

    ExampleCallableMediaChannelPrivate *priv;
};

G_END_DECLS

#endif
