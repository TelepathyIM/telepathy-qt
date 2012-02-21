/*
 * protocol.h - header for an example Protocol
 * Copyright © 2007-2010 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_CALL_PROTOCOL_H
#define EXAMPLE_CALL_PROTOCOL_H

#include <glib-object.h>
#include <telepathy-glib/base-protocol.h>

G_BEGIN_DECLS

typedef struct _ExampleCallProtocol ExampleCallProtocol;
typedef struct _ExampleCallProtocolPrivate ExampleCallProtocolPrivate;
typedef struct _ExampleCallProtocolClass ExampleCallProtocolClass;
typedef struct _ExampleCallProtocolClassPrivate ExampleCallProtocolClassPrivate;

struct _ExampleCallProtocolClass {
    TpBaseProtocolClass parent_class;

    ExampleCallProtocolClassPrivate *priv;
};

struct _ExampleCallProtocol {
    TpBaseProtocol parent;

    ExampleCallProtocolPrivate *priv;
};

GType example_call_protocol_get_type (void);

#define EXAMPLE_TYPE_CALL_PROTOCOL \
    (example_call_protocol_get_type ())
#define EXAMPLE_CALL_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        EXAMPLE_TYPE_CALL_PROTOCOL, \
        ExampleCallProtocol))
#define EXAMPLE_CALL_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
        EXAMPLE_TYPE_CALL_PROTOCOL, \
        ExampleCallProtocolClass))
#define EXAMPLE_IS_CALL_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        EXAMPLE_TYPE_CALL_PROTOCOL))
#define EXAMPLE_IS_CALL_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        EXAMPLE_TYPE_CALL_PROTOCOL))
#define EXAMPLE_CALL_PROTOCOL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        EXAMPLE_TYPE_CALL_PROTOCOL, \
        ExampleCallProtocolClass))

gboolean example_call_protocol_check_contact_id (const gchar *id,
    gchar **normal,
    GError **error);

G_END_DECLS

#endif
