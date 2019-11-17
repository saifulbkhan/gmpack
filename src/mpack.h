#ifndef MPACK_CORE_H
#define MPACK_CORE_H

#ifndef MPACK_API
# define MPACK_API extern
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#ifdef __GNUC__
# define FPURE __attribute__((const))
# define FNONULL __attribute__((nonnull))
# define FNONULL_ARG(x) __attribute__((nonnull x))
# define FUNUSED __attribute__((unused))
#else
# define FPURE
# define FNONULL
# define FNONULL_ARG(x)
# define FUNUSED
#endif

#if UINT_MAX == 0xffffffff
typedef int mpack_sint32_t;
typedef unsigned int mpack_uint32_t;
#elif ULONG_MAX == 0xffffffff
typedef long mpack_sint32_t;
typedef unsigned long mpack_uint32_t;
#else
# error "can't find unsigned 32-bit integer type"
#endif

typedef struct mpack_value_s {
  mpack_uint32_t lo, hi;
} mpack_value_t;


enum {
  MPACK_OK = 0,
  MPACK_EOF = 1,
  MPACK_ERROR = 2
};

#define MPACK_MAX_TOKEN_LEN 9  /* 64-bit ints/floats plus type code */

typedef enum {
  MPACK_TOKEN_NIL       = 1,
  MPACK_TOKEN_BOOLEAN   = 2,
  MPACK_TOKEN_UINT      = 3,
  MPACK_TOKEN_SINT      = 4,
  MPACK_TOKEN_FLOAT     = 5,
  MPACK_TOKEN_CHUNK     = 6,
  MPACK_TOKEN_ARRAY     = 7,
  MPACK_TOKEN_MAP       = 8,
  MPACK_TOKEN_BIN       = 9,
  MPACK_TOKEN_STR       = 10,
  MPACK_TOKEN_EXT       = 11
} mpack_token_type_t;

typedef struct mpack_token_s {
  mpack_token_type_t type;  /* Type of token */
  mpack_uint32_t length;    /* Byte length for str/bin/ext/chunk/float/int/uint.
                               Item count for array/map. */
  union {
    mpack_value_t value;    /* 32-bit parts of primitives (bool,int,float) */
    const char *chunk_ptr;  /* Chunk of data from str/bin/ext */
    int ext_type;           /* Type field for ext tokens */
  } data;
} mpack_token_t;

typedef struct mpack_tokbuf_s {
  char pending[MPACK_MAX_TOKEN_LEN];
  mpack_token_t pending_tok;
  size_t ppos, plen;
  mpack_uint32_t passthrough;
} mpack_tokbuf_t;

#define MPACK_TOKBUF_INITIAL_VALUE { { 0 }, { 0, 0, { { 0, 0 } } }, 0, 0, 0 }

MPACK_API void mpack_tokbuf_init(mpack_tokbuf_t *tb) FUNUSED FNONULL;
MPACK_API int mpack_read(mpack_tokbuf_t *tb, const char **b, size_t *bl,
    mpack_token_t *tok) FUNUSED FNONULL;
MPACK_API int mpack_write(mpack_tokbuf_t *tb, char **b, size_t *bl,
    const mpack_token_t *tok) FUNUSED FNONULL;

#endif  /* MPACK_CORE_H */
#ifndef MPACK_CONV_H
#define MPACK_CONV_H


#if ULLONG_MAX == 0xffffffffffffffff
typedef long long mpack_sintmax_t;
typedef unsigned long long mpack_uintmax_t;
#elif UINT64_MAX == 0xffffffffffffffff
typedef int64_t mpack_sintmax_t;
typedef uint64_t mpack_uintmax_t;
#else
typedef mpack_sint32_t mpack_sintmax_t;
typedef mpack_uint32_t mpack_uintmax_t;
#endif

#ifndef bool
# define bool unsigned
#endif

MPACK_API mpack_token_t mpack_pack_nil(void) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_boolean(unsigned v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_uint(mpack_uintmax_t v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_sint(mpack_sintmax_t v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_float_compat(double v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_float_fast(double v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_number(double v) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_chunk(const char *p, mpack_uint32_t l)
  FUNUSED FPURE FNONULL;
MPACK_API mpack_token_t mpack_pack_str(mpack_uint32_t l) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_bin(mpack_uint32_t l) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_ext(int type, mpack_uint32_t l)
  FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_array(mpack_uint32_t l) FUNUSED FPURE;
MPACK_API mpack_token_t mpack_pack_map(mpack_uint32_t l) FUNUSED FPURE;
MPACK_API bool mpack_unpack_boolean(mpack_token_t t) FUNUSED FPURE;
MPACK_API mpack_uintmax_t mpack_unpack_uint(mpack_token_t t) FUNUSED FPURE;
MPACK_API mpack_sintmax_t mpack_unpack_sint(mpack_token_t t) FUNUSED FPURE;
MPACK_API double mpack_unpack_float_fast(mpack_token_t t) FUNUSED FPURE;
MPACK_API double mpack_unpack_float_compat(mpack_token_t t) FUNUSED FPURE;
MPACK_API double mpack_unpack_number(mpack_token_t t) FUNUSED FPURE;

/* The mpack_{pack,unpack}_float_fast functions should work in 99% of the
 * platforms. When compiling for a platform where floats don't use ieee754 as
 * the internal format, pass
 * -Dmpack_{pack,unpack}_float=mpack_{pack,unpack}_float_compat to the
 *  compiler.*/
#ifndef mpack_pack_float
# define mpack_pack_float mpack_pack_float_fast
#endif
#ifndef mpack_unpack_float
# define mpack_unpack_float mpack_unpack_float_fast
#endif

#endif  /* MPACK_CONV_H */
#ifndef MPACK_OBJECT_H
#define MPACK_OBJECT_H


#ifndef MPACK_MAX_OBJECT_DEPTH
# define MPACK_MAX_OBJECT_DEPTH 32
#endif

#define MPACK_PARENT_NODE(n) (((n) - 1)->pos == (size_t)-1 ? NULL : (n) - 1)

#define MPACK_THROW(parser)           \
  do {                                \
    parser->status = MPACK_EXCEPTION; \
    return;                           \
  } while (0)

enum {
  MPACK_EXCEPTION = -1,
  MPACK_NOMEM = MPACK_ERROR + 1
};

/* Storing integer in pointers in undefined behavior according to the C
 * standard. Define a union type to accomodate arbitrary user data associated
 * with nodes(and with requests in rpc.h). */
typedef union {
  void *p;
  mpack_uintmax_t u;
  mpack_sintmax_t i;
  double d;
} mpack_data_t;

typedef struct mpack_node_s {
  mpack_token_t tok;
  size_t pos;
  /* flag to determine if the key was visited when traversing a map */
  int key_visited;
  /* allow 2 instances mpack_data_t per node. the reason is that when
   * serializing, the user may need to keep track of traversal state besides the
   * parent node reference */
  mpack_data_t data[2];
} mpack_node_t;

#define MPACK_PARSER_STRUCT(c)      \
  struct {                          \
    mpack_data_t data;              \
    mpack_uint32_t size, capacity;  \
    int status;                     \
    int exiting;                    \
    mpack_tokbuf_t tokbuf;          \
    mpack_node_t items[c + 1];      \
  }

/* Some compilers warn against anonymous structs:
 * https://github.com/libmpack/libmpack/issues/6 */
typedef MPACK_PARSER_STRUCT(0) mpack_one_parser_t;

#define MPACK_PARSER_STRUCT_SIZE(c) \
  (sizeof(mpack_node_t) * c +       \
   sizeof(mpack_one_parser_t))

typedef MPACK_PARSER_STRUCT(MPACK_MAX_OBJECT_DEPTH) mpack_parser_t;
typedef void(*mpack_walk_cb)(mpack_parser_t *w, mpack_node_t *n);

MPACK_API void mpack_parser_init(mpack_parser_t *p, mpack_uint32_t c)
  FUNUSED FNONULL;

MPACK_API int mpack_parse_tok(mpack_parser_t *walker, mpack_token_t tok,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
  FUNUSED FNONULL_ARG((1,3,4));
MPACK_API int mpack_unparse_tok(mpack_parser_t *walker, mpack_token_t *tok,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
  FUNUSED FNONULL_ARG((1,2,3,4));

MPACK_API int mpack_parse(mpack_parser_t *parser, const char **b, size_t *bl,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
  FUNUSED FNONULL_ARG((1,2,3,4,5));
MPACK_API int mpack_unparse(mpack_parser_t *parser, char **b, size_t *bl,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
  FUNUSED FNONULL_ARG((1,2,3,4,5));

MPACK_API void mpack_parser_copy(mpack_parser_t *d, mpack_parser_t *s)
  FUNUSED FNONULL;

#endif  /* MPACK_OBJECT_H */
#ifndef MPACK_RPC_H
#define MPACK_RPC_H


#ifndef MPACK_RPC_MAX_REQUESTS
# define MPACK_RPC_MAX_REQUESTS 32
#endif

enum {
  MPACK_RPC_REQUEST = MPACK_NOMEM + 1,
  MPACK_RPC_RESPONSE,
  MPACK_RPC_NOTIFICATION,
  MPACK_RPC_ERROR
};

enum {
  MPACK_RPC_EARRAY = MPACK_RPC_ERROR,
  MPACK_RPC_EARRAYL,
  MPACK_RPC_ETYPE,
  MPACK_RPC_EMSGID,
  MPACK_RPC_ERESPID
};

typedef struct mpack_rpc_header_s {
  mpack_token_t toks[3];
  int index;
} mpack_rpc_header_t;

typedef struct mpack_rpc_message_s {
  mpack_uint32_t id;
  mpack_data_t data;
} mpack_rpc_message_t;

struct mpack_rpc_slot_s {
  int used;
  mpack_rpc_message_t msg;
};

#define MPACK_RPC_SESSION_STRUCT(c)      \
  struct {     \
    mpack_tokbuf_t reader, writer;       \
    mpack_rpc_header_t receive, send;    \
    mpack_uint32_t request_id, capacity; \
    struct mpack_rpc_slot_s slots[c];    \
  }

/* Some compilers warn against anonymous structs:
 * https://github.com/libmpack/libmpack/issues/6 */
typedef MPACK_RPC_SESSION_STRUCT(1) mpack_rpc_one_session_t;

#define MPACK_RPC_SESSION_STRUCT_SIZE(c)        \
  (sizeof(struct mpack_rpc_slot_s) * (c - 1) +  \
   sizeof(mpack_rpc_one_session_t))

typedef MPACK_RPC_SESSION_STRUCT(MPACK_RPC_MAX_REQUESTS) mpack_rpc_session_t;

MPACK_API void mpack_rpc_session_init(mpack_rpc_session_t *s, mpack_uint32_t c)
  FUNUSED FNONULL;

MPACK_API int mpack_rpc_receive_tok(mpack_rpc_session_t *s, mpack_token_t t,
    mpack_rpc_message_t *msg) FUNUSED FNONULL;
MPACK_API int mpack_rpc_request_tok(mpack_rpc_session_t *s, mpack_token_t *t,
    mpack_data_t d) FUNUSED FNONULL_ARG((1,2));
MPACK_API int mpack_rpc_reply_tok(mpack_rpc_session_t *s, mpack_token_t *t,
    mpack_uint32_t i) FUNUSED FNONULL;
MPACK_API int mpack_rpc_notify_tok(mpack_rpc_session_t *s, mpack_token_t *t)
  FUNUSED FNONULL;

MPACK_API int mpack_rpc_receive(mpack_rpc_session_t *s, const char **b,
    size_t *bl, mpack_rpc_message_t *m) FUNUSED FNONULL;
MPACK_API int mpack_rpc_request(mpack_rpc_session_t *s, char **b, size_t *bl,
    mpack_data_t d) FUNUSED FNONULL_ARG((1,2,3));
MPACK_API int mpack_rpc_reply(mpack_rpc_session_t *s, char **b, size_t *bl,
    mpack_uint32_t i) FNONULL FUNUSED;
MPACK_API int mpack_rpc_notify(mpack_rpc_session_t *s, char **b, size_t *bl)
  FNONULL FUNUSED;

MPACK_API void mpack_rpc_session_copy(mpack_rpc_session_t *d,
    mpack_rpc_session_t *s) FUNUSED FNONULL;

#endif  /* MPACK_RPC_H */
