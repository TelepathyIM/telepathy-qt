/*
 * conn.h - header for an example connection
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_CONTACT_LIST_CONN_H__
#define __EXAMPLE_CONTACT_LIST_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/contacts-mixin.h>
#include <telepathy-glib/presence-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleContactListConnection ExampleContactListConnection;
typedef struct _ExampleContactListConnectionClass
    ExampleContactListConnectionClass;
typedef struct _ExampleContactListConnectionPrivate
    ExampleContactListConnectionPrivate;

struct _ExampleContactListConnectionClass {
    TpBaseConnectionClass parent_class;
    TpPresenceMixinClass presence_mixin;
    TpContactsMixinClass contacts_mixin;
};

struct _ExampleContactListConnection {
    TpBaseConnection parent;
    TpPresenceMixin presence_mixin;
    TpContactsMixin contacts_mixin;

    ExampleContactListConnectionPrivate *priv;
};

GType example_contact_list_connection_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST_CONNECTION \
  (example_contact_list_connection_get_type ())
#define EXAMPLE_CONTACT_LIST_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CONTACT_LIST_CONNECTION, \
                              ExampleContactListConnection))
#define EXAMPLE_CONTACT_LIST_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CONTACT_LIST_CONNECTION, \
                           ExampleContactListConnectionClass))
#define EXAMPLE_IS_CONTACT_LIST_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CONTACT_LIST_CONNECTION))
#define EXAMPLE_IS_CONTACT_LIST_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CONTACT_LIST_CONNECTION))
#define EXAMPLE_CONTACT_LIST_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_LIST_CONNECTION, \
                              ExampleContactListConnectionClass))

gchar *example_contact_list_normalize_contact (TpHandleRepoIface *repo,
    const gchar *id, gpointer context, GError **error);

const gchar * const * example_contact_list_connection_get_possible_interfaces (
    void);

G_END_DECLS

#endif
