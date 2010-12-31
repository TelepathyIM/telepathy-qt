/*
 * protocol.h - header for an example Protocol
 * Copyright © 2007-2010 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_CONTACT_LIST_PROTOCOL_H
#define EXAMPLE_CONTACT_LIST_PROTOCOL_H

#include <glib-object.h>
#include <telepathy-glib/base-protocol.h>

G_BEGIN_DECLS

typedef struct _ExampleContactListProtocol
    ExampleContactListProtocol;
typedef struct _ExampleContactListProtocolPrivate
    ExampleContactListProtocolPrivate;
typedef struct _ExampleContactListProtocolClass
    ExampleContactListProtocolClass;
typedef struct _ExampleContactListProtocolClassPrivate
    ExampleContactListProtocolClassPrivate;

struct _ExampleContactListProtocolClass {
    TpBaseProtocolClass parent_class;

    ExampleContactListProtocolClassPrivate *priv;
};

struct _ExampleContactListProtocol {
    TpBaseProtocol parent;

    ExampleContactListProtocolPrivate *priv;
};

GType example_contact_list_protocol_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL \
    (example_contact_list_protocol_get_type ())
#define EXAMPLE_CONTACT_LIST_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL, \
        ExampleContactListProtocol))
#define EXAMPLE_CONTACT_LIST_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
        EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL, \
        ExampleContactListProtocolClass))
#define EXAMPLE_IS_CONTACT_LIST_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL))
#define EXAMPLE_IS_CONTACT_LIST_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL))
#define EXAMPLE_CONTACT_LIST_PROTOCOL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        EXAMPLE_TYPE_CONTACT_LIST_PROTOCOL, \
        ExampleContactListProtocolClass))

gboolean example_contact_list_protocol_check_contact_id (const gchar *id,
    gchar **normal,
    GError **error);

G_END_DECLS

#endif
