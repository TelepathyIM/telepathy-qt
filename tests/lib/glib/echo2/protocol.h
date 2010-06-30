/*
 * protocol.h - header for an example Protocol
 * Copyright (C) 2007-2010 Collabora Ltd.
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_ECHO_MESSAGE_PARTS_PROTOCOL_H
#define EXAMPLE_ECHO_MESSAGE_PARTS_PROTOCOL_H

#include <glib-object.h>
#include <telepathy-glib/base-protocol.h>

G_BEGIN_DECLS

typedef struct _ExampleEcho2Protocol
    ExampleEcho2Protocol;
typedef struct _ExampleEcho2ProtocolPrivate
    ExampleEcho2ProtocolPrivate;
typedef struct _ExampleEcho2ProtocolClass
    ExampleEcho2ProtocolClass;
typedef struct _ExampleEcho2ProtocolClassPrivate
    ExampleEcho2ProtocolClassPrivate;

struct _ExampleEcho2ProtocolClass {
    TpBaseProtocolClass parent_class;

    ExampleEcho2ProtocolClassPrivate *priv;
};

struct _ExampleEcho2Protocol {
    TpBaseProtocol parent;

    ExampleEcho2ProtocolPrivate *priv;
};

GType example_echo_2_protocol_get_type (void);

#define EXAMPLE_TYPE_ECHO_2_PROTOCOL \
    (example_echo_2_protocol_get_type ())
#define EXAMPLE_ECHO_2_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        EXAMPLE_TYPE_ECHO_2_PROTOCOL, \
        ExampleEcho2Protocol))
#define EXAMPLE_ECHO_2_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), \
        EXAMPLE_TYPE_ECHO_2_PROTOCOL, \
        ExampleEcho2ProtocolClass))
#define EXAMPLE_IS_ECHO_2_PROTOCOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        EXAMPLE_TYPE_ECHO_2_PROTOCOL))
#define EXAMPLE_IS_ECHO_2_PROTOCOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        EXAMPLE_TYPE_ECHO_2_PROTOCOL))
#define EXAMPLE_ECHO_2_PROTOCOL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        EXAMPLE_TYPE_ECHO_2_PROTOCOL, \
        ExampleEcho2ProtocolClass))

gchar *example_echo_2_protocol_normalize_contact (const gchar *id,
    GError **error);

G_END_DECLS

#endif
