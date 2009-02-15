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

#ifndef __EXAMPLE_CSH_ROOM_MANAGER_H__
#define __EXAMPLE_CSH_ROOM_MANAGER_H__

#include <glib-object.h>
#include <telepathy-glib/channel-manager.h>

G_BEGIN_DECLS

typedef struct _ExampleCSHRoomManager ExampleCSHRoomManager;
typedef struct _ExampleCSHRoomManagerClass ExampleCSHRoomManagerClass;
typedef struct _ExampleCSHRoomManagerPrivate ExampleCSHRoomManagerPrivate;

struct _ExampleCSHRoomManagerClass {
    GObjectClass parent_class;
};

struct _ExampleCSHRoomManager {
    GObject parent;

    ExampleCSHRoomManagerPrivate *priv;
};

GType example_csh_room_manager_get_type (void);

void example_csh_room_manager_set_enable_change_members_detailed (ExampleCSHRoomManager *manager,
                                                                  gboolean enable);
void example_csh_room_manager_accept_invitations (ExampleCSHRoomManager *manager);

void example_csh_room_manager_set_use_properties_room (ExampleCSHRoomManager *manager,
                                                       gboolean use_properties_room);

/* TYPE MACROS */
#define EXAMPLE_TYPE_CSH_ROOM_MANAGER \
  (example_csh_room_manager_get_type ())
#define EXAMPLE_CSH_ROOM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLE_TYPE_CSH_ROOM_MANAGER, \
                              ExampleCSHRoomManager))
#define EXAMPLE_CSH_ROOM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLE_TYPE_CSH_ROOM_MANAGER, \
                           ExampleCSHRoomManagerClass))
#define EXAMPLE_IS_CSH_ROOM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLE_TYPE_CSH_ROOM_MANAGER))
#define EXAMPLE_IS_CSH_ROOM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLE_TYPE_CSH_ROOM_MANAGER))
#define EXAMPLE_CSH_ROOM_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_TYPE_CSH_ROOM_MANAGER, \
                              ExampleCSHRoomManagerClass))

G_END_DECLS

#endif
