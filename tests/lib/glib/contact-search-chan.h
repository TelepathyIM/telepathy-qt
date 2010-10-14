/*
 * contact-search-channel.h - header for an tp_tests contact search channel
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

#ifndef TP_TESTS_CONTACT_SEARCH_CHANNEL_H
#define TP_TESTS_CONTACT_SEARCH_CHANNEL_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _TpTestsContactSearchChannel TpTestsContactSearchChannel;
typedef struct _TpTestsContactSearchChannelPrivate TpTestsContactSearchChannelPrivate;

typedef struct _TpTestsContactSearchChannelClass TpTestsContactSearchChannelClass;
typedef struct _TpTestsContactSearchChannelClassPrivate TpTestsContactSearchChannelClassPrivate;

GType tp_tests_contact_search_channel_get_type (void);

#define TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL \
  (tp_tests_contact_search_channel_get_type ())
#define TP_TESTS_CONTACT_SEARCH_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL, \
                               TpTestsContactSearchChannel))
#define TP_TESTS_CONTACT_SEARCH_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL, \
                            TpTestsContactSearchChannelClass))
#define TP_TESTS_IS_CONTACT_SEARCH_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL))
#define TP_TESTS_IS_CONTACT_SEARCH_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL))
#define TP_TESTS_CONTACT_SEARCH_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_CONTACT_SEARCH_CHANNEL, \
                              TpTestsContactSearchChannelClass))

struct _TpTestsContactSearchChannelClass {
    GObjectClass parent_class;

    TpDBusPropertiesMixinClass dbus_properties_class;
    TpGroupMixinClass group_class;

    TpTestsContactSearchChannelClassPrivate *priv;
};

struct _TpTestsContactSearchChannel {
    GObject parent;

    TpGroupMixin group;

    TpTestsContactSearchChannelPrivate *priv;
};

G_END_DECLS

#endif
