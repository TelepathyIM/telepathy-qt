/*
 * Example channel manager for contact lists
 *
 * Copyright © 2007-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2010 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "contact-list-manager.h"

#include <string.h>
#include <telepathy-glib/telepathy-glib.h>

struct _TestContactListManagerPrivate
{
  TpBaseConnection *conn;

  gulong status_changed_id;

  /* TpHandle => ContactDetails */
  GHashTable *contact_details;

  TpHandleRepoIface *contact_repo;
  TpHandleRepoIface *group_repo;
  TpHandleSet *groups;
};

static void contact_groups_iface_init (TpContactGroupListInterface *iface);
static void mutable_contact_groups_iface_init (
    TpMutableContactGroupListInterface *iface);

G_DEFINE_TYPE_WITH_CODE (TestContactListManager, test_contact_list_manager,
    TP_TYPE_BASE_CONTACT_LIST,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CONTACT_GROUP_LIST,
      contact_groups_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_MUTABLE_CONTACT_GROUP_LIST,
      mutable_contact_groups_iface_init))

typedef struct {
  TpSubscriptionState subscribe;
  TpSubscriptionState publish;
  gchar *publish_request;
  TpHandleSet *groups;

  TpHandle handle;
  TpHandleRepoIface *contact_repo;
} ContactDetails;

static void
contact_detail_destroy (gpointer p)
{
  ContactDetails *d = p;

  g_free (d->publish_request);
  tp_handle_set_destroy (d->groups);

  g_slice_free (ContactDetails, d);
}

static ContactDetails *
lookup_contact (TestContactListManager *self,
                TpHandle handle)
{
  return g_hash_table_lookup (self->priv->contact_details,
      GUINT_TO_POINTER (handle));
}

static ContactDetails *
ensure_contact (TestContactListManager *self,
                TpHandle handle)
{
  ContactDetails *d = lookup_contact (self, handle);

  if (d == NULL)
    {
      d = g_slice_new0 (ContactDetails);
      d->subscribe = TP_SUBSCRIPTION_STATE_NO;
      d->publish = TP_SUBSCRIPTION_STATE_NO;
      d->publish_request = NULL;
      d->groups = tp_handle_set_new (self->priv->group_repo);
      d->handle = handle;
      d->contact_repo = self->priv->contact_repo;

      g_hash_table_insert (self->priv->contact_details,
          GUINT_TO_POINTER (handle), d);
    }

  return d;
}

static void
test_contact_list_manager_init (TestContactListManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      TEST_TYPE_CONTACT_LIST_MANAGER, TestContactListManagerPrivate);

  self->priv->contact_details = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, NULL, contact_detail_destroy);
}

static void
close_all (TestContactListManager *self)
{
  if (self->priv->status_changed_id != 0)
    {
      g_signal_handler_disconnect (self->priv->conn,
          self->priv->status_changed_id);
      self->priv->status_changed_id = 0;
    }
  tp_clear_pointer (&self->priv->contact_details, g_hash_table_unref);
  tp_clear_pointer (&self->priv->groups, tp_handle_set_destroy);
}

static void
dispose (GObject *object)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (object);

  close_all (self);

  ((GObjectClass *) test_contact_list_manager_parent_class)->dispose (
    object);
}

static TpHandleSet *
contact_list_dup_contacts (TpBaseContactList *base)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  TpHandleSet *set;
  GHashTableIter iter;
  gpointer k, v;

  set = tp_handle_set_new (self->priv->contact_repo);

  g_hash_table_iter_init (&iter, self->priv->contact_details);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      ContactDetails *d = v;

      /* add all the interesting items */
      if (d->subscribe != TP_SUBSCRIPTION_STATE_NO ||
          d->publish != TP_SUBSCRIPTION_STATE_NO)
        tp_handle_set_add (set, GPOINTER_TO_UINT (k));
    }

  return set;
}

static void
contact_list_dup_states (TpBaseContactList *base,
    TpHandle contact,
    TpSubscriptionState *subscribe,
    TpSubscriptionState *publish,
    gchar **publish_request)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  ContactDetails *d = lookup_contact (self, contact);

  if (d == NULL)
    {
      if (subscribe != NULL)
        *subscribe = TP_SUBSCRIPTION_STATE_NO;

      if (publish != NULL)
        *publish = TP_SUBSCRIPTION_STATE_NO;

      if (publish_request != NULL)
        *publish_request = NULL;
    }
  else
    {
      if (subscribe != NULL)
        *subscribe = d->subscribe;

      if (publish != NULL)
        *publish = d->publish;

      if (publish_request != NULL)
        *publish_request = g_strdup (d->publish_request);
    }
}

static GStrv
contact_list_dup_groups (TpBaseContactList *base)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  GPtrArray *ret;

  if (self->priv->groups != NULL)
    {
      TpIntSetFastIter iter;
      TpHandle group;

      ret = g_ptr_array_sized_new (tp_handle_set_size (self->priv->groups) + 1);

      tp_intset_fast_iter_init (&iter, tp_handle_set_peek (self->priv->groups));
      while (tp_intset_fast_iter_next (&iter, &group))
        {
          g_ptr_array_add (ret, g_strdup (tp_handle_inspect (
              self->priv->group_repo, group)));
        }
    }
  else
    {
      ret = g_ptr_array_sized_new (1);
    }

  g_ptr_array_add (ret, NULL);

  return (GStrv) g_ptr_array_free (ret, FALSE);
}

static GStrv
contact_list_dup_contact_groups (TpBaseContactList *base,
    TpHandle contact)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  ContactDetails *d = lookup_contact (self, contact);
  GPtrArray *ret;

  if (d != NULL && d->groups != NULL)
    {
      TpIntSetFastIter iter;
      TpHandle group;

      ret = g_ptr_array_sized_new (tp_handle_set_size (d->groups) + 1);

      tp_intset_fast_iter_init (&iter, tp_handle_set_peek (d->groups));
      while (tp_intset_fast_iter_next (&iter, &group))
        {
          g_ptr_array_add (ret, g_strdup (tp_handle_inspect (
              self->priv->group_repo, group)));
        }
    }
  else
    {
      ret = g_ptr_array_sized_new (1);
    }

  g_ptr_array_add (ret, NULL);

  return (GStrv) g_ptr_array_free (ret, FALSE);
}

static TpHandleSet *
contact_list_dup_group_members (TpBaseContactList *base,
    const gchar *group)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  TpHandleSet *set;
  TpHandle group_handle;
  GHashTableIter iter;
  gpointer k, v;

  set = tp_handle_set_new (self->priv->contact_repo);
  group_handle = tp_handle_lookup (self->priv->group_repo, group, NULL, NULL);
  if (G_UNLIKELY (group_handle == 0))
    {
      /* clearly it doesn't have members */
      return set;
    }

  g_hash_table_iter_init (&iter, self->priv->contact_details);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      ContactDetails *d = v;

      if (d->groups != NULL &&
          tp_handle_set_is_member (d->groups, group_handle))
        tp_handle_set_add (set, GPOINTER_TO_UINT (k));
    }

  return set;
}

static void
contact_list_set_contact_groups_async (TpBaseContactList *base,
    TpHandle contact,
    const gchar * const *names,
    gsize n,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (base);
  ContactDetails *d;
  TpIntset *set, *added_set, *removed_set;
  GPtrArray *added_names, *removed_names;
  TpIntSetFastIter iter;
  TpHandle group_handle;
  guint i;

  d = ensure_contact (self, contact);

  set = tp_intset_new ();
  for (i = 0; i < n; i++)
    {
      group_handle = tp_handle_ensure (self->priv->group_repo, names[i], NULL, NULL);
      tp_intset_add (set, group_handle);
    }

  added_set = tp_intset_difference (set, tp_handle_set_peek (d->groups));
  added_names = g_ptr_array_sized_new (tp_intset_size (added_set));
  tp_intset_fast_iter_init (&iter, added_set);
  while (tp_intset_fast_iter_next (&iter, &group_handle))
    {
      g_ptr_array_add (added_names, (gchar *) tp_handle_inspect (
          self->priv->group_repo, group_handle));
    }
  tp_intset_destroy (added_set);

  removed_set = tp_intset_difference (tp_handle_set_peek (d->groups), set);
  removed_names = g_ptr_array_sized_new (tp_intset_size (removed_set));
  tp_intset_fast_iter_init (&iter, removed_set);
  while (tp_intset_fast_iter_next (&iter, &group_handle))
    {
      g_ptr_array_add (removed_names, (gchar *) tp_handle_inspect (
          self->priv->group_repo, group_handle));
    }
  tp_intset_destroy (removed_set);

  tp_handle_set_destroy (d->groups);
  d->groups = tp_handle_set_new_from_intset (self->priv->group_repo, set);
  tp_intset_destroy (set);

  tp_base_contact_list_one_contact_groups_changed (base, contact,
      (const gchar * const *) added_names->pdata, added_names->len,
      (const gchar * const *) removed_names->pdata, removed_names->len);

  tp_simple_async_report_success_in_idle ((GObject *) self, callback,
      user_data, contact_list_set_contact_groups_async);

  g_ptr_array_unref (added_names);
  g_ptr_array_unref (removed_names);
}

static void
contact_list_set_group_members_async (TpBaseContactList *base,
    const gchar *normalized_group,
    TpHandleSet *contacts,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new_error ((GObject *) base, callback,
      user_data, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED, "Not implemented");
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static void
contact_list_add_to_group_async (TpBaseContactList *base,
    const gchar *group,
    TpHandleSet *contacts,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new_error ((GObject *) base, callback,
      user_data, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED, "Not implemented");
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static void
contact_list_remove_from_group_async (TpBaseContactList *base,
    const gchar *group,
    TpHandleSet *contacts,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new_error ((GObject *) base, callback,
      user_data, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED, "Not implemented");
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static void
contact_list_remove_group_async (TpBaseContactList *base,
    const gchar *group,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new_error ((GObject *) base, callback,
      user_data, TP_ERROR, TP_ERROR_NOT_IMPLEMENTED, "Not implemented");
  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
}

static void
status_changed_cb (TpBaseConnection *conn,
                   guint status,
                   guint reason,
                   TestContactListManager *self)
{
  switch (status)
    {
    case TP_CONNECTION_STATUS_CONNECTED:
        {
          tp_base_contact_list_set_list_received (TP_BASE_CONTACT_LIST (self));
        }
      break;

    case TP_CONNECTION_STATUS_DISCONNECTED:
        {
          close_all (self);
        }
      break;
    }
}

static void
constructed (GObject *object)
{
  TestContactListManager *self = TEST_CONTACT_LIST_MANAGER (object);
  void (*chain_up) (GObject *) =
      ((GObjectClass *) test_contact_list_manager_parent_class)->constructed;

  if (chain_up != NULL)
    {
      chain_up (object);
    }

  self->priv->conn = tp_base_contact_list_get_connection (
      TP_BASE_CONTACT_LIST (self), NULL);
  self->priv->status_changed_id = g_signal_connect (self->priv->conn,
      "status-changed", G_CALLBACK (status_changed_cb), self);

  self->priv->contact_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_CONTACT);
  self->priv->group_repo = tp_base_connection_get_handles (self->priv->conn,
      TP_HANDLE_TYPE_GROUP);
  self->priv->groups = tp_handle_set_new (self->priv->group_repo);
}

static void
contact_groups_iface_init (TpContactGroupListInterface *iface)
{
  iface->dup_groups = contact_list_dup_groups;
  iface->dup_contact_groups = contact_list_dup_contact_groups;
  iface->dup_group_members = contact_list_dup_group_members;
}

static void
mutable_contact_groups_iface_init (
    TpMutableContactGroupListInterface *iface)
{
  iface->set_contact_groups_async = contact_list_set_contact_groups_async;
  iface->set_group_members_async = contact_list_set_group_members_async;
  iface->add_to_group_async = contact_list_add_to_group_async;
  iface->remove_from_group_async = contact_list_remove_from_group_async;
  iface->remove_group_async = contact_list_remove_group_async;
}

static void
test_contact_list_manager_class_init (TestContactListManagerClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseContactListClass *base_class =(TpBaseContactListClass *) klass;

  g_type_class_add_private (klass, sizeof (TestContactListManagerPrivate));

  object_class->constructed = constructed;
  object_class->dispose = dispose;

  base_class->dup_states = contact_list_dup_states;
  base_class->dup_contacts = contact_list_dup_contacts;
}

void
test_contact_list_manager_add_to_group (TestContactListManager *self,
    const gchar *group_name, TpHandle member)
{
  TpBaseContactList *base = TP_BASE_CONTACT_LIST (self);
  ContactDetails *d = ensure_contact (self, member);
  TpHandle group_handle;

  group_handle = tp_handle_ensure (self->priv->group_repo, group_name, NULL, NULL);

  tp_handle_set_add (d->groups, group_handle);
  tp_base_contact_list_one_contact_groups_changed (base, member,
      &group_name, 1, NULL, 0);
}

void
test_contact_list_manager_remove_from_group (TestContactListManager *self,
    const gchar *group_name, TpHandle member)
{
  TpBaseContactList *base = TP_BASE_CONTACT_LIST (self);
  ContactDetails *d = lookup_contact (self, member);
  TpHandle group_handle;

  if (d == NULL)
    return;

  group_handle = tp_handle_ensure (self->priv->group_repo, group_name, NULL, NULL);

  tp_handle_set_remove (d->groups, group_handle);
  tp_base_contact_list_one_contact_groups_changed (base, member,
      NULL, 0, &group_name, 1);
}

typedef struct {
    TestContactListManager *self;
    TpHandleSet *handles;
} SelfAndContact;

static SelfAndContact *
self_and_contact_new (TestContactListManager *self,
  TpHandleSet *handles)
{
  SelfAndContact *ret = g_slice_new0 (SelfAndContact);

  ret->self = g_object_ref (self);
  ret->handles = tp_handle_set_copy (handles);

  return ret;
}

static void
self_and_contact_destroy (gpointer p)
{
  SelfAndContact *s = p;

  tp_handle_set_destroy (s->handles);
  g_object_unref (s->self);
  g_slice_free (SelfAndContact, s);
}

static gboolean
receive_authorized (gpointer p)
{
  SelfAndContact *s = p;
  GArray *handles_array;
  guint i;

  handles_array = tp_handle_set_to_array (s->handles);
  for (i = 0; i < handles_array->len; i++)
    {
      ContactDetails *d = lookup_contact (s->self,
          g_array_index (handles_array, TpHandle, i));

      if (d == NULL)
        continue;

      d->subscribe = TP_SUBSCRIPTION_STATE_YES;

      /* if we're not publishing to them, also pretend they have asked us to do so */
      if (d->publish != TP_SUBSCRIPTION_STATE_YES)
        {
          d->publish = TP_SUBSCRIPTION_STATE_ASK;
          tp_clear_pointer (&d->publish_request, g_free);
          d->publish_request = g_strdup ("automatic publish request");
        }
    }
  g_array_unref (handles_array);

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (s->self),
      s->handles, NULL);

  return FALSE;
}

static gboolean
receive_unauthorized (gpointer p)
{
  SelfAndContact *s = p;
  GArray *handles_array;
  guint i;

  handles_array = tp_handle_set_to_array (s->handles);
  for (i = 0; i < handles_array->len; i++)
    {
      ContactDetails *d = lookup_contact (s->self,
          g_array_index (handles_array, TpHandle, i));

      if (d == NULL)
        continue;

      d->subscribe = TP_SUBSCRIPTION_STATE_REMOVED_REMOTELY;
    }
  g_array_unref (handles_array);

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (s->self),
      s->handles, NULL);

  return FALSE;
}

void
test_contact_list_manager_request_subscription (TestContactListManager *self,
    guint n_members, TpHandle *members,  const gchar *message)
{
  TpHandleSet *handles;
  guint i;
  gchar *message_lc;

  handles = tp_handle_set_new (self->priv->contact_repo);
  for (i = 0; i < n_members; i++)
    {
      ContactDetails *d = ensure_contact (self, members[i]);

      if (d->subscribe == TP_SUBSCRIPTION_STATE_YES)
        continue;

      d->subscribe = TP_SUBSCRIPTION_STATE_ASK;
      tp_handle_set_add (handles, members[i]);
    }

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (self), handles,
      NULL);

  message_lc = g_ascii_strdown (message, -1);
  if (strstr (message_lc, "please") != NULL)
    {
      g_idle_add_full (G_PRIORITY_DEFAULT,
          receive_authorized,
          self_and_contact_new (self, handles),
          self_and_contact_destroy);
    }
  else if (strstr (message_lc, "no") != NULL)
    {
      g_idle_add_full (G_PRIORITY_DEFAULT,
          receive_unauthorized,
          self_and_contact_new (self, handles),
          self_and_contact_destroy);
    }

  g_free (message_lc);
  tp_handle_set_destroy (handles);
}

void
test_contact_list_manager_unsubscribe (TestContactListManager *self,
    guint n_members, TpHandle *members)
{
  TpHandleSet *handles;
  guint i;

  handles = tp_handle_set_new (self->priv->contact_repo);
  for (i = 0; i < n_members; i++)
    {
      ContactDetails *d = lookup_contact (self, members[i]);

      if (d == NULL || d->subscribe == TP_SUBSCRIPTION_STATE_NO)
        continue;

      d->subscribe = TP_SUBSCRIPTION_STATE_NO;
      tp_handle_set_add (handles, members[i]);
    }

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (self), handles,
      NULL);

  tp_handle_set_destroy (handles);
}

void
test_contact_list_manager_authorize_publication (TestContactListManager *self,
    guint n_members, TpHandle *members)
{
  TpHandleSet *handles;
  guint i;

  handles = tp_handle_set_new (self->priv->contact_repo);
  for (i = 0; i < n_members; i++)
    {
      ContactDetails *d = lookup_contact (self, members[i]);

      if (d == NULL || d->publish != TP_SUBSCRIPTION_STATE_ASK)
        continue;

      d->publish = TP_SUBSCRIPTION_STATE_YES;
      tp_clear_pointer (&d->publish_request, g_free);
      tp_handle_set_add (handles, members[i]);
    }

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (self), handles,
      NULL);

  tp_handle_set_destroy (handles);
}

void
test_contact_list_manager_unpublish (TestContactListManager *self,
    guint n_members, TpHandle *members)
{
  TpHandleSet *handles;
  guint i;

  handles = tp_handle_set_new (self->priv->contact_repo);
  for (i = 0; i < n_members; i++)
    {
      ContactDetails *d = lookup_contact (self, members[i]);

      if (d == NULL || d->publish == TP_SUBSCRIPTION_STATE_NO)
        continue;

      d->publish = TP_SUBSCRIPTION_STATE_NO;
      tp_clear_pointer (&d->publish_request, g_free);
      tp_handle_set_add (handles, members[i]);
    }

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (self), handles,
      NULL);

  tp_handle_set_destroy (handles);
}

void
test_contact_list_manager_remove (TestContactListManager *self,
    guint n_members, TpHandle *members)
{
  TpHandleSet *handles;
  guint i;

  handles = tp_handle_set_new (self->priv->contact_repo);
  for (i = 0; i < n_members; i++)
    {
      ContactDetails *d = lookup_contact (self, members[i]);

      if (d == NULL)
        continue;

      g_hash_table_remove (self->priv->contact_details,
          GUINT_TO_POINTER (members[i]));
      tp_handle_set_add (handles, members[i]);
    }

  tp_base_contact_list_contacts_changed (TP_BASE_CONTACT_LIST (self), NULL,
      handles);

  tp_handle_set_destroy (handles);
}

