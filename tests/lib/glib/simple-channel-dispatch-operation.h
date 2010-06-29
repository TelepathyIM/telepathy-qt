/*
 * simple-channel-dispatch-operation.h - a simple channel dispatch operation
 * service.
 *
 * Copyright © 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION_H__
#define __TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION_H__

#include <glib-object.h>

#include <telepathy-glib/channel.h>
#include <telepathy-glib/dbus-properties-mixin.h>

G_BEGIN_DECLS

typedef struct _SimpleChannelDispatchOperation TpTestsSimpleChannelDispatchOperation;
typedef struct _SimpleChannelDispatchOperationClass TpTestsSimpleChannelDispatchOperationClass;
typedef struct _SimpleChannelDispatchOperationPrivate TpTestsSimpleChannelDispatchOperationPrivate;

struct _SimpleChannelDispatchOperationClass {
    GObjectClass parent_class;
    TpDBusPropertiesMixinClass dbus_props_class;
};

struct _SimpleChannelDispatchOperation {
    GObject parent;

    TpTestsSimpleChannelDispatchOperationPrivate *priv;
};

GType tp_tests_simple_channel_dispatch_operation_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION \
  (tp_tests_simple_channel_dispatch_operation_get_type ())
#define TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION, \
                              TpTestsSimpleChannelDispatchOperation))
#define TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION, \
                           TpTestsSimpleChannelDispatchOperationClass))
#define SIMPLE_IS_CHANNEL_DISPATCH_OPERATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION))
#define SIMPLE_IS_CHANNEL_DISPATCH_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION))
#define TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_CHANNEL_DISPATCH_OPERATION, \
                              TpTestsSimpleChannelDispatchOperationClass))

void tp_tests_simple_channel_dispatch_operation_set_conn_path (
    TpTestsSimpleChannelDispatchOperation *self,
    const gchar *conn_path);

void tp_tests_simple_channel_dispatch_operation_add_channel (
    TpTestsSimpleChannelDispatchOperation *self,
    TpChannel *chan);

void tp_tests_simple_channel_dispatch_operation_lost_channel (
    TpTestsSimpleChannelDispatchOperation *self,
    TpChannel *chan);

void tp_tests_simple_channel_dispatch_operation_set_account_path (
    TpTestsSimpleChannelDispatchOperation *self,
    const gchar *account_path);

G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_CHANNEL_DISPATCH_OPERATION_H__ */
