/*
 * simple-conn.h - header for a simple connection
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __SIMPLE_CONN_H__
#define __SIMPLE_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

G_BEGIN_DECLS

typedef struct _SimpleConnection SimpleConnection;
typedef struct _SimpleConnectionClass SimpleConnectionClass;
typedef struct _SimpleConnectionPrivate SimpleConnectionPrivate;

struct _SimpleConnectionClass {
    TpBaseConnectionClass parent_class;
};

struct _SimpleConnection {
    TpBaseConnection parent;

    SimpleConnectionPrivate *priv;
};

GType simple_connection_get_type (void);

/* TYPE MACROS */
#define SIMPLE_TYPE_CONNECTION \
  (simple_connection_get_type ())
#define SIMPLE_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SIMPLE_TYPE_CONNECTION, \
                              SimpleConnection))
#define SIMPLE_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), SIMPLE_TYPE_CONNECTION, \
                           SimpleConnectionClass))
#define SIMPLE_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), SIMPLE_TYPE_CONNECTION))
#define SIMPLE_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), SIMPLE_TYPE_CONNECTION))
#define SIMPLE_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SIMPLE_TYPE_CONNECTION, \
                              SimpleConnectionClass))

/* Cause "network events", for debugging/testing */

void simple_connection_inject_disconnect (SimpleConnection *self);

G_END_DECLS

#endif /* #ifndef __SIMPLE_CONN_H__ */
