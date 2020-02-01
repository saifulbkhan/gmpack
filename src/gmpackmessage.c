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

#include "gmpackmessage.h"

struct _GmpackMessage
{
  GObject               parent_instance;
  GmpackMessageRpcType  rpc_type;
  guint32               rpc_id;
  gpointer              data;
  GVariant             *procedure;
  GVariant             *args;
  GVariant             *result;
  GVariant             *error;
};

G_DEFINE_TYPE (GmpackMessage, gmpack_message, G_TYPE_OBJECT)

static void
gmpack_message_class_init (GmpackMessageClass *klass)
{
}

static void
gmpack_message_init (GmpackMessage *self)
{
  self->rpc_type = GMPACK_MESSAGE_RPC_TYPE_NONE;
  self->rpc_id = -1;
  self->data = NULL;
  self->procedure = NULL;
  self->error = NULL;
  self->args = NULL;
  self->result = NULL;
}

static void
gmpack_message_dispose (GObject *object)
{
  GmpackMessage *self = GMPACK_MESSAGE (object);

  g_clear_object (&self->procedure);
  g_clear_object (&self->error);
  g_clear_object (&self->args);
  g_clear_object (&self->result);
}

static void
gmpack_message_finalize (GObject *object)
{
  GmpackMessage *self = GMPACK_MESSAGE (object);
  self->data = NULL;
  G_OBJECT_CLASS (gmpack_message_parent_class)->finalize (object);
}

GmpackMessage *
gmpack_message_new ()
{
  GmpackMessage *message = g_object_new (GMPACK_MESSAGE_TYPE, NULL);
  return message;
}

void
gmpack_message_set_rpc_type (GmpackMessage        *self,
                             GmpackMessageRpcType  rpc_type)
{
  self->rpc_type = rpc_type;
}

void
gmpack_message_set_rpc_id (GmpackMessage *self, guint32 rpc_id)
{
  self->rpc_id = rpc_id;
}

void
gmpack_message_set_data (GmpackMessage *self, gpointer data)
{
  self->data = data;
}

void
gmpack_message_set_procedure (GmpackMessage *self, GVariant *procedure)
{
  self->procedure = procedure;
}

void
gmpack_message_set_args (GmpackMessage *self, GVariant *args)
{
  self->args = args;
}

void
gmpack_message_set_result (GmpackMessage *self, GVariant *result)
{
  self->result = result;
}

void
gmpack_message_set_error (GmpackMessage *self, GVariant *error)
{
  self->error = error;
}

GmpackMessageRpcType
gmpack_message_get_rpc_type (GmpackMessage *self)
{
  return self->rpc_type;
}

guint32
gmpack_message_get_rpc_id (GmpackMessage *self)
{
  return self->rpc_id;
}

gpointer
gmpack_message_get_data (GmpackMessage *self)
{
  return self->data;
}

GVariant *
gmpack_message_get_procedure (GmpackMessage *self)
{
  return self->procedure;
}

GVariant *
gmpack_message_get_args (GmpackMessage *self)
{
  return self->args;
}

GVariant *
gmpack_message_get_result (GmpackMessage *self)
{
  return self->result;
}

GVariant *
gmpack_message_get_error (GmpackMessage *self)
{
  return self->error;
}
