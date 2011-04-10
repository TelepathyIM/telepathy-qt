/*
 * bug16307-conn.h - header for a connection that reproduces the #15307 bug
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_BUG16307_CONN_H__
#define __TP_TESTS_BUG16307_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

#include "simple-conn.h"

G_BEGIN_DECLS

typedef struct _TpTestsBug16307Connection TpTestsBug16307Connection;
typedef struct _TpTestsBug16307ConnectionClass TpTestsBug16307ConnectionClass;
typedef struct _TpTestsBug16307ConnectionPrivate TpTestsBug16307ConnectionPrivate;

struct _TpTestsBug16307ConnectionClass {
    TpTestsSimpleConnectionClass parent_class;
};

struct _TpTestsBug16307Connection {
    TpTestsSimpleConnection parent;

    TpTestsBug16307ConnectionPrivate *priv;
};

GType tp_tests_bug16307_connection_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_BUG16307_CONNECTION \
  (tp_tests_bug16307_connection_get_type ())
#define TP_TESTS_BUG16307_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_BUG16307_CONNECTION, \
                              TpTestsBug16307Connection))
#define TP_TESTS_BUG16307_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_BUG16307_CONNECTION, \
                           TpTestsBug16307ConnectionClass))
#define TP_TESTS_BUG16307_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_BUG16307_CONNECTION))
#define TP_TESTS_BUG16307_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_BUG16307_CONNECTION))
#define TP_TESTS_BUG16307_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_BUG16307_CONNECTION, \
                              TpTestsBug16307ConnectionClass))

/* Cause "network events", for debugging/testing */

void tp_tests_bug16307_connection_inject_get_status_return (TpTestsBug16307Connection *self);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_BUG16307_CONN_H__ */
