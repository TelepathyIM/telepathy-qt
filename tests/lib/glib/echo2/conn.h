/*
 * conn.h - header for an example connection
 *
 * Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_ECHO_MESSAGE_PARTS_CONN_H
#define EXAMPLE_ECHO_MESSAGE_PARTS_CONN_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

G_BEGIN_DECLS

typedef struct _ExampleEcho2Connection ExampleEcho2Connection;
typedef struct _ExampleEcho2ConnectionClass ExampleEcho2ConnectionClass;
typedef struct _ExampleEcho2ConnectionPrivate ExampleEcho2ConnectionPrivate;

struct _ExampleEcho2ConnectionClass {
    TpBaseConnectionClass parent_class;
};

struct _ExampleEcho2Connection {
    TpBaseConnection parent;

    ExampleEcho2ConnectionPrivate *priv;
};

GType example_echo_2_connection_get_type (void);

#define EXAMPLE_TYPE_ECHO_2_CONNECTION \
  (example_echo_2_connection_get_type ())
#define EXAMPLE_ECHO_2_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_ECHO_2_CONNECTION, \
                              ExampleEcho2Connection))
#define EXAMPLE_ECHO_2_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_ECHO_2_CONNECTION, \
                           ExampleEcho2ConnectionClass))
#define EXAMPLE_IS_ECHO_2_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_ECHO_2_CONNECTION))
#define EXAMPLE_IS_ECHO_2_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_ECHO_2_CONNECTION))
#define EXAMPLE_ECHO_2_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_2_CONNECTION, \
                              ExampleEcho2ConnectionClass))

const gchar * const *example_echo_2_connection_get_guaranteed_interfaces (
    void);
const gchar * const *example_echo_2_connection_get_possible_interfaces (
    void);

G_END_DECLS

#endif
