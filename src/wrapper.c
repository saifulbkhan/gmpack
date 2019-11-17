#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mpack.h"

typedef enum {
  ELEMENT_TYPE_NULL      = 1,
  ELEMENT_TYPE_BOOLEAN   = 2,
  ELEMENT_TYPE_UINT      = 3,
  ELEMENT_TYPE_SINT      = 4,
  ELEMENT_TYPE_FLOAT     = 5,
  ELEMENT_TYPE_ARRAY     = 6,
  ELEMENT_TYPE_MAP       = 7,
  ELEMENT_TYPE_BIN       = 8,
  ELEMENT_TYPE_STR       = 9,
  ELEMENT_TYPE_EXT       = 10
} element_type_t;

typedef struct element_t {
  mpack_data_t *data;
  int ext_code;
  element_type_t type;
  size_t length;
} element_t;

typedef struct pair_t {
  element_t *key;
  element_t *value;
} pair_t;

typedef struct unpacker_t {
  mpack_parser_t *parser;
  int unpacking;
  element_t *root;
  char *string_buffer;
} unpacker_t;

typedef struct  packer_t {
  mpack_parser_t *parser;
  int packing;
  element_t *root;
} packer_t;

typedef struct session_t {
  mpack_rpc_session_t *session;
  unpacker_t *unpacker;
  packer_t *packer;
  int type;
  mpack_rpc_message_t msg;
  void *method_or_error;
  void *args_or_result;
} session_t;

typedef struct transit_values_t {
  size_t pos;
  int msg_type;
  void *method_or_error;
  void *args_or_result;
  int msg_id;
  void *msg_data;
} transit_values_t;

static char *name_for_element(element_type_t et)
{
  switch (et) {
  case ELEMENT_TYPE_NULL: return "null";
  case ELEMENT_TYPE_BOOLEAN: return "bool";
  case ELEMENT_TYPE_UINT: return "uint";
  case ELEMENT_TYPE_SINT: return "int";
  case ELEMENT_TYPE_FLOAT: return "float";
  case ELEMENT_TYPE_ARRAY: return "array";
  case ELEMENT_TYPE_MAP: return "map";
  case ELEMENT_TYPE_BIN: return "binary";
  case ELEMENT_TYPE_STR: return "string";
  case ELEMENT_TYPE_EXT: return "ext";
  }
  return "";
}

static char *name_for_token(mpack_token_type_t tt)
{
  switch (tt) {
  case MPACK_TOKEN_NIL: return "null";
  case MPACK_TOKEN_BOOLEAN: return "bool";
  case MPACK_TOKEN_UINT: return "uint";
  case MPACK_TOKEN_SINT: return "int";
  case MPACK_TOKEN_FLOAT: return "float";
  case MPACK_TOKEN_CHUNK: return "chunk";
  case MPACK_TOKEN_ARRAY: return "array";
  case MPACK_TOKEN_MAP: return "map";
  case MPACK_TOKEN_BIN: return "binary";
  case MPACK_TOKEN_STR: return "string";
  case MPACK_TOKEN_EXT: return "ext";
  }
  return "";
}

static element_t *element_new()
{
  element_t *element = malloc(sizeof(*element));

  if (!element) {
    printf("Failed to allocate memory\n");
    return NULL;
  }

  element->ext_code = 0;
  element->data = malloc(sizeof(*element->data));
  if (!element->data) {
    printf("Failed to allocate memory for element data.\n");
    return NULL;
  }
  element->data->p = NULL;
  element->type = ELEMENT_TYPE_NULL;
  element->length = 0;

  return element;
}

static int element_create_array(element_t *parent, size_t length)
{
  int index = 0;
  element_t **element_array = malloc(sizeof(*element_array) * length);
  if (!element_array) {
    printf("Failed to allocate memory for element array.\n");
    return -1;
  }

  for (index = 0; index < length; ++index) {
    element_array[index] = element_new();
  }
  parent->data->p = element_array;
  parent->length = length;

  return 0;
}

static element_t *element_at_index(element_t *parent, size_t index)
{
  if (index >= parent->length || parent->data->p == NULL)
    return NULL;

  if (parent->type == ELEMENT_TYPE_ARRAY
      || parent->type == ELEMENT_TYPE_MAP) {
    element_t **array = parent->data->p;
    return array[index];
  }

  return NULL;
}

static int element_delete(element_t *element)
{
  int i = 0;

  if (element == NULL)
    return 0;

  if (element->data != NULL) {
    if (element->data->p != NULL) {
      if (element->type == ELEMENT_TYPE_ARRAY) {
        for (i = 0; i < element->length; ++i)
          element_delete(element_at_index(element, i));
      } else if (element->type == ELEMENT_TYPE_MAP) {
        for (i = 0; i < element->length; ++i) {
          element_t *child = element_at_index(element, i);
          pair_t* pair = child->data->p;
          element_delete(pair->key);
          element_delete(pair->value);
        }
      } else {
        free(element->data->p);
      }
    }
    free(element->data);
  }
  free(element);
  element = NULL;
  return 0;
}

static mpack_parser_t *lmpack_grow_parser(mpack_parser_t *parser)
{
  mpack_parser_t *old = parser;
  mpack_uint32_t new_capacity = old->capacity * 2;
  parser = malloc(MPACK_PARSER_STRUCT_SIZE(new_capacity));
  if (!parser) goto end;
  mpack_parser_init(parser, new_capacity);
  mpack_parser_copy(parser, old);
  free(old);
end:
  return parser;
}

static unpacker_t* lmpack_unpacker_new()
{
  unpacker_t *rv = malloc(sizeof(*rv));

  rv->parser = malloc(sizeof(*rv->parser));
  if (!rv->parser) {
    printf("Failed to allocate memory for unpacker parser.\n");
    return NULL;
  }
  mpack_parser_init(rv->parser, 0);
  rv->parser->data.p = rv;
  rv->string_buffer = NULL;
  rv->unpacking = 0;
  rv->root = NULL;

  return rv;
}

static int lmpack_unpacker_delete(unpacker_t *unpacker)
{
  if (unpacker == NULL)
    return 0;

  if (unpacker->root != NULL)
    element_delete(unpacker->root);
  free(unpacker->parser);
  free(unpacker);
  unpacker = NULL;
  return 0;
}

static void lmpack_parse_enter(mpack_parser_t *parser, mpack_node_t *node)
{
  unpacker_t *unpacker = parser->data.p;
  element_t *obj = element_new();

  switch (node->tok.type) {
    case MPACK_TOKEN_BOOLEAN:
      obj->data->u = mpack_unpack_boolean(node->tok);
      obj->type = ELEMENT_TYPE_BOOLEAN;
      break;
    case MPACK_TOKEN_UINT:
      obj->data->u = mpack_unpack_uint(node->tok);
      obj->type = ELEMENT_TYPE_UINT;
      break;
    case MPACK_TOKEN_SINT:
      obj->data->i = mpack_unpack_sint(node->tok);
      obj->type = ELEMENT_TYPE_SINT;
      break;
    case MPACK_TOKEN_FLOAT:
      obj->data->d = mpack_unpack_float(node->tok);
      obj->type = ELEMENT_TYPE_FLOAT;
      break;
    case MPACK_TOKEN_CHUNK:
      /* chunks should always follow string/bin/ext tokens */
      assert(unpacker->string_buffer);
      memcpy(unpacker->string_buffer + MPACK_PARENT_NODE(node)->pos,
             node->tok.data.chunk_ptr,
             node->tok.length);
      break;
    case MPACK_TOKEN_BIN:
    case MPACK_TOKEN_STR:
    case MPACK_TOKEN_EXT:
      unpacker->string_buffer = malloc(node->tok.length);
      assert(unpacker->string_buffer);
      break;
    case MPACK_TOKEN_ARRAY:
      element_create_array(obj, node->tok.length);
      obj->type = ELEMENT_TYPE_ARRAY;
      break;
    case MPACK_TOKEN_MAP:
      element_create_array(obj, node->tok.length);
      obj->type = ELEMENT_TYPE_MAP;
      break;
    default:
      obj->type = ELEMENT_TYPE_NULL;
      break;
  }
  node->data[0].p = obj;
}

static void lmpack_parse_exit(mpack_parser_t *parser, mpack_node_t *node)
{
  unpacker_t *unpacker = parser->data.p;
  mpack_node_t *parent = MPACK_PARENT_NODE(node);
  element_t *obj = node->data[0].p;

  switch (node->tok.type) {
    case MPACK_TOKEN_CHUNK:
      return;
    case MPACK_TOKEN_BIN:
    case MPACK_TOKEN_STR:
    case MPACK_TOKEN_EXT:
      obj->data->p = malloc(sizeof(*unpacker->string_buffer) *
                            node->tok.length);
      memcpy(obj->data->p, unpacker->string_buffer, node->tok.length);
      obj->length = node->tok.length;
      free(unpacker->string_buffer);
      unpacker->string_buffer = NULL;
      if (node->tok.type == MPACK_TOKEN_EXT) {
        obj->type = ELEMENT_TYPE_EXT;
        obj->ext_code = node->tok.data.ext_type;
      } else if (node->tok.type == MPACK_TOKEN_BIN) {
        obj->type = ELEMENT_TYPE_BIN;
      } else {
        obj->type = ELEMENT_TYPE_STR;
      }
      break;
    default:
      break;
  }

  if (parent) {
    element_t *parent_obj = parent->data[0].p;
    if (parent->tok.type == MPACK_TOKEN_ARRAY) {
      element_t *element = element_at_index(parent_obj, parent->pos - 1);
      element->data->p = obj;
    }
    if (parent->tok.type == MPACK_TOKEN_MAP) {
      if (parent->key_visited) {
        /* save key */
        parent->data[1].p = obj;
      } else {
        /* set pair using last saved key and current object as value */
        element_t *element = element_at_index(parent_obj, parent->pos - 1);
        pair_t *pair = malloc(sizeof(*pair));
        pair->key = parent->data[1].p;
        pair->value = obj;
        element->data->p = pair;
      }
    }
  } else {
    unpacker->root = obj;
  }
}

static int lmpack_unpacker_unpack_str(unpacker_t *unpacker,
                                      const char **str,
                                      size_t *length)
{
  int rv = 0;

  if (unpacker->unpacking) {
    printf("This unpacker instance is already working.\n");
    return -1;
  }

  do {
    unpacker->unpacking = 1;
    rv = mpack_parse(unpacker->parser,
                     str,
                     length,
                     lmpack_parse_enter,
                     lmpack_parse_exit);

    if (rv == MPACK_NOMEM) {
      unpacker->parser = lmpack_grow_parser(unpacker->parser);
      if (!unpacker->parser) {
        unpacker->unpacking = 0;
        printf("Failed to grow unpacker capacity.\n");
        return rv;
      }
    }
  } while (rv == MPACK_NOMEM);

  if (rv == MPACK_ERROR) {
    printf("Invalid msgpack string.\n");
  }

  unpacker->unpacking = 0;
  return rv;
}

static packer_t *lmpack_packer_new()
{
  packer_t *rv = malloc(sizeof(*rv));

  rv->parser = malloc(sizeof(*rv->parser));
  if (!rv->parser) {
    printf("Failed to allocate memory for packer parser.\n");
    return NULL;
  }
  mpack_parser_init(rv->parser, 0);
  rv->parser->data.p = rv;
  rv->packing = 0;
  rv->root = NULL;

  return rv;
}

static int lmpack_packer_delete(packer_t *packer)
{
  if (!packer)
    return 0;

  if (packer->root != NULL) {
    /* We avoid freeing an object constructed by the user. */
    packer->root = NULL;
  }
  free(packer->parser);
  free(packer);
  packer = NULL;
  return 0;
}

static void lmpack_unparse_enter(mpack_parser_t *parser, mpack_node_t *node)
{
  packer_t *packer = parser->data.p;
  mpack_node_t *parent = MPACK_PARENT_NODE(node);
  element_t *obj = NULL;

  if (parent) {
    /* get the parent */
    element_t *parent_obj = parent->data[0].p;

    if (parent->tok.type > MPACK_TOKEN_MAP) {
      /* strings are a special case, they are packed as single child chunk
       * node */
      node->tok = mpack_pack_chunk(parent_obj->data->p,
                                   parent->tok.length);
      return;
    }

    if (parent->tok.type == MPACK_TOKEN_ARRAY) {
      obj = element_at_index(parent_obj, parent->pos)->data->p;
    } else if (parent->tok.type == MPACK_TOKEN_MAP) {
      if (parent->key_visited) {
        element_t *entry = parent->data[1].p;
        pair_t *pair = entry->data->p;
        /* key has already been serialized, now do value */
        obj = pair->value;
      } else {
        element_t *entry = element_at_index(parent_obj, parent->pos);
        pair_t *pair = entry->data->p;
        /* store key-value pair for the next iteration where value is needed */
        parent->data[1].p = entry;
        /* serialize the key first */
        obj = pair->key;
      }
    }
  } else {
    obj = packer->root;
  }

  switch (obj->type) {
    case ELEMENT_TYPE_BOOLEAN:
      node->tok = mpack_pack_boolean(obj->data->u);
      break;
    case ELEMENT_TYPE_UINT:
      node->tok = mpack_pack_uint(obj->data->u);
      break;
    case ELEMENT_TYPE_SINT:
      node->tok = mpack_pack_sint(obj->data->i);
      break;
    case ELEMENT_TYPE_FLOAT:
      node->tok = mpack_pack_float(obj->data->d);
      break;
    case ELEMENT_TYPE_BIN:
      node->tok = mpack_pack_bin(obj->length);
      break;
    case ELEMENT_TYPE_STR:
      node->tok = mpack_pack_str(obj->length);
      break;
    case ELEMENT_TYPE_EXT:
      node->tok = mpack_pack_ext(obj->ext_code,
                                 obj->length);
      break;
    case ELEMENT_TYPE_ARRAY:
      node->tok = mpack_pack_array(obj->length);
      break;
    case ELEMENT_TYPE_MAP:
      node->tok = mpack_pack_map(obj->length);
      break;
    default:
      printf("Cannot serialize object.\n");
      node->tok = mpack_pack_nil();
      break;
  }

  node->data[0].p = obj;
}

static void lmpack_unparse_exit(mpack_parser_t *parser, mpack_node_t *node)
{
  if (node->tok.type != MPACK_TOKEN_CHUNK) {
    /* release the object */
    node->data[0].p = NULL;
  }
}

static int lmpack_packer_pack_obj(packer_t *packer,
                                  element_t *obj,
                                  char **str,
                                  size_t *length)
{
  int result = 1;
  char *final_buffer = NULL;
  char *buffer = NULL;
  char *buffer_cursor = NULL;
  size_t buffer_size = 16;
  size_t buffer_left = 0;

  buffer = malloc(sizeof(*buffer) * buffer_size);
  if (!buffer) {
    printf("Failed to allocate memory for buffer.\n");
    return -1;
  }
  buffer_cursor = buffer;
  buffer_left = buffer_size;
  *length = 0;

  if (packer->packing) {
    printf("This packer instance is already working. Use another packer, "
           "or the module's \"pack\" function.\n");
    return -1;
  }

  packer->root = obj;
  do {
    size_t buffer_left_init = buffer_left;
    packer->packing = 1;
    result = mpack_unparse(packer->parser,
                           &buffer_cursor,
                           &buffer_left,
                           lmpack_unparse_enter,
                           lmpack_unparse_exit);

    if (result == MPACK_NOMEM) {
      packer->parser = lmpack_grow_parser(packer->parser);
      if (!packer->parser) {
        packer->packing = 0;
        printf("Failed to grow packer capacity.\n");
        return -1;
      }
    }

    *length += buffer_left_init - buffer_left;

    if (!buffer_left) {
      /* buffer is empty, resize */
      char *new_buffer = malloc(sizeof(*new_buffer) * buffer_size * 2);
      memcpy(new_buffer, buffer, *length);
      free(buffer);
      buffer = new_buffer;
      buffer_size = buffer_size * 2;
      buffer_cursor = new_buffer + *length;
      buffer_left = buffer_size - *length;
    }
  } while (result == MPACK_EOF || result == MPACK_NOMEM);

  final_buffer = malloc(sizeof(*final_buffer) * (*length));
  memcpy(final_buffer, buffer, *length);
  free(buffer);
  *str = final_buffer;
  packer->packing = 0;
  return 1;
}

static element_t *lmpack_unpack(const char *str, size_t len)
{
  int result;
  unpacker_t *unpacker;
  element_t *output;

  unpacker = lmpack_unpacker_new();
  result = lmpack_unpacker_unpack_str(unpacker, &str, &len);

  if (result == MPACK_NOMEM) {
    printf("Object was too deep to unpack.\n");
    return NULL;
  } else if (result == MPACK_EOF) {
    printf("Incomplete msgpack string.\n");
    return NULL;
  } else if (result == MPACK_ERROR) {
    printf("Invalid msgpack string.\n");
    return NULL;
  } else if (result == MPACK_OK && len) {
    printf("Trailing data in msgpack string.\n");
    return NULL;
  }
  assert(result == MPACK_OK);

  output = unpacker->root;
  unpacker->root = NULL;
  lmpack_unpacker_delete(unpacker);
  return output;
}

static int lmpack_pack(element_t *obj, char **str)
{
  size_t length;
  int result;
  packer_t *packer;

  packer = lmpack_packer_new();
  result = lmpack_packer_pack_obj(packer, obj, str, &length);
  if (result == MPACK_EXCEPTION) {
    printf("An unexpected error occured.\n");
    return -1;
  }
  lmpack_packer_delete(packer);

  return length;
}

static session_t *lmpack_session_new(packer_t *packer, unpacker_t *unpacker)
{
  session_t *rv = malloc(sizeof(*rv));
  if (!rv) {
    printf("Failed to allocate memory for session.\n");
    return NULL;
  }

  rv->session = malloc(sizeof(*rv->session));
  if (!rv->session) {
    printf("Failed to allocate memory for session.\n");
    return NULL;
  }

  mpack_rpc_session_init(rv->session, 0);
  if (packer == NULL) {
    rv->packer = lmpack_packer_new();
  } else {
    rv->packer = packer;
  }
  if (unpacker == NULL) {
    rv->unpacker = lmpack_unpacker_new ();
  } else {
    rv->unpacker = unpacker;
  }
  rv->args_or_result = NULL;
  rv->method_or_error = NULL;
  rv->type = MPACK_EOF;

  return rv;
}

static int lmpack_session_delete(session_t *session)
{
  free(session->session);
  free(session);
  return 0;
}

static mpack_rpc_session_t *lmpack_session_grow(mpack_rpc_session_t *session)
{
  mpack_rpc_session_t *old = session;
  mpack_uint32_t new_capacity = old->capacity * 2;
  session = malloc(MPACK_RPC_SESSION_STRUCT_SIZE(new_capacity));
  if (!session) goto end;
  mpack_rpc_session_init(session, new_capacity);
  mpack_rpc_session_copy(session, old);
  free(old);
end:
  return session;
}

static void lmpack_transit_values_init(transit_values_t *values)
{
  values->pos = 0;
  values->msg_type = MPACK_EOF;
  values->method_or_error = NULL;
  values->args_or_result = NULL;
  values->msg_id = 0;
  values->msg_data = NULL;
}

static transit_values_t lmpack_session_receive(session_t *session,
                                               char *data,
                                               size_t length,
                                               size_t offset)
{
  transit_values_t values;
  lmpack_transit_values_init(&values);
  if (offset >= length) {
    printf("Offset must be less then the input string length.\n");
    return values;
  }

  const char *buffer_init = data;
  const char *buffer = data + offset;
  size_t buffer_length = length - offset;
  int done = 0;

  while (!done) {
    if (session->type == MPACK_EOF) {
      session->type = mpack_rpc_receive(session->session,
                                        &buffer,
                                        &buffer_length,
                                        &(session->msg));
      if (session->type == MPACK_EOF)
        break;
    }

    int result = lmpack_unpacker_unpack_str(session->unpacker,
                                            &buffer,
                                            &buffer_length);
    if (result == MPACK_EOF)
      break;

    if (session->method_or_error == NULL) {
      session->method_or_error = session->unpacker->root;
    } else {
      session->args_or_result = session->unpacker->root;
      done = 1;
    }
  }

  size_t pos = buffer - buffer_init;
  if (done) {
    void *me = session->method_or_error;
    void *ar = session->args_or_result;
    int t = session->type;
    session->method_or_error = NULL;
    session->args_or_result = NULL;
    session->type = MPACK_EOF;
    if (t == MPACK_RPC_REQUEST) {
      values.pos = pos;
      values.msg_type = t;
      values.method_or_error = me;
      values.args_or_result = ar;
      values.msg_id = session->msg.id;
    } else if (t == MPACK_RPC_RESPONSE) {
      values.pos = pos;
      values.msg_type = t;
      values.method_or_error = me;
      values.args_or_result = ar;
      values.msg_data = session->msg.data.p;
    } else if (t == MPACK_RPC_NOTIFICATION) {
      values.pos = pos;
      values.msg_type = t;
      values.method_or_error = me;
      values.args_or_result = ar;
    } else {
      printf("An unexpected error occurred while recieving msgpack data.\n");
    }
  }

  return values;
}

static size_t lmpack_session_send(session_t *session,
                                  transit_values_t values,
                                  char **ret)
{
  size_t buffer_start_size = 8;
  char *buffer = malloc(sizeof(*buffer) * buffer_start_size);
  char *buffer_init = buffer;
  size_t buffer_left = buffer_start_size;
  size_t pos = 0;
  mpack_data_t d;
  char *me_buffer = NULL;
  size_t me_buffer_size = 0;
  char *ar_buffer = NULL;
  size_t ar_buffer_size = 0;
  char *final_buffer = NULL;
  int result = -1;

  if (values.msg_type == MPACK_RPC_REQUEST)
    d.p = values.msg_data;

  while (1) {
    result = -1;
    if (values.msg_type == MPACK_RPC_REQUEST) {
      result = mpack_rpc_request(session->session,
                                 &buffer,
                                 &buffer_left,
                                 d);
    } else if (values.msg_type == MPACK_RPC_RESPONSE) {
      result = mpack_rpc_reply(session->session,
                               &buffer,
                               &buffer_left,
                               values.msg_id);
    } else if (values.msg_type == MPACK_RPC_NOTIFICATION) {
      result = mpack_rpc_notify(session->session,
                                &buffer,
                                &buffer_left);
    }

    if (result == MPACK_NOMEM) {
      session->session = lmpack_session_grow(session->session);
    } else {
      break;
    }
  }

  if (result != MPACK_OK) {
    printf("An unexpected error occurred while sending msgpack data.\n");
    return 0;
  }

  pos = buffer_start_size - buffer_left;
  lmpack_packer_pack_obj(session->packer,
                         values.method_or_error,
                         &me_buffer,
                         &me_buffer_size);
  lmpack_packer_pack_obj(session->packer,
                         values.args_or_result,
                         &ar_buffer,
                         &ar_buffer_size);
  final_buffer = malloc(sizeof(*final_buffer)
                        * (pos + me_buffer_size + ar_buffer_size));
  memcpy(final_buffer, buffer_init, pos);
  memcpy(final_buffer + pos, me_buffer, me_buffer_size);
  memcpy(final_buffer + pos + me_buffer_size, ar_buffer, ar_buffer_size);
  free(buffer_init);
  free(me_buffer);
  free(ar_buffer);
  *ret = final_buffer;
  return pos + me_buffer_size + ar_buffer_size;
}

static size_t lmpack_session_request(session_t *session,
                                     void *method,
                                     void *args,
                                     void *data,
                                     char **ret)
{
  transit_values_t values;
  lmpack_transit_values_init(&values);
  values.msg_type = MPACK_RPC_REQUEST;
  values.method_or_error = method;
  values.args_or_result = args;
  values.msg_data = data;
  return lmpack_session_send(session, values, ret);
}

static size_t lmpack_session_notify(session_t *session,
                                    void *method,
                                    void *args,
                                    char **ret)
{
  transit_values_t values;
  lmpack_transit_values_init(&values);
  values.msg_type = MPACK_RPC_NOTIFICATION;
  values.method_or_error = method;
  values.args_or_result = args;
  return lmpack_session_send(session, values, ret);
}

static size_t lmpack_session_reply(session_t *session,
                                   mpack_uint32_t request_id,
                                   void *data,
                                   int error,
                                   char **ret)
{
  transit_values_t values;
  lmpack_transit_values_init(&values);
  values.msg_type = MPACK_RPC_RESPONSE;
  values.msg_id = request_id;
  if (error) {
    values.method_or_error = data;
  } else {
    values.args_or_result = data;
  }
  return lmpack_session_send(session, values, ret);
}
