/*
 * manager.h - header for an example channel manager
 *
 * Copyright (C) 2007 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2007 Nokia Corporation
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.
 */

#ifndef EXAMPLE_ECHO_MESSAGE_PARTS_IM_MANAGER_H
#define EXAMPLE_ECHO_MESSAGE_PARTS_IM_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ExampleEcho2ImManager ExampleEcho2ImManager;
typedef struct _ExampleEcho2ImManagerClass ExampleEcho2ImManagerClass;
typedef struct _ExampleEcho2ImManagerPrivate ExampleEcho2ImManagerPrivate;

struct _ExampleEcho2ImManagerClass {
    GObjectClass parent_class;
};

struct _ExampleEcho2ImManager {
    GObject parent;

    ExampleEcho2ImManagerPrivate *priv;
};

GType example_echo_2_im_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_ECHO_2_IM_MANAGER \
  (example_echo_2_im_manager_get_type ())
#define EXAMPLE_ECHO_2_IM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_ECHO_2_IM_MANAGER, \
                              ExampleEcho2ImManager))
#define EXAMPLE_ECHO_2_IM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_ECHO_2_IM_MANAGER, \
                           ExampleEcho2ImManagerClass))
#define EXAMPLE_IS_ECHO_2_IM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_ECHO_2_IM_MANAGER))
#define EXAMPLE_IS_ECHO_2_IM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_ECHO_2_IM_MANAGER))
#define EXAMPLE_ECHO_2_IM_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_ECHO_2_IM_MANAGER, \
                              ExampleEcho2ImManagerClass))

G_END_DECLS

#endif
