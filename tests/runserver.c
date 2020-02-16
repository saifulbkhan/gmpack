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

#include "gmpackserver.h"
#include "testutils.h"

static guint16 port = 1500;
static gchar* error_string = "Error: unsuccessful request";
static guint32 timeout = 1000;

static GOptionEntry entries[] =
{
  { "port", 'p', 0, G_OPTION_ARG_INT,
    &port, "Port on which to listen", "N" },
  { "error-string", 'e', 0, G_OPTION_ARG_STRING,
    &error_string, "String used for replying to errored calls", NULL },
  { "timeout", 't', 0, G_OPTION_ARG_INT,
    &timeout, "Timeout for server auto-termination", NULL },
  { NULL }
};

static GVariant *
addition_handler (GList     *args,
                  gpointer   user_data,
                  GError   **error)
{
  GVariant *v1 = g_list_nth_data (args, 0);
  GVariant *v2 = g_list_nth_data (args, 1);
  if (g_variant_is_of_type (v1, G_VARIANT_TYPE_UINT32)
      && g_variant_is_of_type (v2, G_VARIANT_TYPE_INT32)) {
    guint32 first = g_variant_get_uint32 (v1);
    gint32 second = g_variant_get_int32 (v2);
    return g_variant_new_int32 (first + second);
  }
  return g_variant_new_string (error_string);
}

static gboolean
run_server ()
{
  GError *error = NULL;
  GmpackServer *server = gmpack_server_new ();

  gmpack_server_listen_at_port (server, port, &error);
  g_assert_no_error (error);

  gmpack_server_bind (server, "add", addition_handler, NULL, NULL);
  return FALSE;
}

static gboolean
exit_loop (gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_main_loop_quit (loop);
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  context = g_option_context_new ("- run dummy server for testing RPC calls");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_print ("Option parsing failed: %s\n", error->message);
    exit (1);
  }

  run_server();
  g_timeout_add (timeout, (GSourceFunc) exit_loop, loop);
  g_main_loop_run(loop);

  return 0;
}
