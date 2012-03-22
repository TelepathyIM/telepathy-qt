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

#ifndef EXAMPLE_CALL_MANAGER_H
#define EXAMPLE_CALL_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ExampleCallManager ExampleCallManager;
typedef struct _ExampleCallManagerPrivate
    ExampleCallManagerPrivate;

typedef struct _ExampleCallManagerClass
    ExampleCallManagerClass;
typedef struct _ExampleCallManagerClassPrivate
    ExampleCallManagerClassPrivate;

struct _ExampleCallManagerClass {
    GObjectClass parent_class;

    ExampleCallManagerClassPrivate *priv;
};

struct _ExampleCallManager {
    GObject parent;

    ExampleCallManagerPrivate *priv;
};

GType example_call_manager_get_type (void);

/* TYPE MACROS */
#define EXAMPLE_TYPE_CALL_MANAGER \
  (example_call_manager_get_type ())
#define EXAMPLE_CALL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CALL_MANAGER, \
                              ExampleCallManager))
#define EXAMPLE_CALL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CALL_MANAGER, \
                           ExampleCallManagerClass))
#define EXAMPLE_IS_CALL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CALL_MANAGER))
#define EXAMPLE_IS_CALL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CALL_MANAGER))
#define EXAMPLE_CALL_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CALL_MANAGER, \
                              ExampleCallManagerClass))

G_END_DECLS

#endif
