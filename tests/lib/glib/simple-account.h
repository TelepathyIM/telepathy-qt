/*
 * simple-account.h - header for a simple account service.
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_SIMPLE_ACCOUNT_H__
#define __TP_TESTS_SIMPLE_ACCOUNT_H__

#include <glib-object.h>
#include <telepathy-glib/dbus-properties-mixin.h>


G_BEGIN_DECLS

typedef struct _TpTestsSimpleAccount TpTestsSimpleAccount;
typedef struct _TpTestsSimpleAccountClass TpTestsSimpleAccountClass;
typedef struct _TpTestsSimpleAccountPrivate TpTestsSimpleAccountPrivate;

struct _TpTestsSimpleAccountClass {
    GObjectClass parent_class;
    TpDBusPropertiesMixinClass dbus_props_class;
};

struct _TpTestsSimpleAccount {
    GObject parent;

    TpTestsSimpleAccountPrivate *priv;
};

GType tp_tests_simple_account_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_ACCOUNT \
  (tp_tests_simple_account_get_type ())
#define TP_TESTS_SIMPLE_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT, \
                              TpTestsSimpleAccount))
#define TP_TESTS_SIMPLE_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_ACCOUNT, \
                           TpTestsSimpleAccountClass))
#define TP_TESTS_SIMPLE_IS_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT))
#define TP_TESTS_SIMPLE_IS_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_ACCOUNT))
#define TP_TESTS_SIMPLE_ACCOUNT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT, \
                              TpTestsSimpleAccountClass))

G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_ACCOUNT_H__ */
