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
#include "gmpacksession.h"
#include "mpack.h"

typedef struct {
  GBytes *data;
  gsize   start_pos;
  gsize  *stop_pos;
} ReceiveData;

struct _GmpackSession
{
  GObject              parent_instance;
  mpack_rpc_session_t *session;
};

G_DEFINE_TYPE (GmpackSession, gmpack_session, G_TYPE_OBJECT)

static void
gmpack_session_class_init (GmpackSessionClass *klass)
{
}

static void
gmpack_session_init (GmpackSession *self)
{
  self->session = g_malloc (sizeof (*self->session));
  if (!self->session) {
    g_error ("Failed to allocate memory for session.\n");
  }

  mpack_rpc_session_init(self->session, 0);
}

static void
gmpack_session_finalize (GObject *object)
{
  GmpackSession *self = GMPACK_SESSION (object);
  g_free (self->session);
  G_OBJECT_CLASS (gmpack_session_parent_class)->finalize (object);
}

GmpackSession *
gmpack_session_new ()
{
  GmpackSession *session = g_object_new (GMPACK_SESSION_TYPE, NULL);
  return session;
}

GQuark
gmpack_session_error_quark (void)
{
  return g_quark_from_static_string ("gmpack-session-error-quark");
}

static mpack_rpc_session_t *session_grow(mpack_rpc_session_t *session)
{
  mpack_rpc_session_t *old = session;
  mpack_uint32_t new_capacity = old->capacity * 2;
  session = malloc (MPACK_RPC_SESSION_STRUCT_SIZE (new_capacity));
  if (!session) goto end;
  mpack_rpc_session_init (session, new_capacity);
  mpack_rpc_session_copy (session, old);
  free (old);
end:
  return session;
}

GmpackMessage *
gmpack_session_receive (GmpackSession  *self,
                        GBytes         *data,
                        gsize           start_pos,
                        gsize          *stop_pos,
                        GError        **error)
{
  GmpackMessage *message = gmpack_message_new ();
  GmpackUnpacker *unpacker = gmpack_unpacker_new ();
  GVariant *proc_or_error = NULL;
  GVariant *args_or_result = NULL;
  gsize length;
  const gchar *buffer_init = g_bytes_get_data (data, &length);
  const gchar *buffer = buffer_init + start_pos;
  gsize buffer_length = length - start_pos;
  gboolean done = FALSE;
  gint message_type = MPACK_EOF;
  mpack_rpc_message_t rpc_message;

  error = NULL;

  if (start_pos >= length) {
    g_set_error (error,
                 GMPACK_SESSION_ERROR,
                 GMPACK_SESSION_ERROR_IMPROPER,
                 "Offset must be less then the input string length.\n");
    g_object_unref (unpacker);
    return message;
  }

  while (!done) {
    GVariant *unpacked;
    if (message_type == MPACK_EOF) {
      /* The starting header gives us the type of RPC request, unique
       * call ID and user data.
       */
      message_type = mpack_rpc_receive (self->session,
                                        &buffer,
                                        &buffer_length,
                                        &rpc_message);
      if (message_type == MPACK_EOF)
        break;
    }

    /* The remaining of the message are message data. If the
     * message is a request or notification then message data will
     * be a procedure name followed by arguments for the procedure.
     * If message is a response, then message data will be a result
     * or error object returned by the server.
     */
    unpacked = gmpack_unpacker_unpack_string (unpacker,
                                              &buffer,
                                              &buffer_length,
                                              error);
    if (error != NULL) {
      break;
    }

    if (proc_or_error == NULL) {
      proc_or_error = unpacked;
    } else {
      args_or_result = unpacked;
      done = 1;
    }
  }

  *stop_pos = buffer - buffer_init;
  if (done) {
    if (message_type == MPACK_RPC_REQUEST) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_REQUEST);
      gmpack_message_set_rpc_id (message, rpc_message.id);
      gmpack_message_set_procedure (message, proc_or_error);
      gmpack_message_set_args (message, args_or_result);
    } else if (message_type == MPACK_RPC_RESPONSE) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_RESPONSE);
      gmpack_message_set_result (message, args_or_result);
      gmpack_message_set_error (message, proc_or_error);
      gmpack_message_set_data (message, rpc_message.data.p);
    } else if (message_type == MPACK_RPC_NOTIFICATION) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION);
      gmpack_message_set_procedure (message, proc_or_error);
      gmpack_message_set_args (message, args_or_result);
    } else {
      g_set_error (error,
                   GMPACK_SESSION_ERROR,
                   GMPACK_SESSION_ERROR_MISC,
                   "An unexpected error occurred while deserializing (RPC) "
                   "msgpack data.\n");
    }
  }

  g_object_unref (unpacker);
  return message;
}

static void
receive_data_free (ReceiveData *receive_data)
{
  g_bytes_unref (receive_data->data);
  g_slice_free (ReceiveData, receive_data);
}

static void
session_receive_thread (GTask         *task,
                        gpointer       source_object,
                        gpointer       task_data,
                        GCancellable  *cancellable)
{
  GmpackSession *self = source_object;
  ReceiveData *receive_data = task_data;
  GmpackMessage *message;
  GError *error = NULL;

  message = gmpack_session_receive (self,
                                    receive_data->data,
                                    receive_data->start_pos,
                                    receive_data->stop_pos,
                                    &error);
  if (!error) {
    g_task_return_pointer (task, message, g_object_unref);
  } else {
    g_task_return_error (task, error);
  }
}

void
gmpack_session_receive_async (GmpackSession       *self,
                              GBytes              *data,
                              gsize                start_pos,
                              gsize               *stop_pos,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  ReceiveData *receive_data;
  GBytes *copied;
  GTask *task;

  copied = g_bytes_new_from_bytes (data, 0, g_bytes_get_size (data));
  receive_data = g_slice_new (ReceiveData);
  receive_data->data = copied;
  receive_data->start_pos = start_pos;
  receive_data->stop_pos = stop_pos;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, receive_data, (GDestroyNotify) receive_data_free);
  g_task_run_in_thread (task, session_receive_thread);
  g_object_unref (task);
}

GmpackMessage *
gmpack_session_receive_finish (GmpackSession  *self,
                               GAsyncResult   *result,
                               GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

GBytes *
session_send (GmpackSession  *self,
              GmpackMessage  *message,
              GError        **error)
{
  GmpackMessageRpcType message_type = gmpack_message_get_rpc_type (message);
  GmpackPacker *packer = gmpack_packer_new ();
  GBytes *output;
  gsize pos = 0;
  gsize me_buffer_size = 0;
  gsize ar_buffer_size = 0;
  gsize buffer_start_size = 8;
  gsize buffer_left = buffer_start_size;
  gchar *buffer = malloc(sizeof(*buffer) * buffer_start_size);
  gchar *buffer_init = buffer;
  gchar *me_buffer = NULL;
  gchar *ar_buffer = NULL;
  gchar *final_buffer = NULL;
  gint result = -1;
  mpack_data_t d;

  error = NULL;

  if (message_type == GMPACK_MESSAGE_RPC_TYPE_REQUEST)
    d.p = gmpack_message_get_data (message);

  /* Our bytestring (that represents an RPC message) starts with the
   * RPC headers for one of the three possible modes of messaging.
   */
  while (TRUE) {
    result = -1;
    if (message_type == GMPACK_MESSAGE_RPC_TYPE_REQUEST) {
      result = mpack_rpc_request(self->session,
                                 &buffer,
                                 &buffer_left,
                                 d);
    } else if (message_type == GMPACK_MESSAGE_RPC_TYPE_RESPONSE) {
      result = mpack_rpc_reply(self->session,
                               &buffer,
                               &buffer_left,
                               gmpack_message_get_rpc_id (message));
    } else if (message_type == GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION) {
      result = mpack_rpc_notify(self->session,
                                &buffer,
                                &buffer_left);
    }

    if (result == MPACK_NOMEM) {
      self->session = session_grow(self->session);
    } else {
      break;
    }
  }

  if (result != MPACK_OK) {
    g_set_error (error,
                 GMPACK_SESSION_ERROR,
                 GMPACK_SESSION_ERROR_MISC,
                 "An unexpected error occurred while serializing (RPC) "
                 "msgpack data.\n");
    g_object_unref (packer);
    return 0;
  }

  pos = buffer_start_size - buffer_left;

  /* Once the RPC headers have been encoded, we pack relevant
   * objects that convey our message.
   */
  if (message_type == GMPACK_MESSAGE_RPC_TYPE_REQUEST
      || message_type == GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION) {
    me_buffer_size = gmpack_packer_pack_variant (packer,
                                                 gmpack_message_get_procedure (message),
                                                 &me_buffer,
                                                 error);
    ar_buffer_size = gmpack_packer_pack_variant (packer,
                                                 gmpack_message_get_args (message),
                                                 &ar_buffer,
                                                 error);
  } else if (message_type == GMPACK_MESSAGE_RPC_TYPE_RESPONSE) {
    ar_buffer_size = gmpack_packer_pack_variant (packer,
                                                 gmpack_message_get_result (message),
                                                 &ar_buffer,
                                                 error);
    me_buffer_size = gmpack_packer_pack_variant (packer,
                                                 gmpack_message_get_error (message),
                                                 &me_buffer,
                                                 error);
  }

  final_buffer = g_malloc (sizeof (*final_buffer) * (pos
                                                     + me_buffer_size
                                                     + ar_buffer_size));
  memcpy (final_buffer, buffer_init, pos);
  memcpy (final_buffer + pos, me_buffer, me_buffer_size);
  memcpy (final_buffer + pos + me_buffer_size, ar_buffer, ar_buffer_size);
  g_free (buffer_init);
  g_free (me_buffer);
  g_free (ar_buffer);
  output = g_bytes_new (final_buffer, pos + me_buffer_size + ar_buffer_size);

  g_object_unref (packer);
  g_free (final_buffer);

  return output;
}

GBytes *
gmpack_session_request (GmpackSession  *self,
                        GVariant       *method,
                        GVariant       *args,
                        gpointer        data,
                        GError        **error)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_REQUEST);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);
  gmpack_message_set_data (message, data);
  return session_send (self, message, error);
}

static void
session_send_thread (GTask         *task,
                     gpointer       source_object,
                     gpointer       task_data,
                     GCancellable  *cancellable)
{
  GmpackSession *self = source_object;
  GmpackMessage *message = task_data;
  GError *error = NULL;
  GBytes *output;

  output = session_send (self,
                         message,
                         &error);
  if (!error) {
    g_task_return_pointer (task, output, g_object_unref);
  } else {
    g_task_return_error (task, error);
  }
}

void
gmpack_session_request_async (GmpackSession        *self,
                              GVariant             *method,
                              GVariant             *args,
                              gpointer              data,
                              GCancellable         *cancellable,
                              GAsyncReadyCallback   callback,
                              gpointer              user_data)
{
  GTask *task;
  GmpackMessage *message = gmpack_message_new ();

  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_REQUEST);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);
  gmpack_message_set_data (message, data);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, message, (GDestroyNotify) g_object_unref);
  g_task_run_in_thread (task, session_send_thread);
  g_object_unref (task);
}

GBytes *
gmpack_session_request_finish (GmpackSession  *self,
                               GAsyncResult   *result,
                               GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

GBytes *
gmpack_session_notify (GmpackSession  *self,
                       GVariant       *method,
                       GVariant       *args,
                       GError        **error)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);
  return session_send (self, message, error);
}

void
gmpack_session_notify_async (GmpackSession        *self,
                             GVariant             *method,
                             GVariant             *args,
                             GCancellable         *cancellable,
                             GAsyncReadyCallback   callback,
                             gpointer              user_data)
{
  GTask *task;
  GmpackMessage *message = gmpack_message_new ();

  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, message, (GDestroyNotify) g_object_unref);
  g_task_run_in_thread (task, session_send_thread);
  g_object_unref (task);
}

GBytes *
gmpack_session_notify_finish (GmpackSession  *self,
                              GAsyncResult   *result,
                              GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

GBytes *
gmpack_session_respond (GmpackSession  *self,
                        gint32          request_id,
                        GVariant       *result,
                        gboolean        is_error,
                        GError        **error)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_RESPONSE);
  gmpack_message_set_rpc_id (message, request_id);
  if (is_error) {
    gmpack_message_set_result (message, g_variant_new_parsed ("@mv nothing"));
    gmpack_message_set_error (message, result);
  } else {
    gmpack_message_set_result (message, result);
    gmpack_message_set_error (message, g_variant_new_parsed ("@mv nothing"));
  }
  return session_send (self, message, error);
}

void
gmpack_session_respond_async (GmpackSession        *self,
                              gint32                request_id,
                              GVariant             *result,
                              gboolean              is_error,
                              GCancellable         *cancellable,
                              GAsyncReadyCallback   callback,
                              gpointer              user_data)
{
  GTask *task;
  GmpackMessage *message = gmpack_message_new ();

  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_RESPONSE);
  gmpack_message_set_rpc_id (message, request_id);
  if (is_error) {
    gmpack_message_set_result (message, g_variant_new_parsed ("@mv nothing"));
    gmpack_message_set_error (message, result);
  } else {
    gmpack_message_set_result (message, result);
    gmpack_message_set_error (message, g_variant_new_parsed ("@mv nothing"));
  }

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, message, (GDestroyNotify) g_object_unref);
  g_task_run_in_thread (task, session_send_thread);
  g_object_unref (task);
}

GBytes *
gmpack_session_respond_finish (GmpackSession  *self,
                               GAsyncResult   *result,
                               GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}
