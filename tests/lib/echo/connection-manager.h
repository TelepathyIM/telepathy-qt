/*
 * manager.h - header for an example connection manager
 * Copyright (C) 2007 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_ECHO_CONNECTION_MANAGER_H__
#define __EXAMPLE_ECHO_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _ExampleEchoConnectionManager ExampleEchoConnectionManager;
typedef struct _ExampleEchoConnectionManagerClass
    ExampleEchoConnectionManagerClass;

struct _ExampleEchoConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;
};

struct _ExampleEchoConnectionManager {
    TpBaseConnectionManager parent;

    gpointer priv;
};

GType example_echo_connection_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER \
  (example_echo_connection_manager_get_type ())
#define EXAMPLE_ECHO_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER, \
                              ExampleEchoConnectionManager))
#define EXAMPLE_ECHO_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER, \
                           ExampleEchoConnectionManagerClass))
#define EXAMPLE_IS_ECHO_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER))
#define EXAMPLE_IS_ECHO_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER))
#define EXAMPLE_ECHO_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_CONNECTION_MANAGER, \
                              ExampleEchoConnectionManagerClass))

G_END_DECLS

#endif
