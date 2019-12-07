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

struct _GmpackSession
{
  GObject              parent_instance;
  mpack_rpc_session_t *session;
  GmpackUnpacker      *unpacker;
  GmpackPacker        *packer;
  gint                 message_type;
  mpack_rpc_message_t  rpc_message;
  GVariant            *method_or_error;
  GVariant            *args_or_result;
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
  self->packer = gmpack_packer_new ();
  self->unpacker = gmpack_unpacker_new ();
  self->args_or_result = NULL;
  self->method_or_error = NULL;
  self->message_type = MPACK_EOF;
}

static void
viewer_file_dispose (GObject *object)
{
  GmpackSession *self = GMPACK_SESSION (object);
  g_clear_object (&self->packer);
  g_clear_object (&self->unpacker);
}

static void
gmpack_session_finalize (GObject *object)
{
  GmpackSession *self = GMPACK_SESSION (object);
  g_free (self->session);
  G_OBJECT_CLASS (gmpack_session_parent_class)->finalize (object);
}

GmpackSession *
gmpack_session_new (GmpackPacker *packer, GmpackUnpacker *unpacker)
{
  GmpackSession *session = g_object_new (GMPACK_SESSION_TYPE, NULL);
  if (packer != NULL) {
    g_object_unref (session->packer);
    session->packer = packer;
    g_object_ref (session->packer);
  }
  if (unpacker != NULL) {
    g_object_unref (session->unpacker);
    session->unpacker = unpacker;
    g_object_ref (session->unpacker);
  }
  return session;
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
gmpack_session_receive (GmpackSession *self,
                        const gchar   *data,
                        gsize          length,
                        gsize          start_pos,
                        gsize         *stop_pos)
{
  GmpackMessage *message = gmpack_message_new ();
  GError *error = NULL;
  const gchar *buffer_init = data;
  const gchar *buffer = data + start_pos;
  gsize buffer_length = length - start_pos;
  gboolean done = FALSE;

  if (start_pos >= length) {
    g_error ("Offset must be less then the input string length.\n");
    return message;
  }

  while (!done) {
    GVariant *unpacked;
    if (self->message_type == MPACK_EOF) {
      /* The starting header gives us the type of RPC request, unique
       * call ID and user data.
       */
      self->message_type = mpack_rpc_receive (self->session,
                                              &buffer,
                                              &buffer_length,
                                              &(self->rpc_message));
      if (self->message_type == MPACK_EOF)
        break;
    }

    /* The remaining of the message are message data. If the
     * message is a request or notification then message data will
     * be a procedure name followed by arguments for the procedure.
     * If message is a response, then message data will be a result
     * or error object returned by the server.
     */
    unpacked = gmpack_unpacker_unpack_string (self->unpacker,
                                              &buffer,
                                              &buffer_length,
                                              &error);
    if (error != NULL) {
      g_error_free (error);
      break;
    }

    if (self->method_or_error == NULL) {
      self->method_or_error = unpacked;
    } else {
      self->args_or_result = unpacked;
      done = 1;
    }
  }

  *stop_pos = buffer - buffer_init;
  if (done) {
    if (self->message_type == MPACK_RPC_REQUEST) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_REQUEST);
      gmpack_message_set_rpc_id (message, self->rpc_message.id);
      gmpack_message_set_procedure (message, self->method_or_error);
      gmpack_message_set_args (message, self->args_or_result);
    } else if (self->message_type == MPACK_RPC_RESPONSE) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_RESPONSE);
      gmpack_message_set_result (message, self->args_or_result);
      gmpack_message_set_error (message, self->method_or_error);
      gmpack_message_set_data (message, self->rpc_message.data.p);
    } else if (self->message_type == MPACK_RPC_NOTIFICATION) {
      gmpack_message_set_rpc_type (message,
                                   GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION);
      gmpack_message_set_procedure (message, self->method_or_error);
      gmpack_message_set_args (message, self->args_or_result);
    } else {
      g_error ("An unexpected error occurred while deserializing (RPC) "
               "msgpack data.\n");
    }
    self->method_or_error = NULL;
    self->args_or_result = NULL;
    self->message_type = MPACK_EOF;
  }

  return message;
}

gsize
gmpack_session_send (GmpackSession  *self,
                     GmpackMessage  *message,
                     gchar         **data)
{
  GError *error;
  GmpackMessageRpcType message_type = gmpack_message_get_rpc_type (message);
  mpack_data_t d;
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
    g_error ("An unexpected error occurred while serializing (RPC) msgpack "
             "data.\n");
    return 0;
  }

  pos = buffer_start_size - buffer_left;

  /* Once the RPC headers have been encoded, we pack relevant
   * objects that convey our message.
   */
  if (message_type == GMPACK_MESSAGE_RPC_TYPE_REQUEST
      || message_type == GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION) {
    me_buffer_size = gmpack_packer_pack_variant (self->packer,
                                                 gmpack_message_get_procedure (message),
                                                 &me_buffer,
                                                 &error);
    ar_buffer_size = gmpack_packer_pack_variant (self->packer,
                                                 gmpack_message_get_args (message),
                                                 &ar_buffer,
                                                 &error);
  } else if (message_type == GMPACK_MESSAGE_RPC_TYPE_RESPONSE) {
    ar_buffer_size = gmpack_packer_pack_variant (self->packer,
                                                 gmpack_message_get_result (message),
                                                 &ar_buffer,
                                                 &error);
    me_buffer_size = gmpack_packer_pack_variant (self->packer,
                                                 gmpack_message_get_error (message),
                                                 &me_buffer,
                                                 &error);
  }

  final_buffer = g_malloc (sizeof (*final_buffer) * (pos
                                                     + me_buffer_size
                                                     + ar_buffer_size));
  memcpy (final_buffer, buffer_init, pos);
  memcpy (final_buffer + pos, me_buffer, me_buffer_size);
  memcpy (final_buffer + pos + me_buffer_size, ar_buffer, ar_buffer_size);
  free(buffer_init);
  free(me_buffer);
  free(ar_buffer);
  *data = final_buffer;
  return pos + me_buffer_size + ar_buffer_size;
}

gsize
gmpack_session_request (GmpackSession  *self,
                        GVariant       *method,
                        GVariant       *args,
                        gpointer        user_data,
                        gchar         **data)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_REQUEST);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);
  gmpack_message_set_data (message, user_data);
  return gmpack_session_send (self, message, data);
}

gsize
gmpack_session_notify (GmpackSession  *self,
                       GVariant       *method,
                       GVariant       *args,
                       gchar         **data)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION);
  gmpack_message_set_procedure (message, method);
  gmpack_message_set_args (message, args);
  return gmpack_session_send (self, message, data);
}

gsize
gmpack_session_respond (GmpackSession  *self,
                        gint32          request_id,
                        GVariant       *result,
                        gboolean        is_error,
                        gchar         **data)
{
  GmpackMessage *message = gmpack_message_new ();
  gmpack_message_set_rpc_type (message, GMPACK_MESSAGE_RPC_TYPE_RESPONSE);
  gmpack_message_set_rpc_id (message, request_id);
  if (is_error) {
    gmpack_message_set_error (message, result);
  } else {
    gmpack_message_set_result (message, result);
  }
  return gmpack_session_send (self, message, data);
}
