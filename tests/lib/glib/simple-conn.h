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

#ifndef __TP_TESTS_SIMPLE_CONN_H__
#define __TP_TESTS_SIMPLE_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

G_BEGIN_DECLS

typedef struct _TpTestsSimpleConnection TpTestsSimpleConnection;
typedef struct _TpTestsSimpleConnectionClass TpTestsSimpleConnectionClass;
typedef struct _TpTestsSimpleConnectionPrivate TpTestsSimpleConnectionPrivate;

struct _TpTestsSimpleConnectionClass {
    TpBaseConnectionClass parent_class;
};

struct _TpTestsSimpleConnection {
    TpBaseConnection parent;

    TpTestsSimpleConnectionPrivate *priv;
};

GType tp_tests_simple_connection_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_CONNECTION \
  (tp_tests_simple_connection_get_type ())
#define TP_TESTS_SIMPLE_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION, \
                              TpTestsSimpleConnection))
#define TP_TESTS_SIMPLE_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_CONNECTION, \
                           TpTestsSimpleConnectionClass))
#define TP_TESTS_SIMPLE_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION))
#define TP_TESTS_SIMPLE_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_CONNECTION))
#define TP_TESTS_SIMPLE_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_CONNECTION, \
                              TpTestsSimpleConnectionClass))

TpTestsSimpleConnection * tp_tests_simple_connection_new (const gchar *account,
    const gchar *protocol);

/* Cause "network events", for debugging/testing */

void tp_tests_simple_connection_inject_disconnect (
    TpTestsSimpleConnection *self);

void tp_tests_simple_connection_set_identifier (TpTestsSimpleConnection *self,
    const gchar *identifier);

gchar * tp_tests_simple_connection_ensure_text_chan (
    TpTestsSimpleConnection *self,
    const gchar *target_id,
    GHashTable **props);

void tp_tests_simple_connection_set_get_self_handle_error (
    TpTestsSimpleConnection *self,
    GQuark domain,
    gint code,
    const gchar *message);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_CONN_H__ */
