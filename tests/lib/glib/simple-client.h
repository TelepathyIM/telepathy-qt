/*
 * simple-client.h - header for a simple client
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_SIMPLE_CLIENT_H__
#define __TP_TESTS_SIMPLE_CLIENT_H__

#include <glib-object.h>
#include <telepathy-glib/base-client.h>

G_BEGIN_DECLS

typedef struct _TpTestsSimpleClient TpTestsSimpleClient;
typedef struct _TpTestsSimpleClientClass TpTestsSimpleClientClass;

struct _TpTestsSimpleClientClass {
    TpBaseClientClass parent_class;
};

struct _TpTestsSimpleClient {
    TpBaseClient parent;

    TpObserveChannelsContext *observe_ctx;
    TpAddDispatchOperationContext *add_dispatch_ctx;
    TpHandleChannelsContext *handle_channels_ctx;
};

GType tp_tests_simple_client_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_CLIENT \
  (tp_tests_simple_client_get_type ())
#define TP_TESTS_SIMPLE_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_CLIENT, \
                              TpTestsSimpleClient))
#define TP_TESTS_SIMPLE_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_CLIENT, \
                           TpTestsSimpleClientClass))
#define SIMPLE_IS_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_CLIENT))
#define SIMPLE_IS_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_CLIENT))
#define TP_TESTS_SIMPLE_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_CLIENT, \
                              TpTestsSimpleClientClass))

TpTestsSimpleClient * tp_tests_simple_client_new (TpDBusDaemon *dbus_daemon,
    const gchar *name,
    gboolean uniquify_name);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_CONN_H__ */
