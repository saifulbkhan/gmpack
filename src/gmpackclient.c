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

#include <glib/gprintf.h>

#include "common.h"
#include "gmpackclient.h"

typedef struct {
  guint32    request_id;
  GVariant **result;
  GObject   *client;
} RequestData;

static void
request_data_free (RequestData *request_data)
{
  g_slice_free (RequestData, request_data);
}

struct _GmpackClient
{
  GObject        parent_instance;
  GmpackSession *session;
  GIOStream     *iostream;
  GHashTable    *pending_tasks;
  gboolean       listening_async;
};

G_DEFINE_TYPE (GmpackClient, gmpack_client, G_TYPE_OBJECT)

static void
gmpack_client_class_init (GmpackClientClass *klass)
{
}

static void
gmpack_client_init (GmpackClient *self)
{
  self->session = gmpack_session_new ();
}

static void
gmpack_client_finalize (GObject *object)
{
  GmpackClient *self = GMPACK_CLIENT (object);

  g_hash_table_destroy (self->pending_tasks);
  g_io_stream_close (self->iostream, NULL, NULL);
  g_object_unref (self->session);

  G_OBJECT_CLASS (gmpack_client_parent_class)->finalize (object);
}

gboolean
is_nothing (GVariant *variant)
{
  if (g_variant_is_of_type (variant, G_VARIANT_TYPE_MAYBE)
      && g_variant_get_maybe (variant) == NULL) {
        return TRUE;
  }
  return FALSE;
}

static void
listen_cb (GObject      *object,
           GAsyncResult *result,
           gpointer      user_data)
{
  GQueue *messages = NULL;
  GError *error = NULL;
  GInputStream *istream = user_data;
  GmpackClient *self = (GmpackClient *)object;

  g_assert (G_IS_INPUT_STREAM (istream));
  g_assert (GMPACK_IS_CLIENT (self));

  messages = gmpack_read_istream_finish (G_OBJECT (self), result, &error);
  if (messages != NULL) {
    while (g_queue_get_length (messages) > 0) {
      guint32 request_id;
      GTask *task;
      GmpackMessage *message = NULL;
      RequestData *request_data = NULL;

      message = g_queue_pop_tail (messages);
      g_assert (GMPACK_IS_MESSAGE (message));

      request_id = gmpack_message_get_rpc_id (message);
      task = g_hash_table_lookup (self->pending_tasks, &request_id);
      if (task == NULL) {
        g_warning ("Received result for unexpected request.");
        g_object_unref (message);
        continue;
      }

      request_data = g_task_get_task_data (task);
      if (is_nothing (gmpack_message_get_error (message))) {
        *(request_data->result) =
          g_variant_ref (gmpack_message_get_result (message));
        g_task_return_boolean (task, TRUE);
      } else {
        *(request_data->result) =
          g_variant_ref (gmpack_message_get_error (message));
        g_task_return_boolean (task, FALSE);
      }
      g_object_unref (message);
    }
    g_queue_free (messages);
  }

  gmpack_read_istream_async (G_OBJECT (self),
                             istream,
                             self->session,
                             NULL,
                             listen_cb,
                             istream);
}

void
gmpack_client_start_async_read (GmpackClient  *self)
{
  GInputStream *istream = NULL;

  istream = g_io_stream_get_input_stream (self->iostream);
  gmpack_read_istream_async (G_OBJECT (self),
                             istream,
                             self->session,
                             NULL,
                             listen_cb,
                             istream);
}

GmpackClient *
gmpack_client_new (GIOStream *iostream)
{
  GmpackClient *client = g_object_new (GMPACK_CLIENT_TYPE, NULL);

  client->iostream = iostream;
  client->pending_tasks = g_hash_table_new_full (g_int_hash,
                                                 g_int_equal,
                                                 g_free,
                                                 g_object_unref);
  client->listening_async = FALSE;
  return client;
}

GmpackClient *
gmpack_client_new_for_tcp (const gchar *address,
                           guint        port)
{
  GError *error = NULL;
  GSocketConnection *connection = NULL;
  GmpackClient *client = NULL;

  connection = g_socket_client_connect_to_host (g_socket_client_new(),
                                                address,
                                                port,
                                                NULL,
                                                &error);
  if (error != NULL) {
    g_error ("Could not establish a connection with host: %s", error->message);
    return NULL;
  }

  client = gmpack_client_new (G_IO_STREAM (connection));
  return client;
}

GVariant *
build_args_array (GList *args)
{
  GVariant *args_array = NULL;
  GVariantBuilder *builder = NULL;
  GList *l;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("av"));
  for (l = args; l != NULL; l = l->next)
    g_variant_builder_add (builder, "v", l->data);
  args_array = g_variant_new ("av", builder);
  g_variant_builder_unref (builder);

  return args_array;
}

gboolean
gmpack_client_request (GmpackClient  *self,
                       const gchar   *method,
                       GList         *args,
                       GVariant     **result,
                       gpointer       user_data,
                       GCancellable  *cancellable,
                       GError       **error)
{
  guint32 request_id;
  GBytes *data;
  GOutputStream *ostream = NULL;
  GInputStream *istream = NULL;
  GVariant *args_var = NULL;
  GmpackSession *session = NULL;
  GmpackMessage *response = NULL;

  g_assert (GMPACK_IS_CLIENT (self));

  *result = NULL;
  g_return_val_if_fail (self->listening_async == FALSE, FALSE);

  session = self->session;
  args_var = build_args_array (args);
  data = gmpack_session_request (session,
                                 g_variant_new_string (method),
                                 args_var,
                                 user_data,
                                 &request_id,
                                 error);
  g_return_val_if_fail (*error == NULL, FALSE);

  ostream = g_io_stream_get_output_stream (self->iostream);
  g_output_stream_write_bytes (ostream, data, cancellable, error);
  g_return_val_if_fail (*error == NULL, FALSE);

  istream = g_io_stream_get_input_stream (self->iostream);
  response = gmpack_read_istream (istream, session, error);
  g_return_val_if_fail (*error == NULL, FALSE);

  if (is_nothing (gmpack_message_get_error (response))) {
    *result = gmpack_message_get_result (response);
    return TRUE;
  }

  *result = gmpack_message_get_error (response);
  return FALSE;
}

static void
write_bytes_cb (GObject      *object,
                GAsyncResult *result,
                gpointer      user_data)
{
  GError *error = NULL;
  GOutputStream *ostream = G_OUTPUT_STREAM (object);
  GmpackClient *self = NULL;
  GTask *task = user_data;
  RequestData *request_data = NULL;

  g_assert (G_IS_OUTPUT_STREAM (ostream));
  g_assert (G_IS_TASK (task));

  request_data = g_task_get_task_data (task);
  self = GMPACK_CLIENT (request_data->client);

  g_assert (GMPACK_IS_CLIENT (self));

  g_output_stream_write_bytes_finish (ostream, result, &error);
  if (error != NULL)
    g_task_return_error (task, error);

  g_object_unref (task);
}

static void
session_request_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  guint32 *request_id = g_new0 (guint32, 1);
  GError *error = NULL;
  GBytes *data = NULL;
  GOutputStream *ostream = NULL;
  GmpackSession *session = GMPACK_SESSION (object);
  GmpackClient *self = NULL;
  GTask *task = user_data;
  RequestData *request_data = NULL;

  g_assert (GMPACK_IS_SESSION (session));
  g_assert (G_IS_TASK (task));

  request_data = g_task_get_task_data (task);
  self = GMPACK_CLIENT (request_data->client);

  g_assert (GMPACK_IS_CLIENT (self));

  data = gmpack_session_request_finish (session, result, &error);
  if (error != NULL)
    g_task_return_error (task, error);

  *request_id = request_data->request_id;
  g_hash_table_insert (self->pending_tasks, request_id, g_object_ref (task));

  ostream = g_io_stream_get_output_stream (self->iostream);
  g_output_stream_write_bytes_async (ostream,
                                     data,
                                     G_PRIORITY_LOW,
                                     g_task_get_cancellable (task),
                                     write_bytes_cb,
                                     task);
}

void gmpack_client_request_async (GmpackClient         *self,
                                  const gchar          *method,
                                  GList                *args,
                                  GVariant            **result,
                                  GCancellable         *cancellable,
                                  GAsyncReadyCallback   callback,
                                  gpointer              user_data)
{
  GTask *task;
  GVariant *args_var = NULL;
  RequestData *request_data = NULL;

  g_assert (GMPACK_IS_CLIENT (self));

  gmpack_client_start_async_read (self);
  self->listening_async = TRUE;

  *result = NULL;
  args_var = build_args_array (args);

  request_data = g_slice_new0 (RequestData);
  request_data->request_id = -1;
  request_data->result = result;
  request_data->client = G_OBJECT (self);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_priority (task, G_PRIORITY_LOW);
  g_task_set_task_data (task,
                        request_data,
                        (GDestroyNotify) request_data_free);

  gmpack_session_request_async (self->session,
                                g_variant_new_string (method),
                                args_var,
                                NULL,
                                &(request_data->request_id),
                                cancellable,
                                session_request_cb,
                                task);
}

gboolean gmpack_client_request_finish (GmpackClient  *self,
                                       GAsyncResult  *result,
                                       GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
session_notify_cb (GObject      *object,
                   GAsyncResult *result,
                   gpointer      user_data)
{
  GError *error = NULL;
  GBytes *data = NULL;
  GOutputStream *ostream = NULL;
  GmpackSession *session = GMPACK_SESSION (object);
  GmpackClient *self = user_data;

  data = gmpack_session_notify_finish (session, result, &error);
  g_return_if_fail (error == NULL);

  ostream = g_io_stream_get_output_stream (self->iostream);
  g_output_stream_write_bytes_async (ostream,
                                     data,
                                     G_PRIORITY_LOW,
                                     NULL,
                                     NULL,
                                     NULL);
}

void
gmpack_client_notify (GmpackClient  *self,
                      const gchar   *method,
                      GList         *args,
                      GCancellable  *cancellable,
                      GError       **error)
{
  GVariant *args_var = NULL;
  GmpackSession *session = NULL;

  g_assert (GMPACK_IS_CLIENT (self));

  session = self->session;
  args_var = build_args_array (args);
  gmpack_session_notify_async (session,
                               g_variant_new_string (method),
                               args_var,
                               cancellable,
                               session_notify_cb,
                               self);
}
