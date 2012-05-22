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

#include "contact-list-manager.h"

#include <string.h>

#include <dbus/dbus-glib.h>

#include <telepathy-glib/telepathy-glib.h>

#include "contact-list.h"

/* elements 0, 1... of this array must be kept in sync with elements 1, 2...
 * of the enum ExampleContactList in contact-list-manager.h */
static const gchar *_contact_lists[NUM_EXAMPLE_CONTACT_LISTS + 1] = {
    "subscribe",
    "publish",
    "stored",
    "deny",
    NULL
};

const gchar **
example_contact_lists (void)
{
  return _contact_lists;
}

/* this array must be kept in sync with the enum
 * ExampleContactListPresence in contact-list-manager.h */
static const TpPresenceStatusSpec _statuses[] = {
      { "offline", TP_CONNECTION_PRESENCE_TYPE_OFFLINE, FALSE, NULL },
      { "unknown", TP_CONNECTION_PRESENCE_TYPE_UNKNOWN, FALSE, NULL },
      { "error", TP_CONNECTION_PRESENCE_TYPE_ERROR, FALSE, NULL },
      { "away", TP_CONNECTION_PRESENCE_TYPE_AWAY, TRUE, NULL },
      { "available", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE, NULL },
      { NULL }
};

const TpPresenceStatusSpec *
example_contact_list_presence_statuses (void)
{
  return _statuses;
}

typedef struct {
    gchar *alias;

    guint subscribe:1;
    guint publish:1;
    guint subscribe_requested:1;
    guint publish_requested:1;
    guint stored:1;
    guint blocked:1;

    TpHandleSet *tags;

} ExampleContactDetails;

static ExampleContactDetails *
example_contact_details_new (void)
{
  return g_slice_new0 (ExampleContactDetails);
}

static void
example_contact_details_destroy (gpointer p)
{
  ExampleContactDetails *d = p;

  if (d->tags != NULL)
    tp_handle_set_destroy (d->tags);

  g_free (d->alias);
  g_slice_free (ExampleContactDetails, d);
}

static void channel_manager_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (ExampleContactListManager,
    example_contact_list_manager,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER,
      channel_manager_iface_init))

enum
{
  ALIAS_UPDATED,
  PRESENCE_UPDATED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

enum
{
  PROP_CONNECTION = 1,
  PROP_SIMULATION_DELAY,
  N_PROPS
};

struct _ExampleContactListManagerPrivate
{
  TpBaseConnection *conn;
  guint simulation_delay;
  TpHandleRepoIface *contact_repo;
  TpHandleRepoIface *group_repo;

  TpHandleSet *contacts;
  /* GUINT_TO_POINTER (handle borrowed from contacts)
   *    => ExampleContactDetails */
  GHashTable *contact_details;

  ExampleContactList *lists[NUM_EXAMPLE_CONTACT_LISTS];

  /* GUINT_TO_POINTER (handle borrowed from channel) => ExampleContactGroup */
  GHashTable *groups;

  /* borrowed TpExportableChannel => GSList of gpointer (request tokens) that
   * will be satisfied by that channel when the contact list has been
   * downloaded. The requests are in reverse chronological order */
  GHashTable *queued_requests;

  gulong status_changed_id;
};

static void
example_contact_list_manager_init (ExampleContactListManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EXAMPLE_TYPE_CONTACT_LIST_MANAGER, ExampleContactListManagerPrivate);

  self->priv->contact_details = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, example_contact_details_destroy);
  self->priv->groups = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, g_object_unref);
  self->priv->queued_requests = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, NULL);

  /* initialized properly in constructed() */
  self->priv->contact_repo = NULL;
  self->priv->group_repo = NULL;
  self->priv->contacts = NULL;
}

static void
example_contact_list_manager_close_all (ExampleContactListManager *self)
{
  guint i;

  if (self->priv->queued_requests != NULL)
    {
      GHashTable *tmp = self->priv->queued_requests;
      GHashTableIter iter;
      gpointer key, value;

      self->priv->queued_requests = NULL;
      g_hash_table_iter_init (&iter, tmp);

      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          GSList *requests = value;
          GSList *l;

          requests = g_slist_reverse (requests);

          for (l = requests; l != NULL; l = l->next)
            {
              tp_channel_manager_emit_request_failed (self,
                  l->data, TP_ERROR, TP_ERROR_DISCONNECTED,
                  "Unable to complete channel request due to disconnection");
            }

          g_slist_free (requests);
          g_hash_table_iter_steal (&iter);
        }

      g_hash_table_destroy (tmp);
    }

  if (self->priv->contacts != NULL)
    {
      tp_handle_set_destroy (self->priv->contacts);
      self->priv->contacts = NULL;
    }

  if (self->priv->contact_details != NULL)
    {
      GHashTable *tmp = self->priv->contact_details;

      self->priv->contact_details = NULL;
      g_hash_table_destroy (tmp);
    }

  if (self->priv->groups != NULL)
    {
      GHashTable *tmp = self->priv->groups;

      self->priv->groups = NULL;
      g_hash_table_destroy (tmp);
    }

  for (i = 0; i < NUM_EXAMPLE_CONTACT_LISTS; i++)
    {
      if (self->priv->lists[i] != NULL)
        {
          ExampleContactList *list = self->priv->lists[i];

          /* set self->priv->lists[i] to NULL here so list_closed_cb does
           * not try to delete the list again */
          self->priv->lists[i] = NULL;
          g_object_unref (list);
        }
    }

  if (self->priv->status_changed_id != 0)
    {
      g_signal_handler_disconnect (self->priv->conn,
          self->priv->status_changed_id);
      self->priv->status_changed_id = 0;
    }
}

static void
dispose (GObject *object)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (object);

  example_contact_list_manager_close_all (self);
  g_assert (self->priv->groups == NULL);
  g_assert (self->priv->lists[0] == NULL);
  g_assert (self->priv->queued_requests == NULL);

  ((GObjectClass *) example_contact_list_manager_parent_class)->dispose (
    object);
}

static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (object);

  switch (property_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, self->priv->conn);
      break;

    case PROP_SIMULATION_DELAY:
      g_value_set_uint (value, self->priv->simulation_delay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
set_property (GObject *object,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (object);

  switch (property_id)
    {
    case PROP_CONNECTION:
      /* We don't ref the connection, because it owns a reference to the
       * manager, and it guarantees that the manager's lifetime is
       * less than its lifetime */
      self->priv->conn = g_value_get_object (value);
      break;

    case PROP_SIMULATION_DELAY:
      self->priv->simulation_delay = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
satisfy_queued_requests (TpExportableChannel *channel,
                         gpointer user_data)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (user_data);
  GSList *requests = g_hash_table_lookup (self->priv->queued_requests,
      channel);

  /* this is all fine even if requests is NULL */
  g_hash_table_steal (self->priv->queued_requests, channel);
  requests = g_slist_reverse (requests);
  tp_channel_manager_emit_new_channel (self, channel, requests);
  g_slist_free (requests);
}

static ExampleContactDetails *
lookup_contact (ExampleContactListManager *self,
                TpHandle contact)
{
  return g_hash_table_lookup (self->priv->contact_details,
      GUINT_TO_POINTER (contact));
}

static ExampleContactDetails *
ensure_contact (ExampleContactListManager *self,
                TpHandle contact,
                gboolean *created)
{
  ExampleContactDetails *ret = lookup_contact (self, contact);

  if (ret == NULL)
    {
      tp_handle_set_add (self->priv->contacts, contact);

      ret = example_contact_details_new ();
      ret->alias = g_strdup (tp_handle_inspect (self->priv->contact_repo,
            contact));

      g_hash_table_insert (self->priv->contact_details,
          GUINT_TO_POINTER (contact), ret);

      if (created != NULL)
        *created = TRUE;
    }
  else if (created != NULL)
    {
      *created = FALSE;
    }

  return ret;
}

static void
example_contact_list_manager_foreach_channel (TpChannelManager *manager,
                                              TpExportableChannelFunc callback,
                                              gpointer user_data)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (manager);
  GHashTableIter iter;
  gpointer handle, channel;
  guint i;

  for (i = 0; i < NUM_EXAMPLE_CONTACT_LISTS; i++)
    {
      if (self->priv->lists[i] != NULL)
        callback (TP_EXPORTABLE_CHANNEL (self->priv->lists[i]), user_data);
    }

  g_hash_table_iter_init (&iter, self->priv->groups);

  while (g_hash_table_iter_next (&iter, &handle, &channel))
    {
      callback (TP_EXPORTABLE_CHANNEL (channel), user_data);
    }
}

static ExampleContactGroup *ensure_group (ExampleContactListManager *self,
    TpHandle handle);

static ExampleContactList *ensure_list (ExampleContactListManager *self,
    ExampleContactListHandle handle);

static gboolean
receive_contact_lists (gpointer p)
{
  ExampleContactListManager *self = p;
  TpHandle handle, cambridge, montreal, francophones;
  ExampleContactDetails *d;
  TpIntSet *set, *cam_set, *mtl_set, *fr_set;
  TpIntSetFastIter iter;
  ExampleContactList *subscribe, *publish, *stored, *deny;
  ExampleContactGroup *cambridge_group, *montreal_group,
      *francophones_group;

  if (self->priv->groups == NULL)
    {
      /* connection already disconnected, so don't process the
       * "data from the server" */
      return FALSE;
    }

  /* In a real CM we'd have received a contact list from the server at this
   * point. But this isn't a real CM, so we have to make one up... */

  g_message ("Receiving roster from server");

  subscribe = ensure_list (self, EXAMPLE_CONTACT_LIST_SUBSCRIBE);
  publish = ensure_list (self, EXAMPLE_CONTACT_LIST_PUBLISH);
  stored = ensure_list (self, EXAMPLE_CONTACT_LIST_STORED);
  deny = ensure_list (self, EXAMPLE_CONTACT_LIST_DENY);

  cambridge = tp_handle_ensure (self->priv->group_repo, "Cambridge", NULL,
      NULL);
  montreal = tp_handle_ensure (self->priv->group_repo, "Montreal", NULL,
      NULL);
  francophones = tp_handle_ensure (self->priv->group_repo, "Francophones",
      NULL, NULL);

  cambridge_group = ensure_group (self, cambridge);
  montreal_group = ensure_group (self, montreal);
  francophones_group = ensure_group (self, francophones);

  /* Add various people who are already subscribing and publishing */

  set = tp_intset_new ();
  cam_set = tp_intset_new ();
  mtl_set = tp_intset_new ();
  fr_set = tp_intset_new ();

  handle = tp_handle_ensure (self->priv->contact_repo, "sjoerd@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  tp_intset_add (cam_set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Sjoerd");
  d->subscribe = TRUE;
  d->publish = TRUE;
  d->stored = TRUE;
  d->tags = tp_handle_set_new (self->priv->group_repo);
  tp_handle_set_add (d->tags, cambridge);

  handle = tp_handle_ensure (self->priv->contact_repo, "guillaume@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  tp_intset_add (cam_set, handle);
  tp_intset_add (fr_set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Guillaume");
  d->subscribe = TRUE;
  d->publish = TRUE;
  d->stored = TRUE;
  d->tags = tp_handle_set_new (self->priv->group_repo);
  tp_handle_set_add (d->tags, cambridge);
  tp_handle_set_add (d->tags, francophones);

  handle = tp_handle_ensure (self->priv->contact_repo, "olivier@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  tp_intset_add (mtl_set, handle);
  tp_intset_add (fr_set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Olivier");
  d->subscribe = TRUE;
  d->publish = TRUE;
  d->stored = TRUE;
  d->tags = tp_handle_set_new (self->priv->group_repo);
  tp_handle_set_add (d->tags, montreal);
  tp_handle_set_add (d->tags, francophones);

  handle = tp_handle_ensure (self->priv->contact_repo, "travis@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Travis");
  d->subscribe = TRUE;
  d->publish = TRUE;
  d->stored = TRUE;

  tp_group_mixin_change_members ((GObject *) subscribe, "",
      set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) publish, "",
      set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_intset_fast_iter_init (&iter, set);

  while (tp_intset_fast_iter_next (&iter, &handle))
    {
      g_signal_emit (self, signals[ALIAS_UPDATED], 0, handle);
      g_signal_emit (self, signals[PRESENCE_UPDATED], 0, handle);
    }

  tp_intset_destroy (set);

  /* Add a couple of people whose presence we've requested. They are
   * remote-pending in subscribe */

  set = tp_intset_new ();

  handle = tp_handle_ensure (self->priv->contact_repo, "geraldine@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  tp_intset_add (cam_set, handle);
  tp_intset_add (fr_set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Géraldine");
  d->subscribe_requested = TRUE;
  d->stored = TRUE;
  d->tags = tp_handle_set_new (self->priv->group_repo);
  tp_handle_set_add (d->tags, cambridge);
  tp_handle_set_add (d->tags, francophones);

  handle = tp_handle_ensure (self->priv->contact_repo, "helen@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  tp_intset_add (cam_set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Helen");
  d->subscribe_requested = TRUE;
  d->stored = TRUE;
  d->tags = tp_handle_set_new (self->priv->group_repo);
  tp_handle_set_add (d->tags, cambridge);

  tp_group_mixin_change_members ((GObject *) subscribe, "",
      NULL, NULL, NULL, set,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_intset_fast_iter_init (&iter, set);

  while (tp_intset_fast_iter_next (&iter, &handle))
    {
      g_signal_emit (self, signals[ALIAS_UPDATED], 0, handle);
      g_signal_emit (self, signals[PRESENCE_UPDATED], 0, handle);
    }

  tp_intset_destroy (set);

  /* Receive a couple of authorization requests too. These people are
   * local-pending in publish */

  handle = tp_handle_ensure (self->priv->contact_repo, "wim@example.com",
      NULL, NULL);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Wim");
  d->publish_requested = TRUE;
  d->stored = TRUE;

  set = tp_intset_new_containing (handle);
  tp_group_mixin_change_members ((GObject *) publish,
      "I'm more metal than you!",
      NULL, NULL, set, NULL,
      handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);
  g_signal_emit (self, signals[ALIAS_UPDATED], 0, handle);
  g_signal_emit (self, signals[PRESENCE_UPDATED], 0, handle);

  handle = tp_handle_ensure (self->priv->contact_repo, "christian@example.com",
      NULL, NULL);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Christian");
  d->publish_requested = TRUE;
  d->stored = TRUE;

  set = tp_intset_new_containing (handle);
  tp_group_mixin_change_members ((GObject *) publish,
      "I have some fermented herring for you",
      NULL, NULL, set, NULL,
      handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);
  g_signal_emit (self, signals[ALIAS_UPDATED], 0, handle);
  g_signal_emit (self, signals[PRESENCE_UPDATED], 0, handle);

  /* Add a couple of people who are blocked */

  set = tp_intset_new ();

  handle = tp_handle_ensure (self->priv->contact_repo, "bill@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Bill");
  d->blocked = TRUE;

  handle = tp_handle_ensure (self->priv->contact_repo, "steve@example.com",
      NULL, NULL);
  tp_intset_add (set, handle);
  d = ensure_contact (self, handle, NULL);
  g_free (d->alias);
  d->alias = g_strdup ("Steve");
  d->blocked = TRUE;

  tp_group_mixin_change_members ((GObject *) deny, "",
      set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_intset_fast_iter_init (&iter, set);

  while (tp_intset_fast_iter_next (&iter, &handle))
    {
      g_signal_emit (self, signals[ALIAS_UPDATED], 0, handle);
      g_signal_emit (self, signals[PRESENCE_UPDATED], 0, handle);
    }

  tp_intset_destroy (set);

  /* Handle groups */

  tp_group_mixin_change_members ((GObject *) cambridge_group, "",
      cam_set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) montreal_group, "",
      mtl_set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) francophones_group, "",
      fr_set, NULL, NULL, NULL,
      0, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);

  tp_intset_destroy (fr_set);
  tp_intset_destroy (cam_set);
  tp_intset_destroy (mtl_set);

  /* Now we've received the roster, we can satisfy all the queued requests */

  example_contact_list_manager_foreach_channel ((TpChannelManager *) self,
      satisfy_queued_requests, self);

  g_assert (g_hash_table_size (self->priv->queued_requests) == 0);
  g_hash_table_destroy (self->priv->queued_requests);
  self->priv->queued_requests = NULL;

  return FALSE;
}

static void
status_changed_cb (TpBaseConnection *conn,
                   guint status,
                   guint reason,
                   ExampleContactListManager *self)
{
  switch (status)
    {
    case TP_CONNECTION_STATUS_CONNECTED:
        {
          /* Do network I/O to get the contact list. This connection manager
           * doesn't really have a server, so simulate a small network delay
           * then invent a contact list */
          g_timeout_add_full (G_PRIORITY_DEFAULT,
              2 * self->priv->simulation_delay, receive_contact_lists,
              g_object_ref (self), g_object_unref);
        }
      break;

    case TP_CONNECTION_STATUS_DISCONNECTED:
        {
          example_contact_list_manager_close_all (self);
        }
      break;
    }
}

static void
constructed (GObject *object)
{
  ExampleContactListManager *self = EXAMPLE_CONTACT_LIST_MANAGER (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) example_contact_list_manager_parent_class)->constructed;

  if (chain_up != NULL)
    {
      chain_up (object);
    }

  self->priv->contact_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_CONTACT);
  self->priv->group_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_GROUP);
  self->priv->contacts = tp_handle_set_new (self->priv->contact_repo);

  self->priv->status_changed_id = g_signal_connect (self->priv->conn,
      "status-changed", (GCallback) status_changed_cb, self);
}

static void
example_contact_list_manager_class_init (ExampleContactListManagerClass *klass)
{
  GParamSpec *param_spec;
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->constructed = constructed;
  object_class->dispose = dispose;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  param_spec = g_param_spec_object ("connection", "Connection object",
      "The connection that owns this channel manager",
      TP_TYPE_BASE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
      G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_uint ("simulation-delay", "Simulation delay",
      "Delay between simulated network events",
      0, G_MAXUINT32, 1000,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SIMULATION_DELAY,
      param_spec);

  g_type_class_add_private (klass, sizeof (ExampleContactListManagerPrivate));

  signals[ALIAS_UPDATED] = g_signal_new ("alias-updated",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);

  signals[PRESENCE_UPDATED] = g_signal_new ("presence-updated",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL, NULL,
      g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
list_closed_cb (ExampleContactList *chan,
                ExampleContactListManager *self)
{
  TpHandle handle;

  tp_channel_manager_emit_channel_closed_for_object (self,
      TP_EXPORTABLE_CHANNEL (chan));

  g_object_get (chan,
      "handle", &handle,
      NULL);

  if (self->priv->lists[handle] == NULL)
    return;

  g_assert (chan == self->priv->lists[handle]);
  g_object_unref (self->priv->lists[handle]);
  self->priv->lists[handle] = NULL;
}

static void
group_closed_cb (ExampleContactGroup *chan,
                 ExampleContactListManager *self)
{
  tp_channel_manager_emit_channel_closed_for_object (self,
      TP_EXPORTABLE_CHANNEL (chan));

  if (self->priv->groups != NULL)
    {
      TpHandle handle;

      g_object_get (chan,
          "handle", &handle,
          NULL);

      g_hash_table_remove (self->priv->groups, GUINT_TO_POINTER (handle));
    }
}

static ExampleContactListBase *
new_channel (ExampleContactListManager *self,
             TpHandleType handle_type,
             TpHandle handle,
             gpointer request_token)
{
  ExampleContactListBase *chan;
  gchar *object_path;
  GType type;
  GSList *requests = NULL;

  if (handle_type == TP_HANDLE_TYPE_LIST)
    {
      /* Some Telepathy clients wrongly assume that contact lists of type LIST
       * have object paths ending with "/subscribe", "/publish" etc. -
       * telepathy-spec has no such guarantee, so in this example we break
       * those clients. Please read the spec when implementing it :-) */
      object_path = g_strdup_printf ("%s/%sContactList",
          self->priv->conn->object_path, _contact_lists[handle - 1]);
      type = EXAMPLE_TYPE_CONTACT_LIST;
    }
  else
    {
      /* Using Group%u (with handle as the value of %u) would be OK here too,
       * but we'll encode the group name into the object path to be kind
       * to people reading debug logs. */
      gchar *id = tp_escape_as_identifier (tp_handle_inspect (
            self->priv->group_repo, handle));

      g_assert (handle_type == TP_HANDLE_TYPE_GROUP);
      object_path = g_strdup_printf ("%s/Group/%s",
          self->priv->conn->object_path, id);
      type = EXAMPLE_TYPE_CONTACT_GROUP;

      g_free (id);
    }

  chan = g_object_new (type,
      "connection", self->priv->conn,
      "manager", self,
      "object-path", object_path,
      "handle-type", handle_type,
      "handle", handle,
      NULL);

  g_free (object_path);

  if (handle_type == TP_HANDLE_TYPE_LIST)
    {
      g_signal_connect (chan, "closed", (GCallback) list_closed_cb, self);
      g_assert (self->priv->lists[handle] == NULL);
      self->priv->lists[handle] = EXAMPLE_CONTACT_LIST (chan);
    }
  else
    {
      g_signal_connect (chan, "closed", (GCallback) group_closed_cb, self);

      g_assert (g_hash_table_lookup (self->priv->groups,
            GUINT_TO_POINTER (handle)) == NULL);
      g_hash_table_insert (self->priv->groups, GUINT_TO_POINTER (handle),
          EXAMPLE_CONTACT_GROUP (chan));
    }

  if (self->priv->queued_requests == NULL)
    {
      if (request_token != NULL)
        requests = g_slist_prepend (requests, request_token);

      tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (chan),
          requests);
      g_slist_free (requests);
    }
  else if (request_token != NULL)
    {
      /* initial contact list not received yet, so we have to wait for it */
      requests = g_hash_table_lookup (self->priv->queued_requests, chan);
      g_hash_table_steal (self->priv->queued_requests, chan);
      requests = g_slist_prepend (requests, request_token);
      g_hash_table_insert (self->priv->queued_requests, chan, requests);
    }

  return chan;
}

static ExampleContactList *
ensure_list (ExampleContactListManager *self,
             ExampleContactListHandle handle)
{
  if (self->priv->lists[handle] == NULL)
    {
      new_channel (self, TP_HANDLE_TYPE_LIST, handle, NULL);
      g_assert (self->priv->lists[handle] != NULL);
    }

  return self->priv->lists[handle];
}

static ExampleContactGroup *
ensure_group (ExampleContactListManager *self,
              TpHandle handle)
{
  ExampleContactGroup *group = g_hash_table_lookup (self->priv->groups,
      GUINT_TO_POINTER (handle));

  if (group == NULL)
    {
      group = EXAMPLE_CONTACT_GROUP (new_channel (self, TP_HANDLE_TYPE_GROUP,
            handle, NULL));
    }

  return group;
}

static const gchar * const fixed_properties[] = {
    TP_PROP_CHANNEL_CHANNEL_TYPE,
    TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
    NULL
};

static const gchar * const allowed_properties[] = {
    TP_PROP_CHANNEL_TARGET_HANDLE,
    TP_PROP_CHANNEL_TARGET_ID,
    NULL
};

static void
example_contact_list_manager_foreach_channel_class (TpChannelManager *manager,
    TpChannelManagerChannelClassFunc func,
    gpointer user_data)
{
    GHashTable *table = tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE,
            G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_LIST,
        NULL);

    func (manager, table, allowed_properties, user_data);

    g_hash_table_insert (table, TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        tp_g_value_slice_new_uint (TP_HANDLE_TYPE_GROUP));
    func (manager, table, allowed_properties, user_data);

    g_hash_table_destroy (table);
}

static gboolean
example_contact_list_manager_request (ExampleContactListManager *self,
                                      gpointer request_token,
                                      GHashTable *request_properties,
                                      gboolean require_new)
{
  TpHandleType handle_type;
  TpHandle handle;
  ExampleContactListBase *chan;
  GError *error = NULL;

  if (tp_strdiff (tp_asv_get_string (request_properties,
          TP_PROP_CHANNEL_CHANNEL_TYPE),
      TP_IFACE_CHANNEL_TYPE_CONTACT_LIST))
    {
      return FALSE;
    }

  handle_type = tp_asv_get_uint32 (request_properties,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, NULL);

  if (handle_type != TP_HANDLE_TYPE_LIST &&
      handle_type != TP_HANDLE_TYPE_GROUP)
    {
      return FALSE;
    }

  handle = tp_asv_get_uint32 (request_properties,
      TP_PROP_CHANNEL_TARGET_HANDLE, NULL);
  g_assert (handle != 0);

  if (tp_channel_manager_asv_has_unknown_properties (request_properties,
        fixed_properties, allowed_properties, &error))
    {
      goto error;
    }

  if (handle_type == TP_HANDLE_TYPE_LIST)
    {
      /* telepathy-glib has already checked that the handle is valid */
      g_assert (handle < NUM_EXAMPLE_CONTACT_LISTS);

      chan = EXAMPLE_CONTACT_LIST_BASE (self->priv->lists[handle]);
    }
  else
    {
      chan = g_hash_table_lookup (self->priv->groups,
          GUINT_TO_POINTER (handle));
    }

  if (chan == NULL)
    {
      new_channel (self, handle_type, handle, request_token);
    }
  else if (require_new)
    {
      g_set_error (&error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "A ContactList channel for type #%u, handle #%u already exists",
          handle_type, handle);
      goto error;
    }
  else
    {
      tp_channel_manager_emit_request_already_satisfied (self,
          request_token, TP_EXPORTABLE_CHANNEL (chan));
    }

  return TRUE;

error:
  tp_channel_manager_emit_request_failed (self, request_token,
      error->domain, error->code, error->message);
  g_error_free (error);
  return TRUE;
}

static gboolean
example_contact_list_manager_create_channel (TpChannelManager *manager,
                                             gpointer request_token,
                                             GHashTable *request_properties)
{
    return example_contact_list_manager_request (
        EXAMPLE_CONTACT_LIST_MANAGER (manager), request_token,
        request_properties, TRUE);
}

static gboolean
example_contact_list_manager_ensure_channel (TpChannelManager *manager,
                                             gpointer request_token,
                                             GHashTable *request_properties)
{
    return example_contact_list_manager_request (
        EXAMPLE_CONTACT_LIST_MANAGER (manager), request_token,
        request_properties, FALSE);
}

static void
channel_manager_iface_init (gpointer g_iface,
                            gpointer data G_GNUC_UNUSED)
{
  TpChannelManagerIface *iface = g_iface;

  iface->foreach_channel = example_contact_list_manager_foreach_channel;
  iface->foreach_channel_class =
      example_contact_list_manager_foreach_channel_class;
  iface->create_channel = example_contact_list_manager_create_channel;
  iface->ensure_channel = example_contact_list_manager_ensure_channel;
  /* In this channel manager, Request has the same semantics as Ensure */
  iface->request_channel = example_contact_list_manager_ensure_channel;
}

static void
send_updated_roster (ExampleContactListManager *self,
                     TpHandle contact)
{
  ExampleContactDetails *d = g_hash_table_lookup (self->priv->contact_details,
      GUINT_TO_POINTER (contact));
  const gchar *identifier = tp_handle_inspect (self->priv->contact_repo,
      contact);

  /* In a real connection manager, we'd transmit these new details to the
   * server, rather than just printing messages. */

  if (d == NULL)
    {
      g_message ("Deleting contact %s from server", identifier);
    }
  else
    {
      g_message ("Transmitting new state of contact %s to server", identifier);
      g_message ("\talias = %s", d->alias);
      g_message ("\tcan see our presence = %s",
          d->publish ? "yes" :
          (d->publish_requested ? "no, but has requested it" : "no"));
      g_message ("\tsends us presence = %s",
          d->subscribe ? "yes" :
          (d->subscribe_requested ? "no, but we have requested it" : "no"));
      g_message ("\tstored = %s", d->stored ? "yes" : "no");
      g_message ("\tblocked = %s", d->blocked ? "yes" : "no");

      if (d->tags == NULL || tp_handle_set_size (d->tags) == 0)
        {
          g_message ("\tnot in any groups");
        }
      else
        {
          TpIntSet *set = tp_handle_set_peek (d->tags);
          TpIntSetFastIter iter;
          TpHandle member;

          tp_intset_fast_iter_init (&iter, set);

          while (tp_intset_fast_iter_next (&iter, &member))
            {
              g_message ("\tin group: %s",
                  tp_handle_inspect (self->priv->group_repo, member));
            }
        }
    }
}

gboolean
example_contact_list_manager_add_to_group (ExampleContactListManager *self,
                                           GObject *channel,
                                           TpHandle group,
                                           TpHandle member,
                                           const gchar *message,
                                           GError **error)
{
  gboolean updated;
  ExampleContactDetails *d = ensure_contact (self, member, &updated);
  ExampleContactList *stored = self->priv->lists[
    EXAMPLE_CONTACT_LIST_STORED];

  if (d->tags == NULL)
    d->tags = tp_handle_set_new (self->priv->group_repo);

  if (!tp_handle_set_is_member (d->tags, group))
    {
      tp_handle_set_add (d->tags, group);
      updated = TRUE;
    }

  if (updated)
    {
      TpIntSet *added = tp_intset_new_containing (member);

      d->stored = TRUE;

      send_updated_roster (self, member);
      tp_group_mixin_change_members (channel, "", added, NULL, NULL, NULL,
          self->priv->conn->self_handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
      tp_group_mixin_change_members ((GObject *) stored, "",
          added, NULL, NULL, NULL,
          self->priv->conn->self_handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
      tp_intset_destroy (added);
    }

  return TRUE;
}

gboolean
example_contact_list_manager_remove_from_group (
    ExampleContactListManager *self,
    GObject *channel,
    TpHandle group,
    TpHandle member,
    const gchar *message,
    GError **error)
{
  ExampleContactDetails *d = lookup_contact (self, member);

  /* If not on the roster or not in any groups, we have nothing to do */
  if (d == NULL || d->tags == NULL)
    return TRUE;

  if (tp_handle_set_remove (d->tags, group))
    {
      TpIntSet *removed = tp_intset_new_containing (member);

      send_updated_roster (self, member);
      tp_group_mixin_change_members (channel, "", NULL, removed, NULL, NULL,
          self->priv->conn->self_handle, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
      tp_intset_destroy (removed);
    }

  return TRUE;
}

typedef struct {
    ExampleContactListManager *self;
    TpHandle contact;
} SelfAndContact;

static SelfAndContact *
self_and_contact_new (ExampleContactListManager *self,
                      TpHandle contact)
{
  SelfAndContact *ret = g_slice_new0 (SelfAndContact);

  ret->self = g_object_ref (self);
  ret->contact = contact;
  return ret;
}

static void
self_and_contact_destroy (gpointer p)
{
  SelfAndContact *s = p;

  g_object_unref (s->self);
  g_slice_free (SelfAndContact, s);
}

static void
receive_auth_request (ExampleContactListManager *self,
                      TpHandle contact)
{
  ExampleContactDetails *d;
  TpIntSet *set;
  ExampleContactList *publish = self->priv->lists[
    EXAMPLE_CONTACT_LIST_PUBLISH];
  ExampleContactList *stored = self->priv->lists[
    EXAMPLE_CONTACT_LIST_STORED];

  /* if shutting down, do nothing */
  if (publish == NULL)
    return;

  /* A remote contact has asked to see our presence.
   *
   * In a real connection manager this would be the result of incoming
   * data from the server. */

  g_message ("From server: %s has sent us a publish request",
      tp_handle_inspect (self->priv->contact_repo, contact));

  d = ensure_contact (self, contact, NULL);

  if (d->publish)
    return;

  d->publish_requested = TRUE;
  d->stored = TRUE;

  set = tp_intset_new_containing (contact);
  tp_group_mixin_change_members ((GObject *) publish,
      "May I see your presence, please?",
      NULL, NULL, set, NULL,
      contact, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      contact, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);
}

static gboolean
receive_authorized (gpointer p)
{
  SelfAndContact *s = p;
  ExampleContactDetails *d;
  TpIntSet *set;
  ExampleContactList *subscribe = s->self->priv->lists[
    EXAMPLE_CONTACT_LIST_SUBSCRIBE];
  ExampleContactList *stored = s->self->priv->lists[
    EXAMPLE_CONTACT_LIST_STORED];

  /* A remote contact has accepted our request to see their presence.
   *
   * In a real connection manager this would be the result of incoming
   * data from the server. */

  g_message ("From server: %s has accepted our subscribe request",
      tp_handle_inspect (s->self->priv->contact_repo, s->contact));

  d = ensure_contact (s->self, s->contact, NULL);

  /* if we were already subscribed to them, then nothing really happened */
  if (d->subscribe)
    return FALSE;

  d->subscribe_requested = FALSE;
  d->subscribe = TRUE;
  d->stored = TRUE;

  set = tp_intset_new_containing (s->contact);
  tp_group_mixin_change_members ((GObject *) subscribe, "",
      set, NULL, NULL, NULL,
      s->contact, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL,
      s->contact, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);

  /* their presence changes to something other than UNKNOWN */
  g_signal_emit (s->self, signals[PRESENCE_UPDATED], 0, s->contact);

  /* if we're not publishing to them, also pretend they have asked us to
   * do so */
  if (!d->publish)
    {
      receive_auth_request (s->self, s->contact);
    }

  return FALSE;
}

static gboolean
receive_unauthorized (gpointer p)
{
  SelfAndContact *s = p;
  ExampleContactDetails *d;
  TpIntSet *set;
  ExampleContactList *subscribe = s->self->priv->lists[
    EXAMPLE_CONTACT_LIST_SUBSCRIBE];

  /* if shutting down, do nothing */
  if (subscribe == NULL)
    return FALSE;

  /* A remote contact has rejected our request to see their presence.
   *
   * In a real connection manager this would be the result of incoming
   * data from the server. */

  g_message ("From server: %s has rejected our subscribe request",
      tp_handle_inspect (s->self->priv->contact_repo, s->contact));

  d = ensure_contact (s->self, s->contact, NULL);

  if (!d->subscribe && !d->subscribe_requested)
    return FALSE;

  d->subscribe_requested = FALSE;
  d->subscribe = FALSE;

  set = tp_intset_new_containing (s->contact);
  tp_group_mixin_change_members ((GObject *) subscribe, "Say 'please'!",
      NULL, set, NULL, NULL,
      s->contact, TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);

  /* their presence changes to UNKNOWN */
  g_signal_emit (s->self, signals[PRESENCE_UPDATED], 0, s->contact);

  return FALSE;
}

gboolean
example_contact_list_manager_add_to_list (ExampleContactListManager *self,
                                          GObject *channel,
                                          ExampleContactListHandle list,
                                          TpHandle member,
                                          const gchar *message,
                                          GError **error)
{
  TpIntSet *set;
  ExampleContactList *stored = self->priv->lists[EXAMPLE_CONTACT_LIST_STORED];

  switch (list)
    {
    case EXAMPLE_CONTACT_LIST_SUBSCRIBE:
      /* we would like to see member's presence */
        {
          gboolean created;
          ExampleContactDetails *d = ensure_contact (self, member, &created);
          gchar *message_lc;

          /* if they already authorized us, it's a no-op */
          if (d->subscribe)
            return TRUE;

          /* In a real connection manager we'd start a network request here */
          g_message ("Transmitting authorization request to %s: %s",
              tp_handle_inspect (self->priv->contact_repo, member),
              message);

          if (created || !d->subscribe_requested)
            {
              d->subscribe_requested = TRUE;
              d->stored = TRUE;
              send_updated_roster (self, member);
            }

          set = tp_intset_new_containing (member);
          tp_group_mixin_change_members (channel, message,
              NULL, NULL, NULL, set,
              self->priv->conn->self_handle,
              TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
          /* subscribing to someone implicitly puts them on Stored, too */
          tp_group_mixin_change_members ((GObject *) stored, "",
              set, NULL, NULL, NULL,
              self->priv->conn->self_handle,
              TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
          tp_intset_destroy (set);

          /* Pretend that after a delay, the contact notices the request
           * and allows or rejects it. In this example connection manager,
           * empty requests are allowed, as are requests that contain "please"
           * case-insensitively. All other requests are denied. */
          message_lc = g_ascii_strdown (message, -1);

          if (message[0] == '\0' || strstr (message_lc, "please") != NULL)
            {
              g_timeout_add_full (G_PRIORITY_DEFAULT,
                  self->priv->simulation_delay, receive_authorized,
                  self_and_contact_new (self, member),
                  self_and_contact_destroy);
            }
          else
            {
              g_timeout_add_full (G_PRIORITY_DEFAULT,
                  self->priv->simulation_delay,
                  receive_unauthorized,
                  self_and_contact_new (self, member),
                  self_and_contact_destroy);
            }

          g_free (message_lc);
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_PUBLISH:
      /* We would like member to see our presence. This is meaningless,
       * unless they have asked for it. */
        {
          ExampleContactDetails *d = lookup_contact (self, member);

          if (d == NULL || !d->publish_requested)
            {
              /* the group mixin won't actually allow this to be reached,
               * because of the flags we set */
              g_set_error (error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
                  "Can't unilaterally send presence to %s",
                  tp_handle_inspect (self->priv->contact_repo, member));
              return FALSE;
            }

          if (!d->publish)
            {
              d->publish = TRUE;
              d->publish_requested = FALSE;
              d->stored = TRUE;
              send_updated_roster (self, member);

              set = tp_intset_new_containing (member);
              tp_group_mixin_change_members (channel, "",
                  set, NULL, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_group_mixin_change_members ((GObject *) stored, "",
                  set, NULL, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_intset_destroy (set);
            }
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_STORED:
      /* we would like member to be on the roster */
        {
          ExampleContactDetails *d = ensure_contact (self, member, NULL);

          d->stored = TRUE;
          send_updated_roster (self, member);

          set = tp_intset_new_containing (member);
          tp_group_mixin_change_members (channel, "",
              set, NULL, NULL, NULL, self->priv->conn->self_handle,
              TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
          tp_intset_destroy (set);
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_DENY:
      /* We would like member to be blocked */
        {
          ExampleContactDetails *d = ensure_contact (self, member, NULL);

          g_message ("Blocking %s",
              tp_handle_inspect (self->priv->contact_repo, member));

          d->blocked = TRUE;
          send_updated_roster (self, member);

          set = tp_intset_new_containing (member);
          tp_group_mixin_change_members (channel, "",
              set, NULL, NULL, NULL, self->priv->conn->self_handle,
              TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
          tp_intset_destroy (set);
        }
      return TRUE;

    default:
      g_return_val_if_reached (FALSE);
    }
}

static gboolean
auth_request_cb (gpointer p)
{
  SelfAndContact *s = p;

  receive_auth_request (s->self, s->contact);

  return FALSE;
}

gboolean
example_contact_list_manager_remove_from_list (ExampleContactListManager *self,
                                               GObject *channel,
                                               ExampleContactListHandle list,
                                               TpHandle member,
                                               const gchar *message,
                                               GError **error)
{
  TpIntSet *set;

  switch (list)
    {
    case EXAMPLE_CONTACT_LIST_PUBLISH:
      /* we would like member not to see our presence any more, or we
       * would like to reject a request from them to see our presence */
        {
          ExampleContactDetails *d = lookup_contact (self, member);

          if (d != NULL)
            {
              if (d->publish_requested)
                {
                  g_message ("Rejecting authorization request from %s",
                      tp_handle_inspect (self->priv->contact_repo, member));
                  d->publish_requested = FALSE;

                  set = tp_intset_new_containing (member);
                  tp_group_mixin_change_members (channel, "",
                      NULL, set, NULL, NULL,
                      self->priv->conn->self_handle,
                      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
                  tp_intset_destroy (set);
                }
              else if (d->publish)
                {
                  g_message ("Removing authorization from %s",
                      tp_handle_inspect (self->priv->contact_repo, member));
                  d->publish = FALSE;

                  set = tp_intset_new_containing (member);
                  tp_group_mixin_change_members (channel, "",
                      NULL, set, NULL, NULL,
                      self->priv->conn->self_handle,
                      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
                  tp_intset_destroy (set);

                  /* Pretend that after a delay, the contact notices the change
                   * and asks for our presence again */
                  g_timeout_add_full (G_PRIORITY_DEFAULT,
                      self->priv->simulation_delay, auth_request_cb,
                      self_and_contact_new (self, member),
                      self_and_contact_destroy);
                }
              else
                {
                  /* nothing to do, avoid "updating the roster" */
                  return TRUE;
                }

              send_updated_roster (self, member);
            }
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_SUBSCRIBE:
      /* we would like to avoid receiving member's presence any more,
       * or we would like to cancel an outstanding request for their
       * presence */
        {
          ExampleContactDetails *d = lookup_contact (self, member);

          if (d != NULL)
            {
              if (d->subscribe_requested)
                {
                  g_message ("Cancelling our authorization request to %s",
                      tp_handle_inspect (self->priv->contact_repo, member));
                  d->subscribe_requested = FALSE;

                  set = tp_intset_new_containing (member);
                  tp_group_mixin_change_members (channel, "",
                      NULL, set, NULL, NULL,
                      self->priv->conn->self_handle,
                      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
                  tp_intset_destroy (set);
                }
              else if (d->subscribe)
                {
                  g_message ("We no longer want presence from %s",
                      tp_handle_inspect (self->priv->contact_repo, member));
                  d->subscribe = FALSE;

                  set = tp_intset_new_containing (member);
                  tp_group_mixin_change_members (channel, "",
                      NULL, set, NULL, NULL,
                      self->priv->conn->self_handle,
                      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
                  tp_intset_destroy (set);

                  /* since they're no longer on the subscribe list, we can't
                   * see their presence, so emit a signal changing it to
                   * UNKNOWN */
                  g_signal_emit (self, signals[PRESENCE_UPDATED], 0, member);
                }
              else
                {
                  /* nothing to do, avoid "updating the roster" */
                  return TRUE;
                }

              send_updated_roster (self, member);
            }
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_STORED:
      /* we would like to remove member from the roster altogether */
        {
          ExampleContactDetails *d = lookup_contact (self, member);

          if (d != NULL)
            {
              /* if the contact is blocked, do not completely delete it */
              if (d->blocked)
                {
                  d->publish = FALSE;
                  d->publish_requested = FALSE;
                  d->subscribe = FALSE;
                  d->subscribe_requested = FALSE;
                  d->stored = FALSE;
                }
              else
                {
                  g_hash_table_remove (self->priv->contact_details,
                      GUINT_TO_POINTER (member));
                }
              send_updated_roster (self, member);

              set = tp_intset_new_containing (member);
              tp_group_mixin_change_members (channel, "",
                  NULL, set, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_group_mixin_change_members (
                  (GObject *) self->priv->lists[EXAMPLE_CONTACT_LIST_SUBSCRIBE],
                  "", NULL, set, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_group_mixin_change_members (
                  (GObject *) self->priv->lists[EXAMPLE_CONTACT_LIST_PUBLISH],
                  "", NULL, set, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_intset_destroy (set);

              if (!d->blocked)
                tp_handle_set_remove (self->priv->contacts, member);

              /* since they're no longer on the subscribe list, we can't
               * see their presence, so emit a signal changing it to
               * UNKNOWN */
              g_signal_emit (self, signals[PRESENCE_UPDATED], 0, member);

            }
        }
      return TRUE;

    case EXAMPLE_CONTACT_LIST_DENY:
      /* we would like to unblock member */
        {
          ExampleContactDetails *d = lookup_contact (self, member);

          if (d != NULL)
            {
              g_message ("Unblocking %s",
                  tp_handle_inspect (self->priv->contact_repo, member));

              /* if the contact is also not stored, we need to delete it */
              if (!d->stored)
                {
                  g_hash_table_remove (self->priv->contact_details,
                      GUINT_TO_POINTER (member));
                }
              else
                {
                  d->blocked = FALSE;
                }

              send_updated_roster (self, member);

              set = tp_intset_new_containing (member);
              tp_group_mixin_change_members (channel, "",
                  NULL, set, NULL, NULL,
                  self->priv->conn->self_handle,
                  TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
              tp_intset_destroy (set);

              if (!d->stored)
                tp_handle_set_remove (self->priv->contacts, member);
            }
        }
      return TRUE;

    default:
      g_return_val_if_reached (FALSE);
    }
}

ExampleContactListPresence
example_contact_list_manager_get_presence (ExampleContactListManager *self,
                                           TpHandle contact)
{
  ExampleContactDetails *d = lookup_contact (self, contact);
  const gchar *id;

  if (d == NULL || !d->subscribe)
    {
      /* we don't know the presence of people not on the subscribe list,
       * by definition */
      return EXAMPLE_CONTACT_LIST_PRESENCE_UNKNOWN;
    }

  id = tp_handle_inspect (self->priv->contact_repo, contact);

  /* In this example CM, we fake contacts' presence based on their name:
   * contacts in the first half of the alphabet are available, the rest
   * (including non-alphabetic and non-ASCII initial letters) are away. */
  if ((id[0] >= 'A' && id[0] <= 'M') || (id[0] >= 'a' && id[0] <= 'm'))
    {
      return EXAMPLE_CONTACT_LIST_PRESENCE_AVAILABLE;
    }

  return EXAMPLE_CONTACT_LIST_PRESENCE_AWAY;
}

const gchar *
example_contact_list_manager_get_alias (ExampleContactListManager *self,
                                        TpHandle contact)
{
  ExampleContactDetails *d = lookup_contact (self, contact);

  if (d == NULL)
    {
      /* we don't have a user-defined alias for people not on the roster */
      return tp_handle_inspect (self->priv->contact_repo, contact);
    }

  return d->alias;
}

void
example_contact_list_manager_set_alias (ExampleContactListManager *self,
                                        TpHandle contact,
                                        const gchar *alias)
{
  gboolean created;
  ExampleContactDetails *d = ensure_contact (self, contact, &created);
  ExampleContactList *stored = self->priv->lists[
    EXAMPLE_CONTACT_LIST_STORED];
  gchar *old = d->alias;
  TpIntSet *set;

  /* FIXME: if stored list hasn't been retrieved yet, queue the change for
   * later */

  /* if shutting down, do nothing */
  if (stored == NULL)
    return;

  d->alias = g_strdup (alias);
  d->stored = TRUE;

  if (created || tp_strdiff (old, alias))
    send_updated_roster (self, contact);

  g_free (old);

  set = tp_intset_new_containing (contact);
  tp_group_mixin_change_members ((GObject *) stored, "",
      set, NULL, NULL, NULL, self->priv->conn->self_handle,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE);
  tp_intset_destroy (set);
}
