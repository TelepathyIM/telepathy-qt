/*
 * captcha-chan.h - Simple captcha authentication channel
 *
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __TP_CAPTCHA_CHAN_H__
#define __TP_CAPTCHA_CHAN_H__

#include <glib-object.h>
#include <telepathy-glib/base-channel.h>
#include <telepathy-glib/base-connection.h>
#include <telepathy-glib/message-mixin.h>

G_BEGIN_DECLS

/* Base Class */
typedef struct _TpTestsCaptchaChannel TpTestsCaptchaChannel;
typedef struct _TpTestsCaptchaChannelClass TpTestsCaptchaChannelClass;
typedef struct _TpTestsCaptchaChannelPrivate TpTestsCaptchaChannelPrivate;

GType tp_tests_captcha_channel_get_type (void);

#define TP_TESTS_TYPE_CAPTCHA_CHANNEL \
  (tp_tests_captcha_channel_get_type ())
#define TP_TESTS_CAPTCHA_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TP_TESTS_TYPE_CAPTCHA_CHANNEL, \
                               TpTestsCaptchaChannel))
#define TP_TESTS_CAPTCHA_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TP_TESTS_TYPE_CAPTCHA_CHANNEL, \
                            TpTestsCaptchaChannelClass))
#define TP_TESTS_IS_CAPTCHA_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_CAPTCHA_CHANNEL))
#define TP_TESTS_IS_CAPTCHA_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_CAPTCHA_CHANNEL))
#define TP_TESTS_CAPTCHA_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_CAPTCHA_CHANNEL, \
                              TpTestsCaptchaChannelClass))
#define EXAMPLE_IS_ECHO_2_CHANNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TP_TESTS_TYPE_CAPTCHA_CHANNEL))
#define EXAMPLE_IS_ECHO_2_CHANNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), TP_TESTS_TYPE_CAPTCHA_CHANNEL))
#define QUAD_CAPTCHA_CHANNEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TP_TESTS_TYPE_CAPTCHA_CHANNEL, \
    TpTestsCaptchaChannelClass))

struct _TpTestsCaptchaChannelClass {
    TpBaseChannelClass parent_class;
};

struct _TpTestsCaptchaChannel {
    TpBaseChannel parent;

    TpTestsCaptchaChannelPrivate *priv;
};


G_END_DECLS

#endif /* #ifndef __TP_CAPTCHA_CHAN_H__ */
