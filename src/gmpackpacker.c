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
#include "gmpackpacker.h"
#include "mpack.h"

struct _GmpackPacker
{
  GObject         parent_instance;
  mpack_parser_t *parser;
  gboolean        packing;
  GVariant       *root;
};

G_DEFINE_TYPE (GmpackPacker, gmpack_packer, G_TYPE_OBJECT)

static void
gmpack_packer_class_init (GmpackPackerClass *klass)
{
}

static void
gmpack_packer_init (GmpackPacker *self)
{
  self->parser = g_malloc (sizeof (*self->parser));
  if (!self->parser) {
    g_error ("Failed to allocate memory for unpacker parser.\n");
  }
  mpack_parser_init (self->parser, 0);
  self->parser->data.p = (void *) self;
  self->packing = FALSE;
  self->root = NULL;
}

static void
gmpack_packer_finalize (GObject *object)
{
  GmpackPacker *self = GMPACK_PACKER (object);

  /* We avoid freeing the root variant, created by user */
  self->root = NULL;
  g_free (self->parser);

  G_OBJECT_CLASS (gmpack_packer_parent_class)->finalize (object);
}

GmpackPacker *
gmpack_packer_new ()
{
  GmpackPacker *packer = g_object_new (GMPACK_PACKER_TYPE, NULL);
  return packer;
}

GQuark
gmpack_packer_error_quark (void)
{
  return g_quark_from_static_string ("gmpack-packer-error-quark");
}

static void
gmpack_unparse_enter (mpack_parser_t *parser,
                      mpack_node_t   *node)
{
  GmpackPacker *packer = parser->data.p;
  mpack_node_t *parent = MPACK_PARENT_NODE (node);
  GVariant *var = NULL;

  if (parent) {
    /* get the parent */
    GVariant *parent_var = parent->data[0].p;

    /* strings and bytestrings are a special case, they are packed as
     * single child chunk node */
    if (parent->tok.type == MPACK_TOKEN_STR) {
      node->tok = mpack_pack_chunk (g_variant_get_string (parent_var, NULL),
                                    parent->tok.length);
      return;
    } else if (parent->tok.type == MPACK_TOKEN_BIN) {
      node->tok = mpack_pack_chunk (g_variant_get_bytestring (parent_var),
                                    parent->tok.length);
      return;
    } else if (parent->tok.type == MPACK_TOKEN_EXT) {
      gchar *bytestring;
      gint64 ext_code;
      g_variant_get (parent_var, "(iay)", &ext_code, &bytestring);
      node->tok = mpack_pack_chunk (bytestring, parent->tok.length);
      g_free (bytestring);
      return;
    }

    if (parent->tok.type == MPACK_TOKEN_ARRAY) {
      g_variant_get_child (parent_var, parent->pos, "v", &var);
    } else if (parent->tok.type == MPACK_TOKEN_MAP) {
      if (parent->key_visited) {
        /* key has been serialized, no do value */
        var = parent->data[1].p;
      } else {
        GVariant *pair = g_variant_get_child_value (parent_var, parent->pos);
        GVariant *value = NULL;

        /* serialize the key first and store value for later */
        g_variant_get (pair, "(vv)", &var, &value);
        parent->data[1].p = value;
      }
    }
  } else {
    var = packer->root;
  }

  const GVariantType *var_type = g_variant_get_type (var);
  if (g_variant_type_equal (var_type, G_VARIANT_TYPE_BOOLEAN)) {
    node->tok = mpack_pack_boolean (g_variant_get_boolean (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE_UINT64)) {
    node->tok = mpack_pack_uint (g_variant_get_uint64 (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE_INT64)) {
    node->tok = mpack_pack_sint (g_variant_get_int64 (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE_DOUBLE)) {
    node->tok = mpack_pack_float (g_variant_get_double (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE_BYTESTRING)) {
    gsize length;
    g_free ((g_variant_dup_bytestring (var, &length)));
    node->tok = mpack_pack_bin (length);
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE_STRING)) {
    gsize length;
    g_free (g_variant_dup_string (var, &length));
    node->tok = mpack_pack_str (length);
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE ("(iay)"))) {
    gchar *bytestring;
    gint64 ext_code;
    g_variant_get (var, "(iay)", &ext_code, &bytestring);
    node->tok = mpack_pack_ext (ext_code, strlen(bytestring));
    g_free (bytestring);
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE ("av"))) {
    node->tok = mpack_pack_array (g_variant_n_children (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE ("a(vv)"))) {
    node->tok = mpack_pack_map (g_variant_n_children (var));
  } else if (g_variant_type_equal (var_type, G_VARIANT_TYPE ("mv"))){
    node->tok = mpack_pack_nil();
  }

  node->data[0].p = var;
}

static void
gmpack_unparse_exit (mpack_parser_t *parser,
                     mpack_node_t   *node)
{
  if (node->tok.type != MPACK_TOKEN_CHUNK) {
    /* release the object */
    node->data[0].p = NULL;
  }
}

gsize
gmpack_packer_pack_variant (GObject   *object,
                            GVariant  *variant,
                            gchar    **str,
                            GError   **error)
{
  gint32 result = 1;
  gchar *final_buffer = NULL;
  gchar *buffer = NULL;
  gchar *buffer_cursor = NULL;
  gsize length = 0;
  gsize buffer_size = 0;
  gsize buffer_left = 0;
  GmpackPacker *self = GMPACK_PACKER (object);

  if (self->packing) {
    g_set_error (error,
                 GMPACK_PACKER_ERROR,
                 GMPACK_PACKER_ERROR_BUSY,
                 "This packer instance is already working.");
    return -1;
  }

  buffer_size = 16;
  buffer = g_malloc (sizeof (*buffer) * buffer_size);
  if (!buffer) {
    g_set_error (error,
                 GMPACK_PACKER_ERROR,
                 GMPACK_PACKER_ERROR_MEMORY,
                 "Failed to allocate memory for buffer.");
    return -1;
  }
  buffer_cursor = buffer;
  buffer_left = buffer_size;

  self->root = variant;
  do {
    gsize buffer_left_init = buffer_left;
    self->packing = TRUE;
    result = mpack_unparse (self->parser,
                            &buffer_cursor,
                            &buffer_left,
                            gmpack_unparse_enter,
                            gmpack_unparse_exit);

    if (result == MPACK_NOMEM) {
      self->parser = gmpack_grow_parser (self->parser);
      if (!self->parser) {
        self->packing = FALSE;
        g_set_error (error,
                     GMPACK_PACKER_ERROR,
                     GMPACK_PACKER_ERROR_PARSER,
                     "Failed to grow packer capacity.");
        return -1;
      }
    }

    length += buffer_left_init - buffer_left;

    if (!buffer_left) {
      /* buffer is empty, resize */
      gchar *new_buffer = g_malloc (sizeof (*new_buffer) * buffer_size * 2);
      memcpy (new_buffer, buffer, length);
      g_free (buffer);
      buffer = new_buffer;
      buffer_size = buffer_size * 2;
      buffer_cursor = new_buffer + length;
      buffer_left = buffer_size - length;
    }
  } while (result == MPACK_EOF || result == MPACK_NOMEM);

  final_buffer = g_malloc (sizeof (*final_buffer) * (length));
  memcpy (final_buffer, buffer, length);
  free (buffer);
  *str = final_buffer;
  self->packing = FALSE;
  return length;
}

gboolean
gmpack_packer_is_busy (GObject *object)
{
  GmpackPacker *self = GMPACK_PACKER (object);
  return self->packing;
}
