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

#include "gmpack-config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include "gmpacksession.h"
#include "gmpackserver.h"

GVariant *
request_handler (GList     *args,
                 gpointer   user_data,
                 GError   **error)
{
  gint64 first = g_variant_get_int64 (args->data);
  guint64 second = g_variant_get_uint64 (args->next->data);
  return g_variant_new_boolean (first == second);
}

GVariant *
notification_handler (GList     *args,
                      gpointer   user_data,
                      GError   **error)
{
  const gchar *first = g_variant_get_string (args->data, NULL);
  const gchar *second = g_variant_get_string (args->next->data, NULL);
  g_print ("%s: %s\n", first, second);
  return NULL;
}

void
run_server ()
{
  GError *error = NULL;
  GmpackServer *server = gmpack_server_new ();

  gmpack_server_listen_at_port (server, 1500, &error);
  if (error != NULL)
    g_print ("%s\n", error->message);

  gmpack_server_bind (server, "REQUEST", request_handler, NULL, NULL);
  gmpack_server_bind (server, "NOTIFY", notification_handler, NULL, NULL);
}

gint main (gint   argc,
           gchar *argv[])
{
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  run_server ();
  g_main_loop_run(loop);
  return TRUE;
}
