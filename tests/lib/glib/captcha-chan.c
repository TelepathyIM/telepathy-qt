/*
 * captcha-chan.c - Simple captcha channel
 *
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#include "captcha-chan.h"

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/svc-channel.h>

#include <glib/gstdio.h>

enum
{
  PROP_METHOD = 1,
  PROP_RETRY,
  PROP_STATUS,
  PROP_ERROR,
  PROP_ERROR_DETAILS
};


struct _TpTestsCaptchaChannelPrivate {
    TpCaptchaStatus status;

    TpSocketAccessControl access_control;

    GHashTable *parameters;

    gchar *error_string;
    GHashTable *error_details;

    gboolean can_retry;
    gboolean is_retrying;
};

static void
tp_tests_captcha_channel_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) object;

  switch (property_id)
    {
      case PROP_METHOD:
        g_value_set_string (value, TP_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION);
        break;

      case PROP_RETRY:
        g_value_set_boolean (value, self->priv->can_retry);
        break;

      case PROP_STATUS:
        g_value_set_uint (value, self->priv->status);
        break;

      case PROP_ERROR:
        g_value_set_string (value, self->priv->error_string);
        break;

      case PROP_ERROR_DETAILS:
        g_value_set_boxed (value, self->priv->error_details);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tp_tests_captcha_channel_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) object;

  switch (property_id)
    {
      case PROP_RETRY:
        self->priv->can_retry = g_value_get_boolean(value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void captcha_iface_init (gpointer iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (TpTestsCaptchaChannel,
    tp_tests_captcha_channel,
    TP_TYPE_BASE_CHANNEL,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_SERVER_AUTHENTICATION,
      NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION,
      captcha_iface_init);
    )

/* type definition stuff */

static const char * tp_tests_captcha_channel_interfaces[] = {
    TP_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION,
    NULL
};

static void
tp_tests_captcha_channel_init (TpTestsCaptchaChannel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
      TP_TESTS_TYPE_CAPTCHA_CHANNEL, TpTestsCaptchaChannelPrivate);
}

static GObject *
constructor (GType type,
             guint n_props,
             GObjectConstructParam *props)
{
  GObject *object =
      G_OBJECT_CLASS (tp_tests_captcha_channel_parent_class)->constructor (
          type, n_props, props);
  TpTestsCaptchaChannel *self = TP_TESTS_CAPTCHA_CHANNEL (object);

  self->priv->status = TP_CAPTCHA_STATUS_LOCAL_PENDING;
  self->priv->is_retrying = FALSE;
  self->priv->error_string = NULL;
  self->priv->error_details = tp_asv_new (NULL, NULL);

  tp_base_channel_register (TP_BASE_CHANNEL (self));

  return object;
}

static void
dispose (GObject *object)
{
//  TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) object;

  // Free memory here if needed

  ((GObjectClass *) tp_tests_captcha_channel_parent_class)->dispose (
    object);
}

static void
channel_close (TpBaseChannel *channel)
{
  tp_base_channel_destroyed (channel);
}

static void
fill_immutable_properties (TpBaseChannel *chan,
    GHashTable *properties)
{
  TpBaseChannelClass *klass = TP_BASE_CHANNEL_CLASS (
      tp_tests_captcha_channel_parent_class);

  tp_dbus_properties_mixin_fill_properties_hash (
                G_OBJECT (chan), properties,
                TP_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION, "AuthenticationMethod",
                TP_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION, "CanRetryCaptcha",
                NULL);

  klass->fill_immutable_properties (chan, properties);
}

static void
tp_tests_captcha_channel_class_init (TpTestsCaptchaChannelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);
  GParamSpec *param_spec;

  static TpDBusPropertiesMixinPropImpl server_authentication_props[] = {
      { "AuthenticationMethod", "authentication-method", NULL },
      { NULL, NULL, NULL }
  };

  static TpDBusPropertiesMixinPropImpl captcha_authentication_props[] = {
      { "CanRetryCaptcha", "can-retry-captcha", NULL },
      { "CaptchaStatus", "captcha-status", NULL },
      { "CaptchaError",  "captcha-error", NULL },
      { "CaptchaErrorDetails", "captcha-error-details", NULL },
      { NULL }
  };

  object_class->constructor = constructor;
  object_class->get_property = tp_tests_captcha_channel_get_property;
  object_class->set_property = tp_tests_captcha_channel_set_property;
  object_class->dispose = dispose;

  base_class->channel_type = TP_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION;
  base_class->target_handle_type = TP_HANDLE_TYPE_NONE;
  base_class->interfaces = tp_tests_captcha_channel_interfaces;
  base_class->close = channel_close;
  base_class->fill_immutable_properties = fill_immutable_properties;

  param_spec = g_param_spec_string ("authentication-method", "AuthenticationMethod",
      "the authentication method for this channel",
       "",
       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_METHOD, param_spec);

  param_spec = g_param_spec_boolean (
      "can-retry-captcha", "CanRetryCaptcha",
      "Whether Captcha can be retried or not.",
      FALSE,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_RETRY,
      param_spec);

  param_spec = g_param_spec_string (
      "captcha-error", "CaptchaError",
      "error details of the captcha",
      "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ERROR,
      param_spec);

  param_spec = g_param_spec_boxed (
      "captcha-error-details", "CaptchaErrorDetails",
      "error details of the captcha",
      TP_HASH_TYPE_STRING_VARIANT_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ERROR_DETAILS,
      param_spec);

  param_spec = g_param_spec_uint (
      "captcha-status", "CaptchaStatus",
      "state of the captcha",
      0, NUM_TP_CAPTCHA_STATUSES - 1, 0,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_STATUS,
      param_spec);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_TYPE_SERVER_AUTHENTICATION,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      server_authentication_props);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_IFACE_QUARK_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      captcha_authentication_props);

  g_type_class_add_private (object_class,
      sizeof (TpTestsCaptchaChannelPrivate));
}

static void
set_status (TpTestsCaptchaChannel *self,
                                 guint status,
                                 const gchar *error,
                                 GHashTable *error_details)
{
    GPtrArray *changed = g_ptr_array_new ();
    GHashTable *realerrors = error_details == NULL ? tp_asv_new (NULL, NULL) : error_details;

    if (self->priv->status != status)
    {
        self->priv->status = status;
        g_ptr_array_add (changed, (gpointer) "CaptchaStatus");
    }

    if (self->priv->error_string != error)
    {
        g_free (self->priv->error_string);
        self->priv->error_string = g_strdup (error);
        g_ptr_array_add (changed, (gpointer) "CaptchaError");
    }

    if (self->priv->error_details != realerrors)
    {
        if (self->priv->error_details != NULL)
            g_hash_table_unref (self->priv->error_details);
        self->priv->error_details = realerrors;
        if (self->priv->error_details != NULL)
            g_hash_table_ref (self->priv->error_details);
        g_ptr_array_add (changed, (gpointer) "CaptchaErrorDetails");
    }

    if (changed->len > 0)
    {
        g_ptr_array_add (changed, NULL);
        tp_dbus_properties_mixin_emit_properties_changed (G_OBJECT (self),
            TP_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION,
            (const gchar * const *) changed->pdata);
    }

    g_ptr_array_unref (changed);
}

static void
captcha_auth_answer_captchas (TpSvcChannelInterfaceCaptchaAuthentication *iface,
                              GHashTable *answers,
                              DBusGMethodInvocation *context)
{
    TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) iface;
    const gchar *answer;
    GError *error;

    if (self->priv->status != TP_CAPTCHA_STATUS_LOCAL_PENDING)
    {
        error = g_error_new (TP_ERROR,
            TP_ERROR_NOT_AVAILABLE, "Captcha status is in state %u",
            self->priv->status);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    set_status(self, TP_CAPTCHA_STATUS_REMOTE_PENDING, NULL, NULL);

    answer = (const gchar *) g_hash_table_lookup (answers,
        GUINT_TO_POINTER (42));

    if (answer == NULL)
    {
        error = g_error_new (TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
            "Missing required challenge ID (%u)", 42);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (tp_str_empty (answer))
    {
        error = g_error_new_literal (TP_ERROR,
            TP_ERROR_INVALID_ARGUMENT, "Empty answer");
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (tp_strdiff (answer, "This is the right answer"))
    {
        if (self->priv->can_retry)
        {
            set_status(self, TP_CAPTCHA_STATUS_TRY_AGAIN, NULL, NULL);
        }
        else
        {
            set_status(self, TP_CAPTCHA_STATUS_FAILED, NULL, NULL);
        }

        error = g_error_new_literal (TP_ERROR,
            TP_ERROR_INVALID_ARGUMENT, "Wrong answer");
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }
    else
    {
        set_status(self, TP_CAPTCHA_STATUS_SUCCEEDED, NULL, NULL);
    }

    tp_svc_channel_interface_captcha_authentication_return_from_answer_captchas (context);
}

static void
captcha_auth_get_captchas (TpSvcChannelInterfaceCaptchaAuthentication *iface,
                           DBusGMethodInvocation *context)
{
    TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) iface;
    GPtrArray *infos;
    GValueArray *info;
    static const gchar *mime_types_1[] = { "image/png", NULL };
    static const gchar *mime_types_2[] = { "lol/wut", NULL };
    static const gchar *no_mime_types[] = { NULL };

    if (self->priv->status != TP_CAPTCHA_STATUS_LOCAL_PENDING &&
        self->priv->status != TP_CAPTCHA_STATUS_TRY_AGAIN)
    {
        GError *error = g_error_new (TP_ERROR,
            TP_ERROR_NOT_AVAILABLE, "Captcha status is in state %u",
            self->priv->status);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    /* Yes, g_ptr_array_new_full() could be used here, but use
     * these functions instead to support older GLibs */
    infos = g_ptr_array_sized_new (5);
    g_ptr_array_set_free_func (infos, (GDestroyNotify) g_value_array_free);
    info = tp_value_array_build (5,
        G_TYPE_UINT, 42,
        G_TYPE_STRING, "ocr",
        G_TYPE_STRING, "Enter the text displayed",
        G_TYPE_UINT, 0,
        G_TYPE_STRV, mime_types_1,
        G_TYPE_INVALID);
    g_ptr_array_add (infos, info);
    info = tp_value_array_build (5,
        G_TYPE_UINT, 76,
        G_TYPE_STRING, "picture_q",
        G_TYPE_STRING, "What in this picture?",
        G_TYPE_UINT, 0,
        G_TYPE_STRV, mime_types_2,
        G_TYPE_INVALID);
    g_ptr_array_add (infos, info);
    info = tp_value_array_build (5,
        G_TYPE_UINT, 15,
        G_TYPE_STRING, "qa",
        G_TYPE_STRING, "What is the answer?",
        G_TYPE_UINT, 0,
        G_TYPE_STRV, no_mime_types,
        G_TYPE_INVALID);
    g_ptr_array_add (infos, info);
    info = tp_value_array_build (5,
        G_TYPE_UINT, 51,
        G_TYPE_STRING, "video_q",
        G_TYPE_STRING, "Totallyfake",
        G_TYPE_UINT, 0,
        G_TYPE_STRV, no_mime_types,
        G_TYPE_INVALID);
    g_ptr_array_add (infos, info);
    info = tp_value_array_build (5,
        G_TYPE_UINT, 17,
        G_TYPE_STRING, "video_recog",
        G_TYPE_STRING, "Totallyfakeurgonnadie",
        G_TYPE_UINT, 0,
        G_TYPE_STRV, mime_types_2,
        G_TYPE_INVALID);
    g_ptr_array_add (infos, info);

    if (self->priv->status == TP_CAPTCHA_STATUS_TRY_AGAIN)
    {
        /* Handler started trying again, change status back */
        set_status (self,
            TP_CAPTCHA_STATUS_LOCAL_PENDING, NULL, NULL);
        self->priv->is_retrying = TRUE;
    }

    tp_svc_channel_interface_captcha_authentication_return_from_get_captchas (context,
        infos, 1, "");

    g_ptr_array_unref (infos);
}

static void
captcha_auth_get_captcha_data (TpSvcChannelInterfaceCaptchaAuthentication *iface,
                               guint id,
                               const gchar *mime_type,
                               DBusGMethodInvocation *context)
{
    TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) iface;
    GArray *captcha;

    if (self->priv->status != TP_CAPTCHA_STATUS_LOCAL_PENDING)
    {
        GError *error = g_error_new (TP_ERROR,
            TP_ERROR_NOT_AVAILABLE, "Captcha status is in state %u",
            self->priv->status);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (id != 42 && id != 76)
    {
        GError *error = g_error_new (TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
            "Invalid captcha ID (%u).",
            id);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    if (tp_strdiff (mime_type, "image/png") && tp_strdiff (mime_type, "lol/wut"))
    {
        GError *error = g_error_new (TP_ERROR,
            TP_ERROR_INVALID_ARGUMENT,
            "MIME type '%s' was not in the list provided. ", mime_type);
        dbus_g_method_return_error (context, error);
        g_error_free (error);
        return;
    }

    captcha = g_array_new (TRUE, FALSE, sizeof (guchar));

    if (self->priv->is_retrying)
    {
        g_array_append_vals (captcha,
            "This is a reloaded payload", 26);
    }
    else
    {
        g_array_append_vals (captcha,
            "This is a fake payload", 22);
    }

    tp_svc_channel_interface_captcha_authentication_return_from_get_captcha_data (context,
        captcha);

    g_array_unref (captcha);
}

static void
captcha_auth_cancel_captcha (TpSvcChannelInterfaceCaptchaAuthentication *iface,
                             guint reason,
                             const gchar *debug_message,
                             DBusGMethodInvocation *context)
{
    TpTestsCaptchaChannel *self = (TpTestsCaptchaChannel *) iface;
    const gchar *error = NULL;
    GError *gerror = NULL;
    GHashTable *error_details = NULL;

    if (self->priv->status == TP_CAPTCHA_STATUS_FAILED)
    {
        gerror = g_error_new_literal (TP_ERROR,
            TP_ERROR_NOT_AVAILABLE, "Captcha status is already Failed");
        dbus_g_method_return_error (context, gerror);
        g_error_free (gerror);
        return;
    }

    switch (reason)
    {
    case TP_CAPTCHA_CANCEL_REASON_USER_CANCELLED:
        error = TP_ERROR_STR_CANCELLED;
        break;
    case TP_CAPTCHA_CANCEL_REASON_NOT_SUPPORTED:
        error = TP_ERROR_STR_CAPTCHA_NOT_SUPPORTED;
        break;
    case TP_CAPTCHA_CANCEL_REASON_SERVICE_CONFUSED:
        error = TP_ERROR_STR_SERVICE_CONFUSED;
        break;
    default:
        g_warning ("Unknown cancel reason");
    }

    error_details = tp_asv_new (
        "debug-message", G_TYPE_STRING, debug_message,
        NULL);

    set_status (self, TP_CAPTCHA_STATUS_FAILED,
        error, error_details);

    g_hash_table_unref (error_details);

    tp_svc_channel_interface_captcha_authentication_return_from_cancel_captcha (context);
}

static void
captcha_iface_init (gpointer g_iface,
                    gpointer data G_GNUC_UNUSED)
{
    TpSvcChannelInterfaceCaptchaAuthenticationClass *iface =
        (TpSvcChannelInterfaceCaptchaAuthenticationClass *) g_iface;

#define IMPLEMENT(x) \
    tp_svc_channel_interface_captcha_authentication_implement_##x (iface, \
        captcha_auth_##x)

    IMPLEMENT(answer_captchas);
    IMPLEMENT(get_captchas);
    IMPLEMENT(get_captcha_data);
    IMPLEMENT(cancel_captcha);
#undef IMPLEMENT
}
