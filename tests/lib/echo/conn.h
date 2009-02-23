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

#ifndef __EXAMPLE_ECHO_CONN_H__
#define __EXAMPLE_ECHO_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

G_BEGIN_DECLS

typedef struct _ExampleEchoConnection ExampleEchoConnection;
typedef struct _ExampleEchoConnectionClass ExampleEchoConnectionClass;
typedef struct _ExampleEchoConnectionPrivate ExampleEchoConnectionPrivate;

struct _ExampleEchoConnectionClass {
    TpBaseConnectionClass parent_class;
};

struct _ExampleEchoConnection {
    TpBaseConnection parent;

    ExampleEchoConnectionPrivate *priv;
};

GType example_echo_connection_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_ECHO_CONNECTION \
  (example_echo_connection_get_type ())
#define EXAMPLE_ECHO_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_ECHO_CONNECTION, \
                              ExampleEchoConnection))
#define EXAMPLE_ECHO_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_ECHO_CONNECTION, \
                           ExampleEchoConnectionClass))
#define EXAMPLE_IS_ECHO_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_ECHO_CONNECTION))
#define EXAMPLE_IS_ECHO_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_ECHO_CONNECTION))
#define EXAMPLE_ECHO_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_CONNECTION, \
                              ExampleEchoConnectionClass))

G_END_DECLS

#endif
