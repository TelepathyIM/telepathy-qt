/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_TESTS_CONTACTS_NOROSTER_CONN_H__
#define __TP_TESTS_CONTACTS_NOROSTER_CONN_H__

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>

#include "simple-conn.h"

G_BEGIN_DECLS

typedef struct _TpTestsContactsNorosterConnection TpTestsContactsNorosterConnection;
typedef struct _TpTestsContactsNorosterConnectionClass TpTestsContactsNorosterConnectionClass;

struct _TpTestsContactsNorosterConnectionClass {
    TpTestsSimpleConnectionClass parent_class;
};

struct _TpTestsContactsNorosterConnection {
    TpTestsSimpleConnection parent;
};

GType tp_tests_contacts_noroster_connection_get_type (void);

/* TYPE MACROS */
#define TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION \
  (tp_tests_contacts_noroster_connection_get_type ())
#define TP_TESTS_CONTACTS_NOROSTER_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION, \
                              TpTestsContactsNorosterConnection))
#define TP_TESTS_CONTACTS_NOROSTER_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION, \
                           TpTestsContactsNorosterConnectionClass))
#define TP_TESTS_CONTACTS_NOROSTER_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION))
#define TP_TESTS_CONTACTS_NOROSTER_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION))
#define TP_TESTS_CONTACTS_NOROSTER_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_CONTACTS_NOROSTER_CONNECTION, \
                              TpTestsContactsNorosterConnectionClass))

G_END_DECLS

#endif /* #ifndef __TP_TESTS_CONTACTS_NOROSTER_CONN_H__ */
