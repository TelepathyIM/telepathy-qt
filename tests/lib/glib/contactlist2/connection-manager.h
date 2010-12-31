/*
 * manager.h - header for an example connection manager
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER_H__
#define __EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _ExampleContactListConnectionManager
    ExampleContactListConnectionManager;
typedef struct _ExampleContactListConnectionManagerClass
    ExampleContactListConnectionManagerClass;
typedef struct _ExampleContactListConnectionManagerPrivate
    ExampleContactListConnectionManagerPrivate;

struct _ExampleContactListConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;
};

struct _ExampleContactListConnectionManager {
    TpBaseConnectionManager parent;

    ExampleContactListConnectionManagerPrivate *priv;
};

GType example_contact_list_connection_manager_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER \
  (example_contact_list_connection_manager_get_type ())
#define EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                              EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER, \
                              ExampleContactListConnectionManager))
#define EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
                           EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER, \
                           ExampleContactListConnectionManagerClass))
#define EXAMPLE_IS_CONTACT_LIST_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                              EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER))
#define EXAMPLE_IS_CONTACT_LIST_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), \
                           EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER))
#define EXAMPLE_CONTACT_LIST_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              EXAMPLE_TYPE_CONTACT_LIST_CONNECTION_MANAGER, \
                              ExampleContactListConnectionManagerClass))

G_END_DECLS

#endif
