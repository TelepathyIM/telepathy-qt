/*
 * /dev/null as a text channel
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_TEXT_CHANNEL_NULL_H__
#define __TP_TESTS_TEXT_CHANNEL_NULL_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/text-mixin.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _TpTestsTextChannelNull TpTestsTextChannelNull;
typedef struct _TpTestsTextChannelNullClass TpTestsTextChannelNullClass;
typedef struct _TpTestsTextChannelNullPrivate TpTestsTextChannelNullPrivate;

GType tp_tests_text_channel_null_get_type (void);

#define TP_TESTS_TYPE_TEXT_CHANNEL_NULL \
  (tp_tests_text_channel_null_get_type ())
#define TP_TESTS_TEXT_CHANNEL_NULL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_NULL, \
                               TpTestsTextChannelNull))
#define TP_TESTS_TEXT_CHANNEL_NULL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_TEXT_CHANNEL_NULL, \
                            TpTestsTextChannelNullClass))
#define TP_TESTS_IS_TEXT_CHANNEL_NULL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_NULL))
#define TP_TESTS_IS_TEXT_CHANNEL_NULL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_TEXT_CHANNEL_NULL))
#define TP_TESTS_TEXT_CHANNEL_NULL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_TEXT_CHANNEL_NULL, \
                              TpTestsTextChannelNullClass))

struct _TpTestsTextChannelNullClass {
    GObjectClass parent_class;

    TpTextMixinClass text_class;
};

struct _TpTestsTextChannelNull {
    GObject parent;
    TpTextMixin text;

    guint get_handle_called;
    guint get_interfaces_called;
    guint get_channel_type_called;

    TpTestsTextChannelNullPrivate *priv;
};

/* Subclass with D-Bus properties */

typedef struct _TestPropsTextChannel TpTestsPropsTextChannel;
typedef struct _TestPropsTextChannelClass TpTestsPropsTextChannelClass;

struct _TestPropsTextChannel {
    TpTestsTextChannelNull parent;

    GHashTable *dbus_property_interfaces_retrieved;
};

struct _TestPropsTextChannelClass {
    TpTestsTextChannelNullClass parent;

    TpDBusPropertiesMixinClass dbus_properties_class;
};

GType tp_tests_props_text_channel_get_type (void);

#define TP_TESTS_TYPE_PROPS_TEXT_CHANNEL \
  (tp_tests_props_text_channel_get_type ())
#define TP_TESTS_PROPS_TEXT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_PROPS_TEXT_CHANNEL, \
                               TpTestsPropsTextChannel))
#define TP_TESTS_PROPS_TEXT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_PROPS_TEXT_CHANNEL, \
                            TpTestsPropsTextChannelClass))
#define TP_TESTS_IS_PROPS_TEXT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_PROPS_TEXT_CHANNEL))
#define TP_TESTS_IS_PROPS_TEXT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_PROPS_TEXT_CHANNEL))
#define TP_TESTS_PROPS_TEXT_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_PROPS_TEXT_CHANNEL, \
                              TpTestsPropsTextChannelClass))

/* Subclass with D-Bus properties and Group */

typedef struct _TestPropsGroupTextChannel TpTestsPropsGroupTextChannel;
typedef struct _TestPropsGroupTextChannelClass TpTestsPropsGroupTextChannelClass;

struct _TestPropsGroupTextChannel {
    TpTestsPropsTextChannel parent;

    TpGroupMixin group;
};

struct _TestPropsGroupTextChannelClass {
    TpTestsPropsTextChannelClass parent;

    TpGroupMixinClass group_class;
};

GType tp_tests_props_group_text_channel_get_type (void);

#define TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL \
  (tp_tests_props_group_text_channel_get_type ())
#define TP_TESTS_PROPS_GROUP_TEXT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL, \
                               TpTestsPropsGroupTextChannel))
#define TP_TESTS_PROPS_GROUP_TEXT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL, \
                            TpTestsPropsGroupTextChannelClass))
#define TP_TESTS_IS_PROPS_GROUP_TEXT_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL))
#define TP_TESTS_IS_PROPS_GROUP_TEXT_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL))
#define TP_TESTS_PROPS_GROUP_TEXT_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_PROPS_GROUP_TEXT_CHANNEL, \
                              TpTestsPropsGroupTextChannelClass))

void tp_tests_text_channel_null_close (TpTestsTextChannelNull *self);

GHashTable * tp_tests_text_channel_get_props (TpTestsTextChannelNull *self);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_TEXT_CHANNEL_NULL_H__ */
