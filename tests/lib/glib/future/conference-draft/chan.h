/*
 * conference-channel.h - header for an example conference channel
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2010 Nokia Corporation
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

#ifndef EXAMPLE_CONFERENCE_DRAFT_CHANNEL_H
#define EXAMPLE_CONFERENCE_DRAFT_CHANNEL_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleConferenceDRAFTChannel ExampleConferenceDRAFTChannel;
typedef struct _ExampleConferenceDRAFTChannelPrivate
    ExampleConferenceDRAFTChannelPrivate;

typedef struct _ExampleConferenceDRAFTChannelClass
    ExampleConferenceDRAFTChannelClass;
typedef struct _ExampleConferenceDRAFTChannelClassPrivate
    ExampleConferenceDRAFTChannelClassPrivate;

GType example_conference_draft_channel_get_type (void);

#define EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL \
  (example_conference_draft_channel_get_type ())
#define EXAMPLE_CONFERENCE_DRAFT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL, \
                               ExampleConferenceDRAFTChannel))
#define EXAMPLE_CONFERENCE_DRAFT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL, \
                            ExampleConferenceDRAFTChannelClass))
#define EXAMPLE_IS_CONFERENCE_DRAFT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL))
#define EXAMPLE_IS_CONFERENCE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL))
#define EXAMPLE_CONFERENCE_DRAFT_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONFERENCE_DRAFT_CHANNEL, \
                              ExampleConferenceDRAFTChannelClass))

struct _ExampleConferenceDRAFTChannelClass {
    GObjectClass parent_class;

    TpDBusPropertiesMixinClass dbus_properties_class;
    TpGroupMixinClass group_class;

    ExampleConferenceDRAFTChannelClassPrivate *priv;
};

struct _ExampleConferenceDRAFTChannel {
    GObject parent;

    TpGroupMixin group;

    ExampleConferenceDRAFTChannelPrivate *priv;
};

void example_conference_draft_channel_disconnected (ExampleConferenceDRAFTChannel *self);

G_END_DECLS

#endif
