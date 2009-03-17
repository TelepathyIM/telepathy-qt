/*
 * media-manager.h - header for an example channel manager
 *
 * Copyright © 2007-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2007-2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __EXAMPLE_CALLABLE_MEDIA_MANAGER_H__
#define __EXAMPLE_CALLABLE_MEDIA_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ExampleCallableMediaManager ExampleCallableMediaManager;
typedef struct _ExampleCallableMediaManagerPrivate
    ExampleCallableMediaManagerPrivate;

typedef struct _ExampleCallableMediaManagerClass
    ExampleCallableMediaManagerClass;
typedef struct _ExampleCallableMediaManagerClassPrivate
    ExampleCallableMediaManagerClassPrivate;

struct _ExampleCallableMediaManagerClass {
    GObjectClass parent_class;

    ExampleCallableMediaManagerClassPrivate *priv;
};

struct _ExampleCallableMediaManager {
    GObject parent;

    ExampleCallableMediaManagerPrivate *priv;
};

GType example_callable_media_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER \
  (example_callable_media_manager_get_type ())
#define EXAMPLE_CALLABLE_MEDIA_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER, \
                              ExampleCallableMediaManager))
#define EXAMPLE_CALLABLE_MEDIA_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER, \
                           ExampleCallableMediaManagerClass))
#define EXAMPLE_IS_CALLABLE_MEDIA_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER))
#define EXAMPLE_IS_CALLABLE_MEDIA_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER))
#define EXAMPLE_CALLABLE_MEDIA_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALLABLE_MEDIA_MANAGER, \
                              ExampleCallableMediaManagerClass))

G_END_DECLS

#endif
