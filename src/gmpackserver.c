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

#define DEFAULT_TCP_PORT 1000
#define SINGLE_READ_COUNT 1024
#define FIRST_HANDLER_ID 1

#include <glib/gprintf.h>

#include "common.h"
#include "gmpackserver.h"
#include "gmpacksession.h"

typedef struct {
  GByteArray    *pending_buffer;
  gint16         priority;
  GmpackSession *session;
} ReadContext;

static void
read_context_free (gpointer data)
{
  ReadContext *context = data;
  g_clear_object (&context->session);
  g_slice_free (ReadContext, context);
}

typedef struct {
  GmpackServerHandler  handler;
  gpointer             user_data;
  GDestroyNotify       user_data_destroy;
} MethodData;

static void
method_data_free (gpointer data)
{
  MethodData *method_data = data;
  if (method_data->user_data != NULL
      && method_data->user_data_destroy != NULL) {
    method_data->user_data_destroy (method_data->user_data);
  }
  g_slice_free (MethodData, method_data);
}

typedef struct {
  MethodData           *method_data;
  GList                *args;
  gint                  rpc_id;
  GmpackMessageRpcType  rpc_type;
  GInputStream         *istream;
} RpcData;

static void
rpc_data_free (gpointer data)
{
  RpcData *rpc_data = data;
  g_list_free_full (rpc_data->args, (GDestroyNotify) g_variant_unref);
  g_slice_free (RpcData, rpc_data);
}

struct _GmpackServer
{
  GObject         parent_instance;
  GHashTable     *connected_io_streams;
  GHashTable     *io_sessions;
  GSocketService *tcp_service;
  guint16         tcp_port;
  GHashTable     *bound_methods;
  GHashTable     *bound_method_data;
  guint           next_handler_id;
};

G_DEFINE_TYPE (GmpackServer, gmpack_server, G_TYPE_OBJECT)

static void
gmpack_server_class_init (GmpackServerClass *klass)
{
}

static void
gmpack_server_init (GmpackServer *self)
{
  self->connected_io_streams = g_hash_table_new (g_direct_hash,
                                                 g_direct_equal);
  self->io_sessions = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);
  self->tcp_service = NULL;
  self->tcp_port = DEFAULT_TCP_PORT;
  self->bound_methods = g_hash_table_new_full (g_int64_hash,
                                               g_int64_equal,
                                               g_free,
                                               g_free);
  self->bound_method_data = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   method_data_free);
  self->next_handler_id = FIRST_HANDLER_ID;
}

static void
gmpack_server_finalize (GObject *object)
{
  GmpackServer *self = GMPACK_SERVER (object);
  g_hash_table_destroy (self->connected_io_streams);
  g_hash_table_destroy (self->io_sessions);
  g_hash_table_destroy (self->bound_methods);
  g_hash_table_destroy (self->bound_method_data);
  g_clear_object (&self->tcp_service);
  G_OBJECT_CLASS (gmpack_server_parent_class)->finalize (object);
}

GmpackServer *
gmpack_server_new ()
{
  GmpackServer *server = g_object_new (GMPACK_SERVER_TYPE, NULL);
  return server;
}

static void
handle_call_thread (GTask         *task,
                    gpointer       source_object,
                    gpointer       task_data,
                    GCancellable  *cancellable)
{
  GError *error = NULL;
  GVariant *result = NULL;
  GBytes *to_write = NULL;
  GmpackServer *self = source_object;
  RpcData *rpc_data = task_data;
  MethodData *method_data = rpc_data->method_data;
  GmpackSession *session = NULL;

  g_assert (GMPACK_IS_SERVER (self));
  g_assert (rpc_data != NULL);
  g_assert (method_data != NULL);

  session = g_hash_table_lookup (self->io_sessions, rpc_data->istream);
  g_assert (session != NULL);

  result = rpc_data->method_data->handler (rpc_data->args,
                                           method_data->user_data,
                                           &error);

  if (!error && rpc_data->rpc_type == GMPACK_MESSAGE_RPC_TYPE_REQUEST) {
    to_write = gmpack_session_respond (session,
                                       rpc_data->rpc_id,
                                       result,
                                       FALSE,
                                       &error);
  }

  if (!error) {
    g_task_return_pointer (task, result, (GDestroyNotify) g_variant_unref);
  } else {
    g_task_return_error (task, error);
  }

  if (to_write != NULL) {
    GOutputStream *ostream = NULL;

    ostream = g_hash_table_lookup (self->connected_io_streams,
                                   rpc_data->istream);
    g_return_if_fail (ostream != NULL);

    g_output_stream_write_bytes_async (ostream,
                                       to_write,
                                       G_PRIORITY_LOW,
                                       NULL,
                                       NULL,
                                       NULL);
  }
}

void
handle_call_async (GmpackServer         *self,
                   RpcData              *rpc_data,
                   GCancellable         *cancellable,
                   GAsyncReadyCallback   callback,
                   gpointer              user_data)
{
  g_autoptr(GTask) task = NULL;

  g_return_if_fail (rpc_data->method_data->handler != NULL);
  g_assert (G_IS_INPUT_STREAM (rpc_data->istream));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, rpc_data, rpc_data_free);
  g_task_run_in_thread (task, handle_call_thread);
}

GVariant *
handle_call_finish (GmpackServer  *self,
                    GAsyncResult  *result,
                    GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static RpcData *
rpc_data_from_message (GmpackServer *self,
                       GmpackMessage *message)
{
  gint rpc_id;
  gchar *method;
  GList *args = NULL;
  GVariant *var = NULL;
  GVariant *arg = NULL;
  GVariantIter iter;
  GmpackMessageRpcType rpc_type;
  MethodData *method_data = NULL;
  RpcData *rpc_data = NULL;

  rpc_type = gmpack_message_get_rpc_type (message);
  if (rpc_type != GMPACK_MESSAGE_RPC_TYPE_REQUEST
      && rpc_type != GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION) {
    return rpc_data;
  }

  var = gmpack_message_get_procedure (message);
  method = g_variant_dup_string (var, NULL);
  method_data = g_hash_table_lookup (self->bound_method_data, method);
  if (method_data == NULL) {
    g_warning ("Received unregistered method \"%s\".", method);
    return rpc_data;
  }

  var = gmpack_message_get_args (message);
  if (!g_str_equal(g_variant_get_type_string (var), "av")) {
    g_warning ("Argument for method \"%s\" is of type \"%s\" "
               "(expected \"av\").",
               method,
               g_variant_get_type_string (var));
    return rpc_data;
  }

  g_variant_iter_init (&iter, var);
  while (g_variant_iter_next (&iter, "v", &arg))
    args = g_list_append (args, arg);

  rpc_id = gmpack_message_get_rpc_id (message);

  rpc_data = g_slice_new0 (RpcData);
  rpc_data->method_data = method_data;
  rpc_data->args = args;
  rpc_data->rpc_id = rpc_id;
  rpc_data->rpc_type = rpc_type;

  g_free (method);
  return rpc_data;
}

static void
read_cb (GObject      *object,
         GAsyncResult *result,
         gpointer      user_data)
{
  GInputStream *istream = (GInputStream *)object;
  GCancellable *cancellable = NULL;
  g_autoptr(GError) error = NULL;
  GBytes *bytes = NULL;
  GQueue *messages = NULL;
  gsize parsed_length = 0;
  GmpackSession *session = NULL;
  ReadContext *context = NULL;
  g_autoptr(GTask) task = user_data;

  g_assert (G_IS_INPUT_STREAM (istream));
  g_assert (G_IS_TASK (task));

  context = g_task_get_task_data (task);
  cancellable = g_task_get_cancellable (task);

  session = context->session;

  bytes = g_input_stream_read_bytes_finish (istream, result, &error);
  if (bytes == NULL) {
    if (error != NULL) {
      g_task_return_error (task, g_steal_pointer (&error));
    } else {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               "No data to read from peer");
    }
    return;
  }

  /* append to any previously pending buffer */
  if (context->pending_buffer != NULL) {
    gsize size = 0;
    gpointer data = g_bytes_unref_to_data (bytes, &size);
    g_byte_array_append (context->pending_buffer, data, size);
    bytes = g_byte_array_free_to_bytes (context->pending_buffer);
    context->pending_buffer = NULL;
  }

  if (bytes != NULL && g_bytes_get_size (bytes) != 0) {
    gsize current_index = 0;
    GmpackMessage *message = NULL;

    messages = g_queue_new ();

    while (parsed_length < g_bytes_get_size (bytes)) {
      message = gmpack_session_receive (session,
                                        bytes,
                                        current_index,
                                        &parsed_length,
                                        &error);
      if (error != NULL && error->code == GMPACK_UNPACKER_ERROR_EOF) {
        if (parsed_length < g_bytes_get_size (bytes)) {
          /* the data sent was incomplete and the rest of it is not
           * a part of the same message, so we discard the parsed bits
           */
          GByteArray *data = g_bytes_unref_to_array (bytes);
          g_byte_array_remove_range (data, current_index, parsed_length);
          bytes = g_byte_array_free_to_bytes (data);
        } else {
          /* the data send was incomplete and further reads might result
           * in a more complete message
           * store already read buffer and scehdule another read to obtain
           * the rest of it
           */
          context->pending_buffer = g_bytes_unref_to_array (bytes);
          g_byte_array_remove_range (context->pending_buffer,
                                     0,
                                     current_index);
          g_input_stream_read_bytes_async (istream,
                                           SINGLE_READ_COUNT,
                                           context->priority,
                                           cancellable,
                                           read_cb,
                                           g_steal_pointer (&task));
          return;
        }
      } else if (error != NULL) {
        g_task_return_error (task, g_steal_pointer (&error));
        g_bytes_unref (bytes);
        return;
      } else {
        g_queue_push_tail (messages, message);
      }
      current_index = parsed_length;
    }
  }

  if (bytes != NULL)
    g_bytes_unref (bytes);

  g_task_return_pointer (task, messages, g_object_unref);
}

void
read_async (GmpackServer        *self,
            GInputStream        *istream,
            GCancellable        *cancellable,
            GAsyncReadyCallback  callback,
            gpointer             user_data)
{
  GmpackSession *session;
  ReadContext *context = NULL;
  g_autoptr(GTask) task = NULL;

  session = g_hash_table_lookup (self->io_sessions, istream);
  g_assert (GMPACK_IS_SESSION (session));

  context = g_slice_new0 (ReadContext);
  context->priority = G_PRIORITY_LOW;
  context->pending_buffer= NULL;
  context->session = g_object_ref (session);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, context, read_context_free);
  g_task_set_priority (task, context->priority);

  g_input_stream_read_bytes_async (istream,
                                   SINGLE_READ_COUNT,
                                   context->priority,
                                   cancellable,
                                   read_cb,
                                   g_steal_pointer (&task));
}

GQueue *
read_finish (GmpackServer  *self,
             GAsyncResult   *result,
             GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
listen_cb (GObject      *object,
           GAsyncResult *result,
           gpointer      user_data)
{
  GQueue *messages = NULL;
  GError *error = NULL;
  GInputStream *istream = user_data;
  GmpackServer *self = (GmpackServer *)object;

  g_assert (G_IS_INPUT_STREAM (istream));
  g_assert (GMPACK_IS_SERVER (self));

  messages = read_finish (self, result, &error);
  if (messages != NULL) {
    while (g_queue_get_length (messages) > 0) {
      GmpackMessage *message = NULL;
      RpcData *rpc_data = NULL;

      message = g_queue_pop_tail (messages);
      g_assert (GMPACK_IS_MESSAGE (message));

      rpc_data = rpc_data_from_message (self, message);
      if (rpc_data == NULL)
        continue;

      rpc_data->istream = istream;
      handle_call_async (self,
                         rpc_data,
                         NULL,
                         NULL,
                         NULL);
      g_object_unref (message);
    }
    g_queue_free (messages);
  }
  read_async (self, istream, NULL, listen_cb, istream);
}

void
gmpack_server_accept_io_stream (GmpackServer  *self,
                                GIOStream     *iostream,
                                GError       **error)
{
  GInputStream *istream = NULL;
  GOutputStream *ostream = NULL;
  GmpackSession *session = NULL;

  g_return_if_fail (GMPACK_IS_SERVER (self));

  istream = g_io_stream_get_input_stream (iostream);
  ostream = g_io_stream_get_output_stream (iostream);
  session = gmpack_session_new ();

  g_return_if_fail (
    g_hash_table_lookup (self->connected_io_streams, iostream) == NULL);

  g_hash_table_insert (self->connected_io_streams, istream, ostream);
  g_hash_table_insert (self->io_sessions, istream, session);
  read_async (self, istream, NULL, listen_cb, istream);
}

static gboolean
incoming_cb (GSocketService    *server,
             GSocketConnection *connection,
             GObject           *source_object,
             gpointer           user_data)
{
  GError *error = NULL;
  GmpackServer *self = user_data;

  g_assert (GMPACK_IS_SERVER (self));

  gmpack_server_accept_io_stream (self,
                                  G_IO_STREAM (connection),
                                  &error);
  if (error != NULL) {
    g_error ("While listening to incoming connection: %s", error->message);
    return FALSE;
  }

  return TRUE;
}

void
gmpack_server_listen_at_port (GmpackServer  *self,
                              guint16        port,
                              GError       **error)
{
  GSocketService *service = NULL;

  if (self->tcp_service != NULL) {
    g_warning ("Already listening at port %d. Please use "
               "gmpack_server_stop_listening to disconnect "
               "from existing TCP service, before calling "
               "gmpack_server_listen_at_port.", self->tcp_port);
  }

  service = g_socket_service_new ();
  g_socket_listener_add_inet_port ((GSocketListener*) service,
                                    port,
                                    NULL,
                                    error);
  if (error != NULL)
    return;

  self->tcp_service = service;
  self->tcp_port = port;

  g_signal_connect (self->tcp_service,
                    "incoming",
                    G_CALLBACK (incoming_cb),
                    self);
}

void
gmpack_server_stop_listening (GmpackServer *self)
{
  g_return_if_fail (self->tcp_service != NULL);

  g_socket_service_stop (self->tcp_service);
  g_socket_listener_close ((GSocketListener *)self->tcp_service);
  g_object_unref (self->tcp_service);

  self->tcp_service = NULL;
  self->tcp_port = DEFAULT_TCP_PORT;
}

guint16
gmpack_server_get_port (GmpackServer *self)
{
  return self->tcp_port;
}

guint
gmpack_server_bind (GmpackServer        *self,
                    const gchar         *method,
                    GmpackServerHandler  handler,
                    gpointer             user_data,
                    GDestroyNotify       user_data_destroy)
{
  MethodData *method_data = NULL;
  guint *handler_id = g_new0 (guint, 1);

  /* if the method has already been registered, we update its data */
  method_data = g_hash_table_lookup (self->bound_method_data, method);
  if (method_data != NULL) {
    method_data->handler = handler;
    method_data->user_data = user_data;
    method_data->user_data_destroy = user_data_destroy;

    *handler_id = FIRST_HANDLER_ID;
    while (*handler_id < self->next_handler_id) {
      gchar *registered = g_hash_table_lookup (self->bound_methods, handler_id);
      if (g_str_equal(method, registered))
        return *handler_id;
    }
    g_assert_not_reached ();
    return 0;
  }

  *handler_id = self->next_handler_id++;
  g_hash_table_insert (self->bound_methods, handler_id, g_strdup (method));

  method_data = g_slice_new0 (MethodData);
  method_data->handler = handler;
  method_data->user_data = user_data;
  method_data->user_data_destroy = user_data_destroy;
  g_hash_table_insert (self->bound_method_data,
                       g_strdup (method),
                       method_data);

  return *handler_id;
}

void
gmpack_server_unbind (GmpackServer *self, guint bound_id)
{
  gchar *method = NULL;

  method = g_hash_table_lookup (self->bound_methods, &bound_id);
  g_hash_table_remove (self->bound_method_data, method);
  g_hash_table_remove (self->bound_methods, &bound_id);
}
