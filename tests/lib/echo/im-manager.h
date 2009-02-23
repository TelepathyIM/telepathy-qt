/*
 * im-manager.h - header for an example channel manager
 *
 * Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef __EXAMPLE_ECHO_IM_MANAGER_H__
#define __EXAMPLE_ECHO_IM_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ExampleEchoImManager ExampleEchoImManager;
typedef struct _ExampleEchoImManagerClass ExampleEchoImManagerClass;
typedef struct _ExampleEchoImManagerPrivate ExampleEchoImManagerPrivate;

struct _ExampleEchoImManagerClass {
    GObjectClass parent_class;
};

struct _ExampleEchoImManager {
    GObject parent;

    ExampleEchoImManagerPrivate *priv;
};

GType example_echo_im_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_ECHO_IM_MANAGER \
  (example_echo_im_manager_get_type ())
#define EXAMPLE_ECHO_IM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_ECHO_IM_MANAGER, \
                              ExampleEchoImManager))
#define EXAMPLE_ECHO_IM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_ECHO_IM_MANAGER, \
                           ExampleEchoImManagerClass))
#define EXAMPLE_IS_ECHO_IM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_ECHO_IM_MANAGER))
#define EXAMPLE_IS_ECHO_IM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_ECHO_IM_MANAGER))
#define EXAMPLE_ECHO_IM_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_IM_MANAGER, \
                              ExampleEchoImManagerClass))

G_END_DECLS

#endif
