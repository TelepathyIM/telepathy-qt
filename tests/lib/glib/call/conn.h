/*
 * conn.h - header for an example connection
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_CALL_CONN_H
#define EXAMPLE_CALL_CONN_H

#include <glib-object.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/contacts-mixin.h>
#include <telepathy-glib/presence-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleCallConnection ExampleCallConnection;
typedef struct _ExampleCallConnectionPrivate
    ExampleCallConnectionPrivate;

typedef struct _ExampleCallConnectionClass ExampleCallConnectionClass;
typedef struct _ExampleCallConnectionClassPrivate
    ExampleCallConnectionClassPrivate;

struct _ExampleCallConnectionClass {
    TpBaseConnectionClass parent_class;
    TpPresenceMixinClass presence_mixin;
    TpContactsMixinClass contacts_mixin;

    ExampleCallConnectionClassPrivate *priv;
};

struct _ExampleCallConnection {
    TpBaseConnection parent;
    TpPresenceMixin presence_mixin;
    TpContactsMixin contacts_mixin;

    ExampleCallConnectionPrivate *priv;
};

GType example_call_connection_get_type (void);

#define EXAMPLE_TYPE_CALL_CONNECTION \
  (example_call_connection_get_type ())
#define EXAMPLE_CALL_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CALL_CONNECTION, \
                              ExampleCallConnection))
#define EXAMPLE_CALL_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CALL_CONNECTION, \
                           ExampleCallConnectionClass))
#define EXAMPLE_IS_CALL_CONNECTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CALL_CONNECTION))
#define EXAMPLE_IS_CALL_CONNECTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CALL_CONNECTION))
#define EXAMPLE_CALL_CONNECTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALL_CONNECTION, \
                              ExampleCallConnectionClass))

/* Must be kept in sync with the array presence_statuses in conn.c */
typedef enum {
    EXAMPLE_CALL_PRESENCE_OFFLINE = 0,
    EXAMPLE_CALL_PRESENCE_UNKNOWN,
    EXAMPLE_CALL_PRESENCE_ERROR,
    EXAMPLE_CALL_PRESENCE_AWAY,
    EXAMPLE_CALL_PRESENCE_AVAILABLE
} ExampleCallPresence;

gchar *example_call_normalize_contact (TpHandleRepoIface *repo,
    const gchar *id, gpointer context, GError **error);

const gchar * const *example_call_connection_get_possible_interfaces (void);

G_END_DECLS

#endif
