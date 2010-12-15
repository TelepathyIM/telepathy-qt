/*
 * Example channel manager for contact lists
 *
 * Copyright © 2007-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_CONTACT_LIST_H__
#define __EXAMPLE_CONTACT_LIST_H__

#include <glib-object.h>

#include <telepathy-glib/base-contact-list.h>
#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/handle.h>
#include <telepathy-glib/presence-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleContactList ExampleContactList;
typedef struct _ExampleContactListClass ExampleContactListClass;
typedef struct _ExampleContactListPrivate ExampleContactListPrivate;

struct _ExampleContactListClass {
    TpBaseContactListClass parent_class;
};

struct _ExampleContactList {
    TpBaseContactList parent;

    ExampleContactListPrivate *priv;
};

GType example_contact_list_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST \
  (example_contact_list_get_type ())
#define EXAMPLE_CONTACT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CONTACT_LIST, \
                              ExampleContactList))
#define EXAMPLE_CONTACT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CONTACT_LIST, \
                           ExampleContactListClass))
#define EXAMPLE_IS_CONTACT_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CONTACT_LIST))
#define EXAMPLE_IS_CONTACT_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CONTACT_LIST))
#define EXAMPLE_CONTACT_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_LIST, \
                              ExampleContactListClass))

/* this enum must be kept in sync with the array _statuses in
 * contact-list.c */
typedef enum {
    EXAMPLE_CONTACT_LIST_PRESENCE_OFFLINE = 0,
    EXAMPLE_CONTACT_LIST_PRESENCE_UNKNOWN,
    EXAMPLE_CONTACT_LIST_PRESENCE_ERROR,
    EXAMPLE_CONTACT_LIST_PRESENCE_AWAY,
    EXAMPLE_CONTACT_LIST_PRESENCE_AVAILABLE
} ExampleContactListPresence;

const TpPresenceStatusSpec *example_contact_list_presence_statuses (
    void);

ExampleContactListPresence example_contact_list_get_presence (
    ExampleContactList *self, TpHandle contact);
const gchar *example_contact_list_get_alias (
    ExampleContactList *self, TpHandle contact);
void example_contact_list_set_alias (
    ExampleContactList *self, TpHandle contact, const gchar *alias);

G_END_DECLS

#endif
