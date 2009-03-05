/*
 * Example ContactList channels with handle type LIST or GROUP
 *
 * Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_CONTACT_LIST_H
#define EXAMPLE_CONTACT_LIST_H

#include <glib-object.h>

#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/group-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleContactListBase ExampleContactListBase;
typedef struct _ExampleContactListBaseClass ExampleContactListBaseClass;
typedef struct _ExampleContactListBasePrivate ExampleContactListBasePrivate;

typedef struct _ExampleContactList ExampleContactList;
typedef struct _ExampleContactListClass ExampleContactListClass;
typedef struct _ExampleContactListPrivate ExampleContactListPrivate;

typedef struct _ExampleContactGroup ExampleContactGroup;
typedef struct _ExampleContactGroupClass ExampleContactGroupClass;
typedef struct _ExampleContactGroupPrivate ExampleContactGroupPrivate;

GType example_contact_list_base_get_type (void);
GType example_contact_list_get_type (void);
GType example_contact_group_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST_BASE \
  (example_contact_list_base_get_type ())
#define EXAMPLE_CONTACT_LIST_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CONTACT_LIST_BASE, \
                               ExampleContactListBase))
#define EXAMPLE_CONTACT_LIST_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CONTACT_LIST_BASE, \
                            ExampleContactListBaseClass))
#define EXAMPLE_IS_CONTACT_LIST_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CONTACT_LIST_BASE))
#define EXAMPLE_IS_CONTACT_LIST_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CONTACT_LIST_BASE))
#define EXAMPLE_CONTACT_LIST_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_LIST_BASE, \
                              ExampleContactListBaseClass))

#define EXAMPLE_TYPE_CONTACT_LIST \
  (example_contact_list_get_type ())
#define EXAMPLE_CONTACT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CONTACT_LIST, \
                               ExampleContactList))
#define EXAMPLE_CONTACT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CONTACT_LIST, \
                            ExampleContactListClass))
#define EXAMPLE_IS_CONTACT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CONTACT_LIST))
#define EXAMPLE_IS_CONTACT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CONTACT_LIST))
#define EXAMPLE_CONTACT_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_LIST, \
                              ExampleContactListClass))

#define EXAMPLE_TYPE_CONTACT_GROUP \
  (example_contact_group_get_type ())
#define EXAMPLE_CONTACT_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_TYPE_CONTACT_GROUP, \
                               ExampleContactGroup))
#define EXAMPLE_CONTACT_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_TYPE_CONTACT_GROUP, \
                            ExampleContactGroupClass))
#define EXAMPLE_IS_CONTACT_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_TYPE_CONTACT_GROUP))
#define EXAMPLE_IS_CONTACT_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_TYPE_CONTACT_GROUP))
#define EXAMPLE_CONTACT_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_GROUP, \
                              ExampleContactGroupClass))

struct _ExampleContactListBaseClass {
    GObjectClass parent_class;
    TpGroupMixinClass group_class;
    TpDBusPropertiesMixinClass dbus_properties_class;
};

struct _ExampleContactListClass {
    ExampleContactListBaseClass parent_class;
};

struct _ExampleContactGroupClass {
    ExampleContactListBaseClass parent_class;
};

struct _ExampleContactListBase {
    GObject parent;
    TpGroupMixin group;
    ExampleContactListBasePrivate *priv;
};

struct _ExampleContactList {
    ExampleContactListBase parent;
    ExampleContactListPrivate *priv;
};

struct _ExampleContactGroup {
    ExampleContactListBase parent;
    ExampleContactGroupPrivate *priv;
};

G_END_DECLS

#endif
