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
#include "gmpackunpacker.h"
#include "mpack.h"

struct _GmpackUnpacker
{
  GObject         parent_instance;
  mpack_parser_t *parser;
  gboolean        unpacking;
  GVariant       *root;
  gchar          *string_buffer;
};

G_DEFINE_TYPE (GmpackUnpacker, gmpack_unpacker, G_TYPE_OBJECT)

static void
gmpack_unpacker_class_init (GmpackUnpackerClass *klass)
{
}

static void
gmpack_unpacker_init (GmpackUnpacker *self)
{
  self->parser = g_malloc (sizeof (*self->parser));
  if (!self->parser) {
    g_error ("Failed to allocate memory for unpacker parser.\n");
  }
  mpack_parser_init (self->parser, 0);
  self->parser->data.p = (void *) self;
  self->string_buffer = NULL;
  self->unpacking = FALSE;
  self->root = NULL;
}

static void
gmpack_unpacker_finalize (GObject *object)
{
  GmpackUnpacker *self = GMPACK_UNPACKER (object);

  g_free (self->parser);
  g_variant_unref (self->root);
  g_free (self->string_buffer);

  G_OBJECT_CLASS (gmpack_unpacker_parent_class)->finalize (object);
}

GmpackUnpacker *
gmpack_unpacker_new ()
{
  GmpackUnpacker *unpacker = g_object_new (GMPACK_UNPACKER_TYPE, NULL);
  return unpacker;
}

GQuark
gmpack_unpacker_error_quark (void)
{
  return g_quark_from_static_string ("gmpack-unpacker-error-quark");
}

static void
gmpack_parse_enter(mpack_parser_t *parser,
                   mpack_node_t   *node)
{
  GmpackUnpacker *unpacker = GMPACK_UNPACKER (parser->data.p);
  GObject *obj = NULL;

  switch (node->tok.type) {
    case MPACK_TOKEN_BOOLEAN: {
      gboolean value = (gboolean) mpack_unpack_boolean (node->tok);
      obj = (GObject *) g_variant_new_boolean (value);
      break;
    }
    case MPACK_TOKEN_UINT: {
      guint64 value = (guint64) mpack_unpack_uint (node->tok);
      obj = (GObject *) g_variant_new_uint64 (value);
      break;
    }
    case MPACK_TOKEN_SINT: {
      gint64 value = (gint64) mpack_unpack_sint (node->tok);
      obj = (GObject *) g_variant_new_int64 (value);
      break;
    }
    case MPACK_TOKEN_FLOAT: {
      gdouble value = (gdouble) mpack_unpack_float (node->tok);
      obj = (GObject *) g_variant_new_double (value);
      break;
    }
    case MPACK_TOKEN_CHUNK: {
      /* chunks should always follow string/bin/ext tokens */
      assert (unpacker->string_buffer != NULL);
      memcpy (unpacker->string_buffer + MPACK_PARENT_NODE (node)->pos,
              node->tok.data.chunk_ptr,
              node->tok.length);
      break;
    }
    case MPACK_TOKEN_BIN:
    case MPACK_TOKEN_STR:
    case MPACK_TOKEN_EXT: {
      unpacker->string_buffer = malloc (node->tok.length);
      assert (unpacker->string_buffer != NULL);
      break;
    }
    case MPACK_TOKEN_ARRAY: {
      obj = (GObject *) g_variant_builder_new (G_VARIANT_TYPE ("av"));
      break;
    }
    case MPACK_TOKEN_MAP: {
      obj = (GObject *) g_variant_builder_new (G_VARIANT_TYPE ("a(vv)"));
      break;
    }
    default: {
      obj = (GObject *) g_variant_new_maybe (G_VARIANT_TYPE_VARIANT, NULL);
      break;
    }
  }
  node->data[0].p = obj;
}

static void
gmpack_parse_exit(mpack_parser_t *parser,
                  mpack_node_t   *node)
{
  GmpackUnpacker *unpacker = GMPACK_UNPACKER (parser->data.p);
  mpack_node_t *parent = MPACK_PARENT_NODE (node);
  GObject *obj = node->data[0].p;
  GVariant *var = NULL;
  GVariantBuilder *builder = NULL;

  switch (node->tok.type) {
    case MPACK_TOKEN_CHUNK:
      return;
    case MPACK_TOKEN_STR:
      var = g_variant_new_take_string (g_strndup (unpacker->string_buffer,
                                                  node->tok.length));
      unpacker->string_buffer = NULL;
      break;
    case MPACK_TOKEN_BIN:
      var = g_variant_new_bytestring (g_strndup (unpacker->string_buffer,
                                                 node->tok.length));
      g_free (unpacker->string_buffer);
      unpacker->string_buffer = NULL;
      break;
    case MPACK_TOKEN_EXT:
      var = g_variant_new ("(iay)",
                           node->tok.data.ext_type,
                           g_strndup (unpacker->string_buffer,
                                      node->tok.length));
      g_free (unpacker->string_buffer);
      unpacker->string_buffer = NULL;
      break;
    case MPACK_TOKEN_ARRAY:
    case MPACK_TOKEN_MAP:
      builder = (GVariantBuilder *) obj;
      if (node->tok.type == MPACK_TOKEN_ARRAY) {
        var = g_variant_new ("av", builder);
      } else if (node->tok.type == MPACK_TOKEN_MAP) {
        var = g_variant_new ("a(vv)", builder);
      }
      g_variant_builder_unref (builder);
      break;
    default:
      break;
  }

  if (var == NULL)
    var = (GVariant *) obj;

  if (parent) {
    builder = (GVariantBuilder *) parent->data[0].p;
    if (parent->tok.type == MPACK_TOKEN_ARRAY) {
      g_variant_builder_add (builder, "v", var);
    }
    if (parent->tok.type == MPACK_TOKEN_MAP) {
      if (parent->key_visited) {
        /* save key */
        parent->data[1].p = var;
      } else {
        /* set pair using last saved key and current object as value */
        GVariant *key = (GVariant *) parent->data[1].p;
        g_variant_builder_add (builder, "(vv)", key, var);
      }
    }
  } else {
    unpacker->root = g_variant_ref_sink (var);
  }
}

GVariant *
gmpack_unpacker_unpack_string (GmpackUnpacker *self,
                               const gchar   **string,
                               gsize          *length,
                               GError        **error)
{
  int result;

  if (self->unpacking) {
    g_set_error (error,
                 GMPACK_UNPACKER_ERROR,
                 GMPACK_UNPACKER_ERROR_BUSY,
                 "Unpacker is busy unpacking another string.");
    return NULL;
  }

  do {
    self->unpacking = TRUE;
    result = mpack_parse (self->parser,
                          string,
                          length,
                          gmpack_parse_enter,
                          gmpack_parse_exit);

    if (result == MPACK_NOMEM) {
      self->parser = gmpack_grow_parser (self->parser);
      if (!self->parser) {
        self->unpacking = FALSE;
        g_set_error (error,
                     GMPACK_UNPACKER_ERROR,
                     GMPACK_UNPACKER_ERROR_PARSER,
                     "Failed to grow unpacker capacity.");
        return NULL;
      }
    }
  } while (result == MPACK_NOMEM);

  if (result == MPACK_ERROR) {
    g_set_error (error,
                 GMPACK_UNPACKER_ERROR,
                 GMPACK_UNPACKER_ERROR_INVALID,
                 "Invalid msgpack string.");
  }

  self->unpacking = FALSE;
  return self->root;
}

gboolean
gmpack_unpacker_is_busy (GmpackUnpacker *self)
{
  return self->unpacking;
}
