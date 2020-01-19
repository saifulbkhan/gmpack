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

#ifndef __GMPACK_SERVER_H__
#define __GMPACK_SERVER_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GMPACK_SERVER_TYPE gmpack_server_get_type ()
G_DECLARE_FINAL_TYPE (GmpackServer, gmpack_server, GMPACK, SERVER, GObject)

typedef GVariant * (*GmpackServerHandler) (GList     *args,
                                           gpointer   user_data,
                                           GError   **error);

GmpackServer *gmpack_server_new (void);
void gmpack_server_accept_io_stream (GmpackServer  *self,
                                     GIOStream     *iostream,
                                     GError       **error);
void gmpack_server_listen_at_port (GmpackServer  *self,
                                   guint16        port,
                                   GError       **error);
void gmpack_server_stop_listening (GmpackServer  *self);
guint16 gmpack_server_get_port (GmpackServer *self);
guint gmpack_server_bind (GmpackServer        *self,
                          const gchar         *method,
                          GmpackServerHandler  handler,
                          gpointer             user_data,
                          GDestroyNotify       user_data_destroy);
void gmpack_server_unbind (GmpackServer *self, guint bound_id);

G_END_DECLS

#endif /* __GMPACK_SERVER_H__ */
