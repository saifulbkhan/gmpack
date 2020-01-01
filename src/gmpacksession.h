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

#include "gmpackmessage.h"
#include "gmpackpacker.h"
#include "gmpackunpacker.h"

G_BEGIN_DECLS

#define GMPACK_SESSION_TYPE gmpack_session_get_type ()
G_DECLARE_FINAL_TYPE (GmpackSession, gmpack_session, GMPACK, SESSION, GObject)

GmpackSession *gmpack_session_new ();
GmpackMessage *gmpack_session_receive (GmpackSession *self,
                                       const gchar   *data,
                                       gsize          length,
                                       gsize          start_pos,
                                       gsize         *stop_pos);
gsize gmpack_session_send (GmpackSession  *self,
                           GmpackMessage  *message,
                           gchar         **data);
gsize gmpack_session_request (GmpackSession  *self,
                              GVariant       *method,
                              GVariant       *args,
                              gpointer        user_data,
                              gchar         **data);
gsize gmpack_session_notify (GmpackSession  *self,
                             GVariant       *method,
                             GVariant       *args,
                             gchar         **data);
gsize gmpack_session_respond (GmpackSession  *self,
                              gint32          request_id,
                              GVariant       *result,
                              gboolean        is_error,
                              gchar         **data);

G_END_DECLS

#endif /* __GMPACK_SESSION_H__ */
