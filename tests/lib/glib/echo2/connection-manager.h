/*
 * manager.h - header for an example connection manager
 * Copyright (C) 2007 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_ECHO_MESSAGE_PARTS_MANAGER_H
#define EXAMPLE_ECHO_MESSAGE_PARTS_MANAGER_H

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _ExampleEcho2ConnectionManager
    ExampleEcho2ConnectionManager;
typedef struct _ExampleEcho2ConnectionManagerPrivate
    ExampleEcho2ConnectionManagerPrivate;
typedef struct _ExampleEcho2ConnectionManagerClass
    ExampleEcho2ConnectionManagerClass;
typedef struct _ExampleEcho2ConnectionManagerClassPrivate
    ExampleEcho2ConnectionManagerClassPrivate;

struct _ExampleEcho2ConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;

    ExampleEcho2ConnectionManagerClassPrivate *priv;
};

struct _ExampleEcho2ConnectionManager {
    TpBaseConnectionManager parent;

    ExampleEcho2ConnectionManagerPrivate *priv;
};

GType example_echo_2_connection_manager_get_type (void);

#define EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER \
    (example_echo_2_connection_manager_get_type ())
#define EXAMPLE_ECHO_2_CONNECTION_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER, \
        ExampleEcho2ConnectionManager))
#define EXAMPLE_ECHO_2_CONNECTION_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER, \
        ExampleEcho2ConnectionManagerClass))
#define EXAMPLE_IS_ECHO_2_CONNECTION_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER))
#define EXAMPLE_IS_ECHO_2_CONNECTION_MANAGER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER))
#define EXAMPLE_ECHO_2_CONNECTION_MANAGER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        EXAMPLE_TYPE_ECHO_2_CONNECTION_MANAGER, \
        ExampleEcho2ConnectionManagerClass))

G_END_DECLS

#endif
