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

#ifndef __GMPACK_PACKER_H__
#define __GMPACK_PACKER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GMPACK_PACKER_ERROR gmpack_packer_error_quark ()

/* Error flags */
typedef enum
{
  GMPACK_PACKER_ERROR_MEMORY, /* memory allocation failed during packing */
  GMPACK_PACKER_ERROR_PARSER, /* bad parser state, cannot proceed */
  GMPACK_PACKER_ERROR_MISC /* unknown or miscellaneous error */
} GmpackPackerError;

#define GMPACK_PACKER_TYPE gmpack_packer_get_type ()
G_DECLARE_FINAL_TYPE (GmpackPacker, gmpack_packer, GMPACK, PACKER, GObject)

GmpackPacker *gmpack_packer_new (void);
gsize gmpack_packer_pack_variant (GmpackPacker *object,
                                  GVariant     *variant,
                                  gchar       **string,
                                  GError      **error);

G_END_DECLS

#endif /* __GMPACK_PACKER_H__ */
