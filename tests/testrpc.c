/*
 * Copyright 2019 Saiful B. Khan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SERVER_EXECUTABLE "run-server"
#define TCP_PORT 1500
#define ERROR_STRING "Error: illegal addition."
#define SERVER_TIMEOUT 50

#include "gmpackclient.h"
#include "testutils.h"

static gint callbacks_due = 0;

static void
child_watch_cb (GPid     pid,
                gint     status,
                gpointer user_data)
{
  GMainLoop *loop = user_data;

  if (g_spawn_check_exit_status (status, NULL)) {
    g_message ("Server process %" G_PID_FORMAT " exited normally", pid);
  } else {
    g_error ("Server process %" G_PID_FORMAT " exited abnormally", pid);
    exit (1);
  }

  g_spawn_close_pid (pid);

  g_main_loop_quit (loop);
  if (callbacks_due != 0) {
    g_error ("Failed to receive responses for some (%d) calls\n", callbacks_due);
    exit (1);
  }
  g_print ("Success! All RPC calls accounted for\n");
}

static gboolean
spawn_server (const gchar* server_path, GMainLoop *loop)
{
  const gchar * const argv[] = {
    server_path,
    "--port", g_strdup_printf ("%d", TCP_PORT),
    "--error-string", ERROR_STRING,
    "--timeout", g_strdup_printf ("%d", SERVER_TIMEOUT),
  };
  gint child_stdout, child_stderr;
  GPid child_pid;
  g_autoptr(GError) error = NULL;

  /* spawn server as child process */
  g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                            NULL, &child_pid, NULL, &child_stdout,
                            &child_stderr, &error);
  if (error != NULL) {
      g_error ("Spawning server failed: %s", error->message);
      return FALSE;
    }

  /* add a child watch function, called when the child process exits */
  g_child_watch_add (child_pid, child_watch_cb, loop);

  return TRUE;
}

static void
client_request_cb (GObject      *object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  GError *error = NULL;
  GPtrArray *values = user_data;
  GVariant *expected = NULL;
  GVariant *actual = NULL;
  GmpackClient *client = GMPACK_CLIENT (object);

  gmpack_client_request_finish (client, result, &error);
  g_assert_no_error (error);

  expected = g_ptr_array_index (values, 0);
  actual = *(GVariant **)(g_ptr_array_index (values, 1));
  g_assert_cmpvariant (expected, actual);

  g_variant_unref (expected);
  g_variant_unref (actual);
  g_ptr_array_unref (values);
  g_object_unref (client);

  callbacks_due -= 1;
}

static void
client_request (const gchar *method,
                GList       *args,
                GVariant    *expected_result,
                gboolean     should_error)
{
  gboolean success = FALSE;
  g_autoptr (GError) error = NULL;
  g_autoptr (GPtrArray) values = NULL;
  GVariant *actual_result = NULL;
  g_autoptr (GmpackClient) client = gmpack_client_new_for_tcp ("localhost",
                                                               1500);

  /* test synchronous request */
  success = gmpack_client_request (client,
                                   "add",
                                   args,
                                   &actual_result,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_false (success == should_error);
  g_assert_cmpvariant (expected_result, actual_result);
  g_variant_unref (actual_result);
  actual_result = NULL;

  values = g_ptr_array_new ();
  g_ptr_array_add (values, expected_result);
  g_ptr_array_add (values, &actual_result);

  /* test asynchronous request */
  gmpack_client_request_async (g_object_ref (client),
                               "add",
                               args,
                               &actual_result,
                               NULL,
                               client_request_cb,
                               g_ptr_array_ref (values));
  callbacks_due += 1;
}

static gboolean
client_request_proper ()
{
  GVariant *arg = NULL;
  GList *args = NULL;

  arg = g_variant_new_parsed ("uint32 1");
  args = g_list_append (args, g_variant_ref (arg));
  arg = g_variant_new_parsed ("int32 -1");
  args = g_list_append (args, g_variant_ref (arg));

  client_request ("add", args, g_variant_new_parsed ("uint32 0"), FALSE);

  g_list_free_full (args, (GDestroyNotify) g_variant_unref);
  return FALSE;
}

static gboolean
client_request_improper ()
{
  GVariant *arg = NULL;
  GList *args = NULL;

  arg = g_variant_new_parsed ("@mv nothing");
  args = g_list_prepend (args, g_variant_ref (arg));
  arg = g_variant_new_parsed ("@ay []");
  args = g_list_prepend (args, g_variant_ref (arg));

  client_request ("add", args, g_variant_new_string (ERROR_STRING), TRUE);

  g_list_free_full (args, (GDestroyNotify) g_variant_unref);
  return FALSE;
}

static gboolean
client_notify ()
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GmpackClient) client = gmpack_client_new_for_tcp ("localhost",
                                                               1500);

  gmpack_client_notify (client,
                        "event-happened",
                        NULL,
                        NULL,
                        &error);
  g_assert_no_error (error);
  return FALSE;
}

int
main (int argc, char *argv[])
{
  gchar *this_path = argv[0];
  gchar *server_path;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  /* assuming server exec is available in the same dir as this exec */
  server_path = g_build_path ("/",
                              g_path_get_dirname (this_path),
                              SERVER_EXECUTABLE,
                              NULL);
  if (!spawn_server (server_path, loop)) {
    g_main_loop_unref (loop);
    return 1;
  }

  g_idle_add ((GSourceFunc) client_request_proper, NULL);
  g_idle_add ((GSourceFunc) client_request_improper, NULL);
  g_idle_add ((GSourceFunc) client_notify, NULL);

  g_main_loop_run(loop);

  return 0;
}
