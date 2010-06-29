/*
 * a stub anonymous MUC
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TEST_TEXT_CHANNEL_GROUP_H__
#define __TEST_TEXT_CHANNEL_GROUP_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>
#include <telepathy-glib/text-mixin.h>

G_BEGIN_DECLS

typedef struct _TpTestsTextChannelGroup TpTestsTextChannelGroup;
typedef struct _TpTestsTextChannelGroupClass TpTestsTextChannelGroupClass;
typedef struct _TpTestsTextChannelGroupPrivate TpTestsTextChannelGroupPrivate;

GType tp_tests_text_channel_group_get_type (void);

#define TP_TESTS_TYPE_TEXT_CHANNEL_GROUP \
  (tp_tests_text_channel_group_get_type ())
#define TP_TESTS_TEXT_CHANNEL_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_GROUP, \
                               TpTestsTextChannelGroup))
#define TP_TESTS_TEXT_CHANNEL_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_TEXT_CHANNEL_GROUP, \
                            TpTestsTextChannelGroupClass))
#define TEST_IS_TEXT_CHANNEL_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_GROUP))
#define TEST_IS_TEXT_CHANNEL_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_TEXT_CHANNEL_GROUP))
#define TP_TESTS_TEXT_CHANNEL_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_GROUP, \
                              TpTestsTextChannelGroupClass))

struct _TpTestsTextChannelGroupClass {
    GObjectClass parent_class;

    TpTextMixinClass text_class;
    TpGroupMixinClass group_class;
    TpDBusPropertiesMixinClass dbus_properties_class;
};

struct _TpTestsTextChannelGroup {
    GObject parent;

    TpBaseConnection *conn;

    TpTextMixin text;
    TpGroupMixin group;

    TpTestsTextChannelGroupPrivate *priv;
};

G_END_DECLS

#endif /* #ifndef __TEST_TEXT_CHANNEL_GROUP_H__ */
