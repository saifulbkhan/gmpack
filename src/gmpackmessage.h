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

#ifndef __GMPACK_MESSAGE_H__
#define __GMPACK_MESSAGE_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
  GMPACK_MESSAGE_RPC_TYPE_NONE,
  GMPACK_MESSAGE_RPC_TYPE_REQUEST,
  GMPACK_MESSAGE_RPC_TYPE_RESPONSE,
  GMPACK_MESSAGE_RPC_TYPE_NOTIFICATION
} GmpackMessageRpcType;

#define GMPACK_MESSAGE_TYPE gmpack_message_get_type ()
G_DECLARE_FINAL_TYPE (GmpackMessage, gmpack_message, GMPACK, MESSAGE, GObject)

GmpackMessage *gmpack_message_new (void);
void gmpack_message_set_rpc_type (GmpackMessage        *self,
                                  GmpackMessageRpcType  rpc_type);
void gmpack_message_set_rpc_id (GmpackMessage *self, guint32 rpc_id);
void gmpack_message_set_data (GmpackMessage *self, gpointer data);
void gmpack_message_set_procedure (GmpackMessage *self, GVariant *procedure);
void gmpack_message_set_args (GmpackMessage *self, GVariant *args);
void gmpack_message_set_result (GmpackMessage *self, GVariant *result);
void gmpack_message_set_error (GmpackMessage *self, GVariant *error);
GmpackMessageRpcType gmpack_message_get_rpc_type (GmpackMessage *self);
guint32 gmpack_message_get_rpc_id (GmpackMessage *self);
gpointer gmpack_message_get_data (GmpackMessage *self);
GVariant *gmpack_message_get_procedure (GmpackMessage *self);
GVariant *gmpack_message_get_args (GmpackMessage *self);
GVariant *gmpack_message_get_result (GmpackMessage *self);
GVariant *gmpack_message_get_error (GmpackMessage *self);

G_END_DECLS

#endif /* __GMPACK_MESSAGE_H__ */
