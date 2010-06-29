/*
 * simple-manager.h - header for a simple connection manager
 * Copyright (C) 2007-2008 Collabora Ltd.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_SIMPLE_CONNECTION_MANAGER_H__
#define __TP_TESTS_SIMPLE_CONNECTION_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection-manager.h>

G_BEGIN_DECLS

typedef struct _TpTestsSimpleConnectionManager TpTestsSimpleConnectionManager;
typedef struct _TpTestsSimpleConnectionManagerPrivate TpTestsSimpleConnectionManagerPrivate;
typedef struct _TpTestsSimpleConnectionManagerClass TpTestsSimpleConnectionManagerClass;
typedef struct _TpTestsSimpleConnectionManagerClassPrivate
  TpTestsSimpleConnectionManagerClassPrivate;

struct _TpTestsSimpleConnectionManagerClass {
    TpBaseConnectionManagerClass parent_class;

    TpTestsSimpleConnectionManagerClassPrivate *priv;
};

struct _TpTestsSimpleConnectionManager {
    TpBaseConnectionManager parent;

    TpTestsSimpleConnectionManagerPrivate *priv;
};

GType tp_tests_simple_connection_manager_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER \
  (tp_tests_simple_connection_manager_get_type ())
#define TP_TESTS_SIMPLE_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER, \
                              simpleConnectionManager))
#define TP_TESTS_SIMPLE_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER, \
                           TpTestsSimpleConnectionManagerClass))
#define TP_TESTS_SIMPLE_IS_CONNECTION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER))
#define TP_TESTS_SIMPLE_IS_CONNECTION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER))
#define TP_TESTS_SIMPLE_CONNECTION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION_MANAGER, \
                              TpTestsSimpleConnectionManagerClass))

G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_CONNECTION_MANAGER_H__*/
