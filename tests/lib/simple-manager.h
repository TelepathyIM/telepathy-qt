/*
 * simple-manager.h - header for a simple connection manager
 * Copyright (C) 2007-2008 Collabora Ltd.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __SIMPLE_CONNECTION_MANAGER_H__
#define __SIMPLE_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _SimpleConnectionManager SimpleConnectionManager;
typedef struct _SimpleConnectionManagerClass SimpleConnectionManagerClass;

struct _SimpleConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;

    gpointer priv;
};

struct _SimpleConnectionManager {
    TpBaseConnectionManager parent;

    gpointer priv;
};

GType simple_connection_manager_get_type (void);

/* TYPE MACROS */
#define SIMPLE_TYPE_CONNECTION_MANAGER \
  (simple_connection_manager_get_type ())
#define SIMPLE_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SIMPLE_TYPE_CONNECTION_MANAGER, \
                              simpleConnectionManager))
#define SIMPLE_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), SIMPLE_TYPE_CONNECTION_MANAGER, \
                           SimpleConnectionManagerClass))
#define SIMPLE_IS_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), SIMPLE_TYPE_CONNECTION_MANAGER))
#define SIMPLE_IS_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), SIMPLE_TYPE_CONNECTION_MANAGER))
#define SIMPLE_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SIMPLE_TYPE_CONNECTION_MANAGER, \
                              SimpleConnectionManagerClass))

G_END_DECLS

#endif /* #ifndef __SIMPLE_CONNECTION_MANAGER_H__*/
