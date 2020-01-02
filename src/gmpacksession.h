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

#ifndef __GMPACK_SESSION_H__
#define __GMPACK_SESSION_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "gmpackmessage.h"
#include "gmpackpacker.h"
#include "gmpackunpacker.h"

G_BEGIN_DECLS

#define GMPACK_SESSION_ERROR gmpack_session_error_quark ()

/* Error flags */
typedef enum
{
  GMPACK_SESSION_ERROR_IMPROPER, /* data (or length) in transit is malformed */
  GMPACK_SESSION_ERROR_MISC, /* miscellaneous errors */
} GmpackSessionError;

#define GMPACK_SESSION_TYPE gmpack_session_get_type ()
G_DECLARE_FINAL_TYPE (GmpackSession, gmpack_session, GMPACK, SESSION, GObject)

GmpackSession *gmpack_session_new ();
GmpackMessage *gmpack_session_receive (GmpackSession  *self,
                                       GBytes         *data,
                                       gsize           start_pos,
                                       gsize          *stop_pos,
                                       GError        **error);
void gmpack_session_receive_async (GmpackSession       *self,
                                   GBytes              *data,
                                   gsize                start_pos,
                                   gsize               *stop_pos,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);
GmpackMessage *gmpack_session_receive_finish (GmpackSession  *self,
                                              GAsyncResult   *result,
                                              GError        **error);
GBytes *gmpack_session_request (GmpackSession  *self,
                                GVariant       *method,
                                GVariant       *args,
                                gpointer        data,
                                GError        **error);
void gmpack_session_request_async (GmpackSession       *self,
                                   GVariant            *method,
                                   GVariant            *args,
                                   gpointer             data,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);
GBytes *gmpack_session_request_finish (GmpackSession  *self,
                                       GAsyncResult   *result,
                                       GError        **error);
GBytes *gmpack_session_notify (GmpackSession  *self,
                               GVariant       *method,
                               GVariant       *args,
                               GError        **error);
void gmpack_session_notify_async (GmpackSession       *self,
                                  GVariant            *method,
                                  GVariant            *args,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data);
GBytes *gmpack_session_notify_finish (GmpackSession  *self,
                                      GAsyncResult   *result,
                                      GError        **error);
GBytes *gmpack_session_respond (GmpackSession  *self,
                                gint32          request_id,
                                GVariant       *result,
                                gboolean        is_error,
                                GError        **error);
void gmpack_session_respond_async (GmpackSession       *self,
                                   gint32               request_id,
                                   GVariant            *result,
                                   gboolean             is_error,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);
GBytes *gmpack_session_respond_finish (GmpackSession  *self,
                                       GAsyncResult   *result,
                                       GError        **error);

G_END_DECLS

#endif /* __GMPACK_SESSION_H__ */
