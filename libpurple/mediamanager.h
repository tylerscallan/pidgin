/**
 * @file mediamanager.h Media Manager API
 * @ingroup core
 */

/* purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MEDIA_MANAGER_H_
#define __MEDIA_MANAGER_H_

#include <glib.h>
#include <glib-object.h>

#include "connection.h"
#include "media.h"

G_BEGIN_DECLS

#define PURPLE_TYPE_MEDIA_MANAGER            (purple_media_manager_get_type())
#define PURPLE_MEDIA_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PURPLE_TYPE_MEDIA_MANAGER, PurpleMediaManager))
#define PURPLE_MEDIA_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PURPLE_TYPE_MEDIA_MANAGER, PurpleMediaManagerClass))
#define PURPLE_IS_MEDIA_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PURPLE_TYPE_MEDIA_MANAGER))
#define PURPLE_IS_MEDIA_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PURPLE_TYPE_MEDIA_MANAGER))
#define PURPLE_MEDIA_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PURPLE_TYPE_MEDIA_MANAGER, PurpleMediaManagerClass))

/** @copydoc _PurpleMediaManager */
typedef struct _PurpleMediaManager PurpleMediaManager;
/** @copydoc _PurpleMediaManagerClass */
typedef struct _PurpleMediaManagerClass PurpleMediaManagerClass;
/** @copydoc _PurpleMediaManagerPrivate */
typedef struct _PurpleMediaManagerPrivate PurpleMediaManagerPrivate;

/** The media manager class. */
struct _PurpleMediaManagerClass
{
	GObjectClass parent_class;       /**< The parent class. */
};

/** The media manager's data. */
struct _PurpleMediaManager
{
	GObject parent;                  /**< The parent of this manager. */
	PurpleMediaManagerPrivate *priv; /**< Private data for the manager. */
};

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @cname Media Manager API                                              */
/**************************************************************************/
/*@{*/

/**
 * Gets the media manager's GType.
 *
 * @return The media manager's GType.
 */
GType purple_media_manager_get_type(void);

/**
 * Gets the "global" media manager object. It's created if it doesn't already exist.
 *
 * @return The "global" instance of the media manager object.
 */
PurpleMediaManager *purple_media_manager_get(void);

/**
 * Creates a media session.
 *
 * @param manager The media manager to create the session under.
 * @param gc The connection to create the session on.
 * @param conference_type The conference type to feed into Farsight2.
 * @param remote_user The remote user to initiate the session with.
 *
 * @return A newly created media session.
 */
PurpleMedia *purple_media_manager_create_media(PurpleMediaManager *manager,
						PurpleConnection *gc,
						const char *conference_type,
						const char *remote_user,
						gboolean initiator);

/**
 * Gets all of the media sessions.
 *
 * @param manager The media manager to get all of the sessions from.
 *
 * @return A list of all the media sessions.
 */
GList *purple_media_manager_get_media(PurpleMediaManager *manager);

/**
 * Gets all of the media sessions for a given connection.
 *
 * @param manager The media manager to get the sessions from.
 * @param pc The connection the sessions are on.
 *
 * @return A list of the media sessions on the given connection.
 */
GList *purple_media_manager_get_media_by_connection(
		PurpleMediaManager *manager, PurpleConnection *pc);

/**
 * Removes a media session from the media manager.
 *
 * @param manager The media manager to remove the media session from.
 * @param media The media session to remove.
 */
void
purple_media_manager_remove_media(PurpleMediaManager *manager,
				  PurpleMedia *media);

/**
 * Signals that output windows should be created for the chosen stream.
 *
 * This shouldn't be called outside of mediamanager.c and media.c
 *
 * @param manager Manager the output windows are registered with.
 * @param media Media session the output windows are registered for.
 * @param session_id The session the output windows are registered with.
 * @param participant The participant the output windows are registered with.
 *
 * @return TRUE if it succeeded, FALSE if it failed.
 */
gboolean purple_media_manager_create_output_window(
		PurpleMediaManager *manager, PurpleMedia *media,
		const gchar *session_id, const gchar *participant);

/**
 * Registers a video output window to be created for a given stream.
 *
 * @param manager The manager to register the output window with.
 * @param media The media instance to find the stream in.
 * @param session_id The session the stream is associated with.
 * @param participant The participant the stream is associated with.
 * @param window_id The window ID to embed the video in.
 *
 * @return A unique ID to the registered output window, 0 if it failed.
 */
gulong purple_media_manager_set_output_window(PurpleMediaManager *manager,
		PurpleMedia *media, const gchar *session_id,
		const gchar *participant, gulong window_id);

/**
 * Remove a previously registerd output window.
 *
 * @param manager The manager the output window was registered with.
 * @param output_window_id The ID of the output window.
 *
 * @return TRUE if it found the output window and was successful, else FALSE.
 */
gboolean purple_media_manager_remove_output_window(
		PurpleMediaManager *manager, gulong output_window_id);

/**
 * Remove all output windows for a given conference/session/participant/stream.
 *
 * @param manager The manager the output windows were registered with.
 * @param media The media instance the output windows were registered for.
 * @param session_id The session the output windows were registered for.
 * @param participant The participant the output windows were registered for.
 */
void purple_media_manager_remove_output_windows(
		PurpleMediaManager *manager, PurpleMedia *media,
		const gchar *session_id, const gchar *participant);
/*}@*/

#ifdef __cplusplus
}
#endif

G_END_DECLS

#endif  /* __MEDIA_MANAGER_H_ */