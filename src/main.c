/* main.c
 *
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

#include "gmpack-config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include "gmpackunpacker.h"
#include "gmpackpacker.h"
#include "gmpacksession.h"

gint pack_unpack_test ()
{
  GError *error;
  GVariant *obj = NULL;
  gchar *str = NULL;
  gsize length = 46;
  const gchar *data = "\x92\x82\xa7\x63\x6f\x6d\x70\x61\x63\x74\xc3\xa6\x73\x63\x68\x65\x6d\x61\x00"
                      "\x82\xa0\xcf\xdc\xc8\x0c\xd4\x00\x00\x00\x00\xa6\x6e\x65\x67\x20\x50\x69\xcb"
                      "\xc0\x09\x1e\xb8\x51\xeb\x85\x1f";
  GmpackUnpacker *unpacker = gmpack_unpacker_new ();
  obj = gmpack_unpacker_unpack_string (unpacker, &data, &length, &error);
  g_object_unref (unpacker);
  if (obj != NULL) {
    GmpackPacker *packer = gmpack_packer_new ();
    g_print ("Unpacked:\n");
    g_print ("%s\n", g_variant_print (obj, TRUE));

    error = NULL;
    length = gmpack_packer_pack_variant (packer, obj, &str, &error);
    g_object_unref (packer);
    if (length >= 0 && str != NULL) {
      int index;
      g_print ("Packed:\n");
      for (index = 0; index < length; ++index) {
        printf ("%hhx ", str[index]);
      }
      printf ("\n");
      return 0;
    }
  }
  g_print ("Error!\n");
  return 1;
}

gint session_test ()
{
  GmpackSession* session = gmpack_session_new (NULL, NULL);
  GVariant *method = g_variant_new_parsed ("'REQ'");
  GVariant *arguments = g_variant_new_parsed ("[<-1>, <uint64 18446744073709551615>]");
  gchar *ret = NULL;
  gsize ret_length = 0;

  ret_length = gmpack_session_request (session,
                                       method,
                                       arguments,
                                       NULL,
                                       &ret);
  if (ret_length != 0) {
    size_t index;
    g_print ("Request sent: ");
    for (index = 0; index < ret_length; ++index) {
      printf ("%hhx ", ret[index]);
    }
    printf ("\n");
  }

  GmpackMessage *message = gmpack_session_receive (session,
                                                   ret,
                                                   ret_length,
                                                   0,
                                                   &ret_length);
  if (gmpack_message_get_rpc_type (message)
      == GMPACK_MESSAGE_RPC_TYPE_REQUEST) {
    GVariant *received_method = gmpack_message_get_procedure (message);
    GVariant *received_arguments = gmpack_message_get_args (message);
    g_print ("Request received: ");
    g_printf ("<%s %s> ",
              g_variant_print (received_method, TRUE),
              g_variant_print (received_arguments, TRUE));
    g_printf ("| id = %d\n", gmpack_message_get_rpc_id (message));
  }

  g_variant_unref (method);
  g_variant_unref (arguments);
  g_object_unref (session);
  g_object_unref (message);
  g_free (ret);

  return 0;
}

gint main (gint   argc,
           gchar *argv[])
{
  return pack_unpack_test () | session_test ();
}
