/*
 * contacts-conn.h - header for a connection with contact info
 *
 * Copyright (C) 2007-2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef TESTS_LIB_CONTACTS_CONN_H
#define TESTS_LIB_CONTACTS_CONN_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/contacts-mixin.h>
#include <telepathy-glib/presence-mixin.h>

#include "simple-conn.h"

G_BEGIN_DECLS

typedef struct _ContactsConnection ContactsConnection;
typedef struct _ContactsConnectionClass ContactsConnectionClass;
typedef struct _ContactsConnectionPrivate ContactsConnectionPrivate;

struct _ContactsConnectionClass {
    SimpleConnectionClass parent_class;

    TpPresenceMixinClass presence_mixin;
    TpContactsMixinClass contacts_mixin;
};

struct _ContactsConnection {
    SimpleConnection parent;

    TpPresenceMixin presence_mixin;
    TpContactsMixin contacts_mixin;

    ContactsConnectionPrivate *priv;
};

GType contacts_connection_get_type (void);

/* Must match my_statuses in the .c */
typedef enum {
    CONTACTS_CONNECTION_STATUS_AVAILABLE,
    CONTACTS_CONNECTION_STATUS_BUSY,
    CONTACTS_CONNECTION_STATUS_AWAY,
    CONTACTS_CONNECTION_STATUS_OFFLINE,
    CONTACTS_CONNECTION_STATUS_UNKNOWN,
    CONTACTS_CONNECTION_STATUS_ERROR
} ContactsConnectionPresenceStatusIndex;

/* TYPE MACROS */
#define CONTACTS_TYPE_CONNECTION \
  (contacts_connection_get_type ())
#define CONTACTS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), CONTACTS_TYPE_CONNECTION, \
                              ContactsConnection))
#define CONTACTS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), CONTACTS_TYPE_CONNECTION, \
                           ContactsConnectionClass))
#define CONTACTS_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), CONTACTS_TYPE_CONNECTION))
#define CONTACTS_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), CONTACTS_TYPE_CONNECTION))
#define CONTACTS_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CONTACTS_TYPE_CONNECTION, \
                              ContactsConnectionClass))

void contacts_connection_change_aliases (ContactsConnection *self, guint n,
    const TpHandle *handles, const gchar * const *aliases);

void contacts_connection_change_presences (ContactsConnection *self, guint n,
    const TpHandle *handles,
    const ContactsConnectionPresenceStatusIndex *indexes,
    const gchar * const *messages);

void contacts_connection_change_avatar_tokens (ContactsConnection *self,
    guint n, const TpHandle *handles, const gchar * const *tokens);

/* Legacy version (no Contacts interface) */

typedef struct _LegacyContactsConnection LegacyContactsConnection;
typedef struct _LegacyContactsConnectionClass LegacyContactsConnectionClass;
typedef struct _LegacyContactsConnectionPrivate
  LegacyContactsConnectionPrivate;

struct _LegacyContactsConnectionClass {
    ContactsConnectionClass parent_class;
};

struct _LegacyContactsConnection {
    ContactsConnection parent;

    LegacyContactsConnectionPrivate *priv;
};

GType legacy_contacts_connection_get_type (void);

/* TYPE MACROS */
#define LEGACY_CONTACTS_TYPE_CONNECTION \
  (legacy_contacts_connection_get_type ())
#define LEGACY_CONTACTS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), LEGACY_CONTACTS_TYPE_CONNECTION, \
                              LegacyContactsConnection))
#define LEGACY_CONTACTS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), LEGACY_CONTACTS_TYPE_CONNECTION, \
                           LegacyContactsConnectionClass))
#define LEGACY_CONTACTS_IS_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), LEGACY_CONTACTS_TYPE_CONNECTION))
#define LEGACY_CONTACTS_IS_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), LEGACY_CONTACTS_TYPE_CONNECTION))
#define LEGACY_CONTACTS_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LEGACY_CONTACTS_TYPE_CONNECTION, \
                              LegacyContactsConnectionClass))

G_END_DECLS

#endif
