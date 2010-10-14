/*
 * conference-channel.h - header for an tp_tests conference channel
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

#ifndef TP_TESTS_CONFERENCE_CHANNEL_H
#define TP_TESTS_CONFERENCE_CHANNEL_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _TpTestsConferenceChannel TpTestsConferenceChannel;
typedef struct _TpTestsConferenceChannelPrivate
    TpTestsConferenceChannelPrivate;

typedef struct _TpTestsConferenceChannelClass
    TpTestsConferenceChannelClass;
typedef struct _TpTestsConferenceChannelClassPrivate
    TpTestsConferenceChannelClassPrivate;

GType tp_tests_conference_channel_get_type (void);

#define TP_TESTS_TYPE_CONFERENCE_CHANNEL \
  (tp_tests_conference_channel_get_type ())
#define TP_TESTS_CONFERENCE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_CONFERENCE_CHANNEL, \
                               TpTestsConferenceChannel))
#define TP_TESTS_CONFERENCE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_CONFERENCE_CHANNEL, \
                            TpTestsConferenceChannelClass))
#define TP_TESTS_IS_CONFERENCE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_CONFERENCE_CHANNEL))
#define TP_TESTS_IS_CONFERENCE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_CONFERENCE_CHANNEL))
#define TP_TESTS_CONFERENCE_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_CONFERENCE_CHANNEL, \
                              TpTestsConferenceChannelClass))

struct _TpTestsConferenceChannelClass {
    GObjectClass parent_class;

    TpDBusPropertiesMixinClass dbus_properties_class;
    TpGroupMixinClass group_class;

    TpTestsConferenceChannelClassPrivate *priv;
};

struct _TpTestsConferenceChannel {
    GObject parent;

    TpGroupMixin group;

    TpTestsConferenceChannelPrivate *priv;
};

void tp_tests_conference_channel_remove_channel (TpTestsConferenceChannel *self,
        const gchar *channel);

G_END_DECLS

#endif
