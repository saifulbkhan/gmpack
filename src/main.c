/* main.c
 *
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

#include "gmpackunpacker.h"
#include "gmpackpacker.h"
#include "gmpacksession.h"
#include "gmpackserver.h"

gint pack_unpack_test ()
{
  GError *error;
  GVariant *obj = NULL;
  gchar *str = NULL;
  gsize length = 46;
  const gchar *data = "\x92\x82\xa7\x63\x6f\x6d\x70\x61\x63\x74\xc3\xa6"
                      "\x73\x63\x68\x65\x6d\x61\x00\x82\xa0\xcf\xdc\xc8"
                      "\x0c\xd4\x00\x00\x00\x00\xa6\x6e\x65\x67\x20\x50"
                      "\x69\xcb\xc0\x09\x1e\xb8\x51\xeb\x85\x1f";
  GmpackUnpacker *unpacker = gmpack_unpacker_new ();
  obj = gmpack_unpacker_unpack_string (unpacker, &data, &length, &error);
  g_object_unref (unpacker);
  if (obj != NULL) {
    GmpackPacker *packer = gmpack_packer_new ();
    g_print ("Unpacked:\n");
    g_print ("%s\n", g_variant_print (obj, TRUE));

    error = NULL;
    length = gmpack_packer_pack_variant (packer, obj, &str, &error);
    g_object_unref (packer);
    if (length >= 0 && str != NULL) {
      int index;
      g_print ("Packed:\n");
      for (index = 0; index < length; ++index) {
        printf ("%hhx ", str[index]);
      }
      printf ("\n");
      return 0;
    }
  }
  g_print ("Error!\n");
  return 1;
}

static void
receive_cb (GObject      *source_object,
            GAsyncResult *res,
            gpointer      user_data)
{
  GmpackSession *session;
  GmpackMessage *message;
  GError *error = NULL;
  GBytes *bytes = user_data;

  g_bytes_unref (bytes);

  session = GMPACK_SESSION (source_object);
  message = gmpack_session_receive_finish (session, res, &error);

  if (!error) {
    if (gmpack_message_get_rpc_type (message)
        == GMPACK_MESSAGE_RPC_TYPE_REQUEST) {
      g_print ("Request received: ");
      g_printf ("%s | args = %s | id = %d\n",
                g_variant_print (gmpack_message_get_procedure (message),
                                 TRUE),
                g_variant_print (gmpack_message_get_args (message), TRUE),
                gmpack_message_get_rpc_id (message));
    } else if (gmpack_message_get_rpc_type (message)
               == GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION) {
      g_print ("Notification received: ");
      g_printf ("%s | args = %s\n",
                g_variant_print (gmpack_message_get_procedure (message),
                                 TRUE),
                g_variant_print (gmpack_message_get_args (message), TRUE));
    } else if (gmpack_message_get_rpc_type (message)
               == GMPACK_MESSAGE_RPC_TYPE_RESPONSE) {
      g_print ("Response received: ");
      g_printf ("%s | error = %s | data = %p\n",
                g_variant_print (gmpack_message_get_result (message), TRUE),
                g_variant_print (gmpack_message_get_error (message), TRUE),
                gmpack_message_get_data (message));
    }
  } else {
    g_printf ("Uh oh! Something went wrong while receiving.\n");
    g_error_free (error);
  }
  g_object_unref (message);
}

static gsize
print_bytes (GBytes *bytes)
{
  gsize size = 0;
  const gchar *arr = (const gchar *) g_bytes_get_data (bytes, &size);
  size_t index;
  for (index = 0; index < size; ++index)
    g_printf ("%hhx ", arr[index]);
  g_print ("\n");
  return size;
}

static void
send_cb (GObject      *source_object,
         GAsyncResult *res,
         gpointer      user_data)
{
  GmpackSession *session;
  GBytes *data;
  GError *error = NULL;
  const gchar *msg_type = (gchar *) user_data;

  session = GMPACK_SESSION (source_object);
  if (g_str_equal (msg_type, "Request")) {
    data = gmpack_session_request_finish (session, res, &error);
  } else if (g_str_equal (msg_type, "Notification")) {
    data = gmpack_session_notify_finish (session, res, &error);
  } else if (g_str_equal (msg_type, "Response")) {
    data = gmpack_session_respond_finish (session, res, &error);
  } else {
    g_print ("Unrecognized message type.");
    return;
  }

  if (!error && g_bytes_get_size (data) != 0) {
    g_printf ("%s sent: ", msg_type);
    print_bytes (data);
    gmpack_session_receive_async (session,
                                  data,
                                  0,
                                  NULL,
                                  NULL,
                                  receive_cb,
                                  data);
  } else {
    g_printf ("Uh oh! Something went wrong while sending.\n");
    g_error_free (error);
  }
}

static void
do_request (GmpackSession *session)
{
  GVariant *method = g_variant_new_parsed ("'REQUEST'");
  GVariant *arguments =
    g_variant_new_parsed ("[<-1>, <uint64 18446744073709551615>]");

  gmpack_session_request_async (session,
                                method,
                                arguments,
                                NULL,
                                NULL,
                                send_cb,
                                "Request");
}

static void
do_notify (GmpackSession *session)
{
  GVariant *method = g_variant_new_parsed ("'NOTIFY'");
  GVariant *arguments = g_variant_new_parsed ("[<'init'>, <'finished'>]");

  gmpack_session_notify_async (session,
                               method,
                               arguments,
                               NULL,
                               send_cb,
                               "Notification");
}

static void
do_respond (GmpackSession *session)
{
  GVariant *result = g_variant_new_parsed ("'unrecognized procedure'");

  gmpack_session_respond_async (session,
                                0,
                                result,
                                TRUE,
                                NULL,
                                send_cb,
                                "Response");
}

gint session_test ()
{
  GmpackSession* session = gmpack_session_new (NULL, NULL);

  do_request (session);
  do_notify (session);
  do_respond (session);

  return 0;
}

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

gboolean
client_request (GMemoryInputStream *istream)
{
  const gchar *data = "\x94\x00\x00\xa7\x52\x45\x51\x55\x45\x53"
                      "\x54\x92\xff\xcf\xff\xff\xff\xff\xff\xff"
                      "\xff\xff";
  g_memory_input_stream_add_data (istream, data, 22, NULL);
  return TRUE;
}

gboolean
client_notify (GMemoryInputStream *istream)
{
  const gchar *data = "\x93\x02\xa6\x4e\x4f\x54\x49\x46\x59\x92"
                      "\xa4\x69\x6e\x69\x74\xa8\x66\x69\x6e\x69"
                      "\x73\x68\x65\x64";
  g_memory_input_stream_add_data (istream, data, 24, NULL);
  return TRUE;
}

GMemoryInputStream *
server_test ()
{
  GmpackServer *server = gmpack_server_new ();
  GInputStream *istream = g_memory_input_stream_new ();
  GIOStream *iostream = g_simple_io_stream_new (istream, NULL);
  gmpack_server_bind (server, "REQUEST", request_handler, NULL, NULL);
  gmpack_server_bind (server, "NOTIFY", notification_handler, NULL, NULL);
  gmpack_server_accept_io_stream (server, iostream, NULL);
  return (GMemoryInputStream *)istream;
}

gboolean
exit_loop (gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_main_loop_quit (loop);
  return TRUE;
}

gint main (gint   argc,
           gchar *argv[])
{
  gint status = pack_unpack_test () | session_test ();
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  GMemoryInputStream *istream = server_test ();
  g_timeout_add (500, (GSourceFunc) client_notify, istream);
  g_timeout_add (1000, (GSourceFunc) client_request, istream);
  g_timeout_add (5005, (GSourceFunc) exit_loop, loop);
  g_main_loop_run(loop);
  return status;
}
