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

#ifndef __GMPACK_UNPACKER_H__
#define __GMPACK_UNPACKER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GMPACK_UNPACKER_ERROR gmpack_unpacker_error_quark ()

/* Error flags */
typedef enum
{
  GMPACK_UNPACKER_ERROR_BUSY, /* unpacker is busy unpacking another string */
  GMPACK_UNPACKER_ERROR_PARSER, /* bad parser state, cannot proceed */
  GMPACK_UNPACKER_ERROR_INVALID, /* input string to be unpacked is not valid */
  GMPACK_UNPACKER_ERROR_MISC /* unknown or miscellaneous error */
} GmpackUnpackerError;

#define GMPACK_UNPACKER_TYPE gmpack_unpacker_get_type ()
G_DECLARE_FINAL_TYPE (GmpackUnpacker, gmpack_unpacker, GMPACK, UNPACKER, GObject)

GmpackUnpacker *gmpack_unpacker_new (void);
GVariant *gmpack_unpacker_unpack_string (GObject      *object,
                                         const gchar **string,
                                         gsize         length,
                                         GError      **error);
gboolean gmpack_unpacker_is_busy (GObject *object);

G_END_DECLS

#endif /* __GMPACK_UNPACKER_H__ */
