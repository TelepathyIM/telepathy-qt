/*
 * simple-account-manager.h - header for a simple account manager service.
 *
 * Copyright (C) 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_SIMPLE_ACCOUNT_MANAGER_H__
#define __TP_TESTS_SIMPLE_ACCOUNT_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/dbus-properties-mixin.h>


G_BEGIN_DECLS

typedef struct _TpTestsSimpleAccountManager TpTestsSimpleAccountManager;
typedef struct _TpTestsSimpleAccountManagerClass TpTestsSimpleAccountManagerClass;
typedef struct _TpTestsSimpleAccountManagerPrivate TpTestsSimpleAccountManagerPrivate;

struct _TpTestsSimpleAccountManagerClass {
    GObjectClass parent_class;
    TpDBusPropertiesMixinClass dbus_props_class;
};

struct _TpTestsSimpleAccountManager {
    GObject parent;

    TpTestsSimpleAccountManagerPrivate *priv;
};

GType tp_tests_simple_account_manager_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER \
  (tp_tests_simple_account_manager_get_type ())
#define SIMPLE_ACCOUNT_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER, \
                              TpTestsSimpleAccountManager))
#define SIMPLE_ACCOUNT_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER, \
                           TpTestsSimpleAccountManagerClass))
#define SIMPLE_IS_ACCOUNT_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER))
#define SIMPLE_IS_ACCOUNT_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER))
#define SIMPLE_ACCOUNT_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_SIMPLE_ACCOUNT_MANAGER, \
                              TpTestsSimpleAccountManagerClass))


G_END_DECLS

#endif /* #ifndef __TP_TESTS_SIMPLE_ACCOUNT_MANAGER_H__ */
