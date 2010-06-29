/*
 * manager.h - header for an example connection manager
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_CSH_CONNECTION_MANAGER_H__
#define __EXAMPLE_CSH_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _ExampleCSHConnectionManager ExampleCSHConnectionManager;
typedef struct _ExampleCSHConnectionManagerPrivate
    ExampleCSHConnectionManagerPrivate;
typedef struct _ExampleCSHConnectionManagerClass
    ExampleCSHConnectionManagerClass;
typedef struct _ExampleCSHConnectionManagerClassPrivate
    ExampleCSHConnectionManagerClassPrivate;

struct _ExampleCSHConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;

    ExampleCSHConnectionManagerClassPrivate *priv;
};

struct _ExampleCSHConnectionManager {
    TpBaseConnectionManager parent;

    ExampleCSHConnectionManagerPrivate *priv;
};

GType example_csh_connection_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_CSH_CONNECTION_MANAGER \
  (example_csh_connection_manager_get_type ())
#define EXAMPLE_CSH_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CSH_CONNECTION_MANAGER, \
                              ExampleCSHConnectionManager))
#define EXAMPLE_CSH_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CSH_CONNECTION_MANAGER, \
                           ExampleCSHConnectionManagerClass))
#define EXAMPLE_IS_CSH_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CSH_CONNECTION_MANAGER))
#define EXAMPLE_IS_CSH_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CSH_CONNECTION_MANAGER))
#define EXAMPLE_CSH_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CSH_CONNECTION_MANAGER, \
                              ExampleCSHConnectionManagerClass))

G_END_DECLS

#endif
