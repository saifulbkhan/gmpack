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

#ifndef __GMPACK_CLIENT_H__
#define __GMPACK_CLIENT_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GMPACK_CLIENT_TYPE gmpack_client_get_type ()
G_DECLARE_FINAL_TYPE (GmpackClient, gmpack_client, GMPACK, CLIENT, GObject)

GmpackClient *gmpack_client_new (GIOStream *iostream);
GmpackClient *gmpack_client_new_for_tcp (const gchar *address, guint port);
gboolean gmpack_client_request (GmpackClient  *self,
                                const gchar   *method,
                                GList         *args,
                                GVariant     **result,
                                GCancellable  *cancellable,
                                GError       **error);
void gmpack_client_request_async (GmpackClient         *self,
                                  const gchar          *method,
                                  GList                *args,
                                  GVariant            **result,
                                  GCancellable         *cancellable,
                                  GAsyncReadyCallback   callback,
                                  gpointer              user_data);
gboolean gmpack_client_request_finish (GmpackClient  *self,
                                       GAsyncResult  *result,
                                       GError       **error);
void gmpack_client_notify (GmpackClient  *self,
                           const gchar   *method,
                           GList         *args,
                           GCancellable  *cancellable,
                           GError       **error);

G_END_DECLS

#endif /* __GMPACK_CLIENT_H__ */
