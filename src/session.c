/*
 * session management for Gaim
 *
 * Copyright (C) 2002, Robert McQueen <robot101@debian.org> but
 * much code shamelessly cribbed from GsmClient (C) 2001 Havoc
 * Pennington, which is in turn inspired by various other pieces
 * of code including GnomeClient (C) 1998 Carsten Schaar, Tom
 * Tromey, and twm session code (C) 1998 The Open Group.
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
 *
 */
#include "internal.h"

#include "core.h"
#include "debug.h"

extern char *opt_rcfile_arg;

#ifdef USE_SM

#include <X11/ICE/ICElib.h>
#include <X11/SM/SMlib.h>
#include <gdk/gdkx.h>
#include <unistd.h>
#include <fcntl.h>

#define ERROR_LENGTH 512

static IceIOErrorHandler ice_installed_io_error_handler;
static SmcConn session = NULL;
static gchar *myself = NULL;
static gboolean had_first_save = FALSE;
gboolean session_managed = FALSE;

/* ICE belt'n'braces stuff */

static gboolean ice_process_messages(GIOChannel *channel, GIOCondition condition,
	      gpointer data) {
	IceConn connection = (IceConn)data;
	IceProcessMessagesStatus status;

	/* please don't block... please! */
	status = IceProcessMessages(connection, NULL, NULL);

	if (status == IceProcessMessagesIOError) {
		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "ICE IO error, closing connection... ");

		/* IO error, please disconnect */
		IceSetShutdownNegotiation(connection, False);
		IceCloseConnection(connection);

		gaim_debug(GAIM_DEBUG_INFO, NULL, "done.\n");

		/* cancel the handler */
		return FALSE;
	}

	/* live to see another day */
	return TRUE;
}

static void ice_connection_watch(IceConn connection, IcePointer client_data,
	      Bool opening, IcePointer *watch_data) {
	guint input_id;

	if (opening) {
		GIOChannel *channel;

		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "Handling new ICE connection... ");

		/* ensure ICE connection is not passed to child processes */
		fcntl(IceConnectionNumber(connection), F_SETFD, FD_CLOEXEC);

		/* get glib to watch the connection for us */
		channel = g_io_channel_unix_new(IceConnectionNumber(connection));
		input_id = g_io_add_watch(channel, G_IO_IN | G_IO_ERR,
			     ice_process_messages, connection);
		g_io_channel_unref(channel);

		/* store the input ID as a pointer for when it closes */
		*watch_data = (IcePointer)GUINT_TO_POINTER(input_id);
	} else {
		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "Handling closed ICE connection... ");

		/* get the input ID back and stop watching it */
		input_id = GPOINTER_TO_UINT((gpointer) *watch_data);
		g_source_remove(input_id);
	}

	gaim_debug(GAIM_DEBUG_INFO, NULL, "done.\n");
}

/* We call any handler installed before (or after) ice_init but 
 * avoid calling the default libICE handler which does an exit().
 *
 * This means we do nothing by default, which is probably correct,
 * the connection will get closed by libICE
 */

static void ice_io_error_handler(IceConn connection) {
	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Handling ICE IO error... ");

	if (ice_installed_io_error_handler)
		(*ice_installed_io_error_handler)(connection);

	gaim_debug(GAIM_DEBUG_INFO, NULL, "done.\n");
}

static void ice_init() {
	IceIOErrorHandler default_handler;

	ice_installed_io_error_handler = IceSetIOErrorHandler(NULL);
	default_handler = IceSetIOErrorHandler(ice_io_error_handler);

	if (ice_installed_io_error_handler == default_handler)
		ice_installed_io_error_handler = NULL;

	IceAddConnectionWatch(ice_connection_watch, NULL);

	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "ICE initialized.\n");
}

/* my magic utility function */

static gchar **session_make_command(gchar *client_id) {
	gint i = 2;
	gint j = 0;
	gchar **ret;

	if (client_id) i += 2;
	if (opt_rcfile_arg) i += 2;

	ret = g_new(gchar *, i);
	ret[j++] = g_strdup(myself);

	if (client_id) {
		ret[j++] = g_strdup("--session");
		ret[j++] = g_strdup(client_id);
	}

	if (opt_rcfile_arg) {
		ret[j++] = g_strdup("--file");
		ret[j++] = g_strdup(opt_rcfile_arg);
	}

	ret[j++] = NULL;

	return ret;
}

/* SM callback handlers */

void session_save_yourself(SmcConn conn, SmPointer data, int save_type,
       Bool shutdown, int interact_style, Bool fast) {
	if (had_first_save == FALSE && save_type == SmSaveLocal &&
	      interact_style == SmInteractStyleNone && !shutdown &&
	      !fast) {
		/* this is just a dry run, spit it back */
		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "Received first save_yourself\n");
		SmcSaveYourselfDone(conn, True);
		had_first_save = TRUE;
		return;
	}

	/* tum ti tum... don't add anything else here without *
         * reading SMlib.PS from an X.org ftp server near you */

	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Received save_yourself\n");

	if (save_type == SmSaveGlobal || save_type == SmSaveBoth) {
		/* may as well do something ... */
		/* or not -- save_prefs(); */
	}

	SmcSaveYourselfDone(conn, True);
}

void session_die(SmcConn conn, SmPointer data) {
	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Received die\n");
	gaim_core_quit();
}

void session_save_complete(SmcConn conn, SmPointer data) {
	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Received save_complete\n");
}

void session_shutdown_cancelled(SmcConn conn, SmPointer data) {
	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Received shutdown_cancelled\n");
}

/* utility functions stolen from Gnome-client */

static void session_set_value(SmcConn conn, gchar *name, char *type,
	      int num_vals, SmPropValue *vals) {
	SmProp *proplist[1];
	SmProp prop;

	g_return_if_fail(conn);

	prop.name = name;
	prop.type = type;
	prop.num_vals = num_vals;
	prop.vals = vals;

	proplist[0] = &prop;
	SmcSetProperties(conn, 1, proplist);
}

static void session_set_string(SmcConn conn, gchar *name, gchar *value) {
	SmPropValue val;

	g_return_if_fail(name);

	val.length = strlen (value)+1;
	val.value  = value;

	session_set_value(conn, name, SmARRAY8, 1, &val);
}

static void session_set_gchar(SmcConn conn, gchar *name, gchar value) {
	SmPropValue val;

	g_return_if_fail(name);

	val.length = 1;
	val.value  = &value;

	session_set_value(conn, name, SmCARD8, 1, &val);
}

static void session_set_array(SmcConn conn, gchar *name, gchar *array[]) {
	gint    argc;
	gchar **ptr;
	gint    i;

	SmPropValue *vals;

	g_return_if_fail (name);

	/* We count the number of elements in our array.  */
	for (ptr = array, argc = 0; *ptr ; ptr++, argc++) /* LOOP */;

	/* Now initialize the 'vals' array.  */
	vals = g_new (SmPropValue, argc);
	for (ptr = array, i = 0 ; i < argc ; ptr++, i++) {
		vals[i].length = strlen (*ptr);
		vals[i].value  = *ptr;
	}

	session_set_value(conn, name, SmLISTofARRAY8, argc, vals);

	g_free (vals);
}

#endif /* USE_SM */

/* setup functions */

void session_init(gchar *argv0, gchar *previous_id) {
#ifdef USE_SM
	SmcCallbacks callbacks;
	gchar *client_id = NULL;
	gchar error[ERROR_LENGTH] = "";
	gchar *tmp = NULL;
	gchar **cmd = NULL;

	if (session != NULL) {
		/* session is already established, what the hell is going on? */
		gaim_debug(GAIM_DEBUG_WARNING, "Session Management",
				   "Duplicated call to session_init!\n");
		return;
	}

	if (g_getenv("SESSION_MANAGER") == NULL) {
		gaim_debug(GAIM_DEBUG_ERROR, "Session Management",
				   "No SESSION_MANAGER found, aborting.\n");
		return;
	}

	ice_init();

	callbacks.save_yourself.callback         = session_save_yourself;
	callbacks.die.callback                   = session_die;
	callbacks.save_complete.callback         = session_save_complete;
	callbacks.shutdown_cancelled.callback    = session_shutdown_cancelled;

	callbacks.save_yourself.client_data      = NULL;
	callbacks.die.client_data                = NULL;
	callbacks.save_complete.client_data      = NULL;
	callbacks.shutdown_cancelled.client_data = NULL;

	if (previous_id) {
		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "Connecting with previous ID %s\n", previous_id);
	} else {
		gaim_debug(GAIM_DEBUG_INFO, "Session Management",
				   "Connecting with no previous ID\n");
	}

	session = SmcOpenConnection(NULL, "session", SmProtoMajor, SmProtoMinor, SmcSaveYourselfProcMask |
		    SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask,
		    &callbacks, previous_id, &client_id, ERROR_LENGTH, error);

	if (session == NULL) {
		if (error[0] != '\0') {
			gaim_debug(GAIM_DEBUG_ERROR, "Session Management",
					   "Connection failed with error: %s\n", error);
		} else {
			gaim_debug(GAIM_DEBUG_ERROR, "Session Management",
					   "Connetion failed with unknown error.\n");
		}
		return;
	}

	tmp = SmcVendor(session);
	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Connected to manager (%s) with client ID %s\n",
			   tmp, client_id);
	g_free(tmp);

	session_managed = TRUE;
	gdk_set_sm_client_id(client_id);

	tmp = g_get_current_dir();
	session_set_string(session, SmCurrentDirectory, tmp);
	g_free(tmp);

	tmp = g_strdup_printf("%d", (int) getpid());
	session_set_string(session, SmProcessID, tmp);
	g_free(tmp);

	tmp = g_strdup(g_get_user_name());
	session_set_string(session, SmUserID, tmp);
	g_free(tmp);

	session_set_gchar(session, SmRestartStyleHint, (gchar) SmRestartIfRunning);
	session_set_string(session, SmProgram, g_get_prgname());

	myself = g_strdup(argv0);
	gaim_debug(GAIM_DEBUG_MISC, "Session Management",
			   "Using %s as command\n", myself);

	cmd = session_make_command(NULL);
	session_set_array(session, SmCloneCommand, cmd);
	g_strfreev(cmd);

	/* this is currently useless, but gnome-session warns 'the following applications will not
	   save their current status' bla bla if we don't have it and the user checks 'Save Session'
	   when they log out */
	cmd = g_new(gchar *, 2);
	cmd[0] = g_strdup("/bin/true");
	cmd[1] = NULL;
	session_set_array(session, SmDiscardCommand, cmd);
	g_strfreev(cmd);

	cmd = session_make_command(client_id);
	session_set_array(session, SmRestartCommand, cmd);
	g_strfreev(cmd);

	g_free(client_id);
#endif /* USE_SM */
}

void session_end() {
#ifdef USE_SM
	if (session == NULL) /* no session to close */
		return;

	SmcCloseConnection(session, 0, NULL);

	gaim_debug(GAIM_DEBUG_INFO, "Session Management",
			   "Connection closed.\n");
#endif /* USE_SM */
}
