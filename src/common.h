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

#define SINGLE_READ_COUNT 1024

#include <stdlib.h>

#include "mpack.h"
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

static mpack_parser_t *
gmpack_grow_parser(mpack_parser_t *parser)
{
  mpack_parser_t *old = parser;
  mpack_uint32_t new_capacity = old->capacity * 2;
  parser = malloc (MPACK_PARSER_STRUCT_SIZE (new_capacity));
  if (!parser) goto end;
  mpack_parser_init (parser, new_capacity);
  mpack_parser_copy (parser, old);
  free (old);
end:
  return parser;
}

static void
gmpack_read_istream_cb (GObject      *object,
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
                                           gmpack_read_istream_cb,
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

static void
gmpack_read_istream_async (GObject             *self,
                           GInputStream        *istream,
                           GmpackSession       *session,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
  ReadContext *context = NULL;
  g_autoptr(GTask) task = NULL;

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
                                   gmpack_read_istream_cb,
                                   g_steal_pointer (&task));
}

static GQueue *
gmpack_read_istream_finish (GObject        *self,
                            GAsyncResult   *result,
                            GError        **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static GmpackMessage *
gmpack_read_istream (GInputStream *istream,
                     GmpackSession *session,
                     GError **error)
{
  gboolean keep_reading;
  GBytes *bytes = NULL;
  GByteArray *pending_buffer = NULL;
  GmpackMessage *message = NULL;

  g_assert (GMPACK_IS_SESSION (session));
  g_assert (G_IS_INPUT_STREAM (istream));

  keep_reading = TRUE;
  while (keep_reading) {
    bytes = g_input_stream_read_bytes (istream, SINGLE_READ_COUNT, NULL, error);
    if (*error != NULL)
      return NULL;

    /* if the read size is less than the capacity, then we have reached EOF */
    if (g_bytes_get_size (bytes) < SINGLE_READ_COUNT) {
      keep_reading = FALSE;

      /* append to any previously pending buffer */
      if (pending_buffer != NULL) {
        gsize size = 0;
        gpointer data = g_bytes_unref_to_data (bytes, &size);
        g_byte_array_append (pending_buffer, data, size);
        bytes = g_byte_array_free_to_bytes (pending_buffer);
        pending_buffer = NULL;
      }
    } else {
      pending_buffer = g_bytes_unref_to_array (bytes);
    }
  }

  if (g_bytes_get_size (bytes) != 0) {
    message = gmpack_session_receive (session, bytes, 0, NULL, error);
  } else if (*error == NULL) {
    /* there was no data to read */
    g_set_error (error,
                 G_IO_ERROR,
                 G_IO_ERROR_FAILED,
                 "No data to read from peer");
  }

  if (*error != NULL && message != NULL) {
    g_object_unref (message);
    message = NULL;
  }

  if (bytes != NULL)
    g_bytes_unref (bytes);

  return message;
}
