/*
 * Example channel manager for contact lists
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_CONTACT_LIST_MANAGER_H__
#define __EXAMPLE_CONTACT_LIST_MANAGER_H__

#include <glib-object.h>

#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/handle.h>
#include <telepathy-glib/presence-mixin.h>

G_BEGIN_DECLS

typedef struct _ExampleContactListManager ExampleContactListManager;
typedef struct _ExampleContactListManagerClass ExampleContactListManagerClass;
typedef struct _ExampleContactListManagerPrivate ExampleContactListManagerPrivate;

struct _ExampleContactListManagerClass {
    GObjectClass parent_class;
};

struct _ExampleContactListManager {
    GObject parent;

    ExampleContactListManagerPrivate *priv;
};

GType example_contact_list_manager_get_type (void);

#define EXAMPLE_TYPE_CONTACT_LIST_MANAGER \
  (example_contact_list_manager_get_type ())
#define EXAMPLE_CONTACT_LIST_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CONTACT_LIST_MANAGER, \
                              ExampleContactListManager))
#define EXAMPLE_CONTACT_LIST_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CONTACT_LIST_MANAGER, \
                           ExampleContactListManagerClass))
#define EXAMPLE_IS_CONTACT_LIST_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CONTACT_LIST_MANAGER))
#define EXAMPLE_IS_CONTACT_LIST_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CONTACT_LIST_MANAGER))
#define EXAMPLE_CONTACT_LIST_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CONTACT_LIST_MANAGER, \
                              ExampleContactListManagerClass))

gboolean example_contact_list_manager_add_to_group (
    ExampleContactListManager *self, GObject *channel,
    TpHandle group, TpHandle member, const gchar *message, GError **error);

gboolean example_contact_list_manager_remove_from_group (
    ExampleContactListManager *self, GObject *channel,
    TpHandle group, TpHandle member, const gchar *message, GError **error);

/* elements 1, 2... of this enum must be kept in sync with elements 0, 1...
 * of the array _contact_lists in contact-list-manager.h */
typedef enum {
    INVALID_EXAMPLE_CONTACT_LIST,
    EXAMPLE_CONTACT_LIST_SUBSCRIBE = 1,
    EXAMPLE_CONTACT_LIST_PUBLISH,
    EXAMPLE_CONTACT_LIST_STORED,
    EXAMPLE_CONTACT_LIST_DENY,
    NUM_EXAMPLE_CONTACT_LISTS
} ExampleContactListHandle;

/* this enum must be kept in sync with the array _statuses in
 * contact-list-manager.c */
typedef enum {
    EXAMPLE_CONTACT_LIST_PRESENCE_OFFLINE = 0,
    EXAMPLE_CONTACT_LIST_PRESENCE_UNKNOWN,
    EXAMPLE_CONTACT_LIST_PRESENCE_ERROR,
    EXAMPLE_CONTACT_LIST_PRESENCE_AWAY,
    EXAMPLE_CONTACT_LIST_PRESENCE_AVAILABLE
} ExampleContactListPresence;

const TpPresenceStatusSpec *example_contact_list_presence_statuses (
    void);

gboolean example_contact_list_manager_add_to_list (
    ExampleContactListManager *self, GObject *channel,
    ExampleContactListHandle list, TpHandle member, const gchar *message,
    GError **error);

gboolean example_contact_list_manager_remove_from_list (
    ExampleContactListManager *self, GObject *channel,
    ExampleContactListHandle list, TpHandle member, const gchar *message,
    GError **error);

const gchar **example_contact_lists (void);

ExampleContactListPresence example_contact_list_manager_get_presence (
    ExampleContactListManager *self, TpHandle contact);
const gchar *example_contact_list_manager_get_alias (
    ExampleContactListManager *self, TpHandle contact);
void example_contact_list_manager_set_alias (
    ExampleContactListManager *self, TpHandle contact, const gchar *alias);

G_END_DECLS

#endif
