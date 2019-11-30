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
#include <stdlib.h>

#include "wrapper.c"
#include "gmpackunpacker.h"

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
  obj = gmpack_unpacker_unpack_string (G_OBJECT (unpacker), &data, length, &error);
  if (obj != NULL) {
    g_print ("Unpacked:\n");
    g_print ("%s\n", g_variant_print (obj, TRUE));
    g_print ("%d\n", gmpack_unpacker_is_busy (G_OBJECT (unpacker)));
    return 0;

    /* length = lmpack_pack (obj, &str); */
    /* if (length >= 0 && str != NULL) { */
    /*   int index; */
    /*   g_print ("Packed!\n"); */
    /*   for (index = 0; index < length; ++index) { */
    /*     printf ("%hhx ", str[index]); */
    /*   } */
    /*   printf ("\n"); */
    /*   return 0; */
    /* } */
  }
  g_print ("Error!\n");
  return 1;
}

gint session_test ()
{
  char *ret = NULL;
  size_t ret_length = 0;
  session_t *session = lmpack_session_new (NULL, NULL);
  element_t *method = element_new ();
  element_t *arguments = element_new ();
  element_t *first_arg = element_new ();
  element_t *second_arg = element_new ();
  element_t *element = NULL;

  arguments->type = ELEMENT_TYPE_ARRAY;
  element_create_array (arguments, 2);
  element = element_at_index (arguments, 0);
  element->data->p = first_arg;
  element = element_at_index (arguments, 1);
  element->data->p = second_arg;

  method->type = ELEMENT_TYPE_STR;
  method->data->p = "req1";
  method->length = 4;
  first_arg->type = ELEMENT_TYPE_SINT;
  first_arg->data->i = -1;
  second_arg->type = ELEMENT_TYPE_UINT;
  second_arg->data->u = 1;
  ret_length = lmpack_session_request (session,
                                       method,
                                       arguments,
                                       NULL,
                                       &ret);
  if (ret_length != 0) {
    size_t index;
    g_print ("Request 1 complete\n");
    for (index = 0; index < ret_length; ++index) {
      printf ("%hhx ", ret[index]);
    }
    printf ("\n");
  }

  method->type = ELEMENT_TYPE_STR;
  method->data->p = "req2";
  method->length = 4;
  first_arg->type = ELEMENT_TYPE_SINT;
  first_arg->data->i = -2;
  second_arg->type = ELEMENT_TYPE_UINT;
  second_arg->data->u = 2;
  free(ret);
  ret = NULL;
  ret_length = lmpack_session_request (session,
                                       method,
                                       arguments,
                                       NULL,
                                       &ret);
  if (ret_length != 0) {
    size_t index;
    g_print ("Request 2 complete\n");
    for (index = 0; index < ret_length; ++index) {
      printf ("%hhx ", ret[index]);
    }
    printf ("\n");
  }

  method->type = ELEMENT_TYPE_STR;
  method->data->p = "req3";
  method->length = 4;
  first_arg->type = ELEMENT_TYPE_SINT;
  first_arg->data->i = -3;
  second_arg->type = ELEMENT_TYPE_UINT;
  second_arg->data->u = 3;
  free(ret);
  ret = NULL;
  ret_length = lmpack_session_request (session,
                                       method,
                                       arguments,
                                       NULL,
                                       &ret);
  if (ret_length != 0) {
    size_t index;
    g_print ("Request 3 complete\n");
    for (index = 0; index < ret_length; ++index) {
      printf ("%hhx ", ret[index]);
    }
    printf ("\n");
  }

  transit_values_t values = lmpack_session_receive (session, ret, ret_length, 0);
  if (values.msg_type == MPACK_RPC_REQUEST)
    printf("request\n");
  printf("%d\n", values.msg_id);
  element_t *args_obj = values.args_or_result;
  size_t index = 0;
  for (index = 0; index < args_obj->length; ++index) {
    element_t *elem = element_at_index (args_obj, index);
    element_t *arg = elem->data->p;
    if (arg->type == ELEMENT_TYPE_SINT)
      printf ("%lli\n", arg->data->i);
    if (arg->type == ELEMENT_TYPE_UINT)
      printf ("%llu\n", arg->data->u);
  }
  printf ("\n");

  lmpack_session_delete (session);
  return 0;
}

gint main (gint   argc,
           gchar *argv[])
{
  return pack_unpack_test () | session_test ();
}
