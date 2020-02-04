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
  GVariant       *root;
  gpointer        buffer;
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
  self->buffer = NULL;
  self->root = NULL;
}

static void
gmpack_unpacker_finalize (GObject *object)
{
  GmpackUnpacker *self = GMPACK_UNPACKER (object);

  free (self->parser);
  g_variant_unref (self->root);
  if (self->buffer != NULL)
    free (self->buffer);

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
      g_assert (unpacker->buffer != NULL);
      memcpy (unpacker->buffer + MPACK_PARENT_NODE (node)->pos,
              node->tok.data.chunk_ptr,
              node->tok.length);
      break;
    }
    case MPACK_TOKEN_BIN:
    case MPACK_TOKEN_STR:
    case MPACK_TOKEN_EXT: {
      unpacker->buffer = malloc (node->tok.length);
      g_assert (unpacker->buffer != NULL);
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
      var = g_variant_new_take_string (g_strndup (unpacker->buffer,
                                                  node->tok.length));
      free (unpacker->buffer);
      unpacker->buffer = NULL;
      break;
    case MPACK_TOKEN_BIN:
      var = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                       unpacker->buffer,
                                       node->tok.length,
                                       sizeof (guint8));
      free (unpacker->buffer);
      unpacker->buffer = NULL;
      break;
    case MPACK_TOKEN_EXT: {
      GVariant *tmp = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                                 unpacker->buffer,
                                                 node->tok.length,
                                                 sizeof (guint8));
      var = g_variant_new ("(i@ay)", node->tok.data.ext_type, tmp);
      free (unpacker->buffer);
      unpacker->buffer = NULL;
      break;
    }
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

  do {
    result = mpack_parse (self->parser,
                          string,
                          length,
                          gmpack_parse_enter,
                          gmpack_parse_exit);

    if (result == MPACK_NOMEM) {
      self->parser = gmpack_grow_parser (self->parser);
      if (!self->parser) {
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
  } else if (result == MPACK_EOF) {
    g_set_error (error,
                 GMPACK_UNPACKER_ERROR,
                 GMPACK_UNPACKER_ERROR_EOF,
                 "Incomplete msgpack string.");
  }

  return self->root;
}
