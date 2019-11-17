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
#include <string.h>


#define UNUSED(p) (void)p;
#define ADVANCE(buf, buflen) ((*buflen)--, (unsigned char)*((*buf)++))
#define TLEN(val, range_start) ((mpack_uint32_t)(1 << (val - range_start)))
#ifndef MIN
# define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

static int mpack_rtoken(const char **buf, size_t *buflen,
    mpack_token_t *tok);
static int mpack_rpending(const char **b, size_t *nl, mpack_tokbuf_t *tb);
static int mpack_rvalue(mpack_token_type_t t, mpack_uint32_t l,
    const char **b, size_t *bl, mpack_token_t *tok);
static int mpack_rblob(mpack_token_type_t t, mpack_uint32_t l,
    const char **b, size_t *bl, mpack_token_t *tok);
static int mpack_wtoken(const mpack_token_t *tok, char **b, size_t *bl);
static int mpack_wpending(char **b, size_t *bl, mpack_tokbuf_t *tb);
static int mpack_wpint(char **b, size_t *bl, mpack_value_t v);
static int mpack_wnint(char **b, size_t *bl, mpack_value_t v);
static int mpack_wfloat(char **b, size_t *bl, const mpack_token_t *v);
static int mpack_wstr(char **buf, size_t *buflen, mpack_uint32_t len);
static int mpack_wbin(char **buf, size_t *buflen, mpack_uint32_t len);
static int mpack_wext(char **buf, size_t *buflen, int type,
    mpack_uint32_t len);
static int mpack_warray(char **buf, size_t *buflen, mpack_uint32_t len);
static int mpack_wmap(char **buf, size_t *buflen, mpack_uint32_t len);
static int mpack_w1(char **b, size_t *bl, mpack_uint32_t v);
static int mpack_w2(char **b, size_t *bl, mpack_uint32_t v);
static int mpack_w4(char **b, size_t *bl, mpack_uint32_t v);
static mpack_value_t mpack_byte(unsigned char b);
static int mpack_value(mpack_token_type_t t, mpack_uint32_t l,
    mpack_value_t v, mpack_token_t *tok);
static int mpack_blob(mpack_token_type_t t, mpack_uint32_t l, int et,
    mpack_token_t *tok);

MPACK_API void mpack_tokbuf_init(mpack_tokbuf_t *tokbuf)
{
  tokbuf->ppos = 0;
  tokbuf->plen = 0;
  tokbuf->passthrough = 0;
}

MPACK_API int mpack_read(mpack_tokbuf_t *tokbuf, const char **buf,
    size_t *buflen, mpack_token_t *tok)
{
  int status;
  size_t initial_ppos, ptrlen, advanced;
  const char *ptr, *ptr_save;
  assert(*buf && *buflen);

  if (tokbuf->passthrough) {
    /* pass data from str/bin/ext directly as a MPACK_TOKEN_CHUNK, adjusting
     * *buf and *buflen */
    tok->type = MPACK_TOKEN_CHUNK;
    tok->data.chunk_ptr = *buf;
    tok->length = MIN((mpack_uint32_t)*buflen, tokbuf->passthrough);
    tokbuf->passthrough -= tok->length;
    *buf += tok->length;
    *buflen -= tok->length;
    goto done;
  }

  initial_ppos = tokbuf->ppos;

  if (tokbuf->plen) {
    if (!mpack_rpending(buf, buflen, tokbuf)) {
      return MPACK_EOF;
    }
    ptr = tokbuf->pending;
    ptrlen = tokbuf->ppos;
  } else {
    ptr = *buf;
    ptrlen = *buflen;
  }

  ptr_save = ptr;

  if ((status = mpack_rtoken(&ptr, &ptrlen, tok))) {
    if (status != MPACK_EOF) return MPACK_ERROR;
    /* need more data */
    assert(!tokbuf->plen);
    /* read the remainder of *buf to tokbuf->pending so it can be parsed
     * later with more data. only required when tokbuf->plen == 0 or else
     * it would have been done already. */
    tokbuf->plen = tok->length + 1;
    assert(tokbuf->plen <= sizeof(tokbuf->pending));
    tokbuf->ppos = 0;
    status = mpack_rpending(buf, buflen, tokbuf);
    assert(!status);
    return MPACK_EOF;
  }

  advanced = (size_t)(ptr - ptr_save) - initial_ppos;
  tokbuf->plen = tokbuf->ppos = 0;
  *buflen -= advanced;
  *buf += advanced;

  if (tok->type > MPACK_TOKEN_MAP) {
    tokbuf->passthrough = tok->length;
  }

done:
  return MPACK_OK;
}

MPACK_API int mpack_write(mpack_tokbuf_t *tokbuf, char **buf, size_t *buflen,
    const mpack_token_t *t)
{
  int status;
  char *ptr;
  size_t ptrlen;
  mpack_token_t tok = tokbuf->plen ? tokbuf->pending_tok : *t;
  assert(*buf && *buflen);

  if (tok.type == MPACK_TOKEN_CHUNK) {
    size_t written, pending, count;
    if (!tokbuf->plen) tokbuf->ppos = 0;
    written = tokbuf->ppos;
    pending = tok.length - written;
    count = MIN(pending, *buflen);
    memcpy(*buf, tok.data.chunk_ptr + written, count);
    *buf += count;
    *buflen -= count;
    tokbuf->ppos += count;
    tokbuf->plen = count == pending ? 0 : tok.length;
    if (count == pending) {
      return MPACK_OK;
    } else {
      tokbuf->pending_tok = tok;
      return MPACK_EOF;
    }
  }

  if (tokbuf->plen) return mpack_wpending(buf, buflen, tokbuf);

  if (*buflen < MPACK_MAX_TOKEN_LEN) {
    ptr = tokbuf->pending;
    ptrlen = sizeof(tokbuf->pending);
  } else {
    ptr = *buf;
    ptrlen = *buflen;
  }

  if ((status = mpack_wtoken(&tok, &ptr, &ptrlen))) return status;

  if (*buflen < MPACK_MAX_TOKEN_LEN) {
    size_t toklen = sizeof(tokbuf->pending) - ptrlen;
    size_t write_cnt = MIN(toklen, *buflen);
    memcpy(*buf, tokbuf->pending, write_cnt);
    *buf += write_cnt;
    *buflen -= write_cnt;
    if (write_cnt < toklen) {
      assert(!*buflen);
      tokbuf->plen = toklen;
      tokbuf->ppos = write_cnt;
      tokbuf->pending_tok = tok;
      return MPACK_EOF;
    }
  } else {
    *buflen -= (size_t)(ptr - *buf);
    *buf = ptr;
  }

  return MPACK_OK;
}

static int mpack_rtoken(const char **buf, size_t *buflen,
    mpack_token_t *tok)
{
  unsigned char t = ADVANCE(buf, buflen);
  if (t < 0x80) {
    /* positive fixint */
    return mpack_value(MPACK_TOKEN_UINT, 1, mpack_byte(t), tok);
  } else if (t < 0x90) {
    /* fixmap */
    return mpack_blob(MPACK_TOKEN_MAP, t & 0xf, 0, tok);
  } else if (t < 0xa0) {
    /* fixarray */
    return mpack_blob(MPACK_TOKEN_ARRAY, t & 0xf, 0, tok);
  } else if (t < 0xc0) {
    /* fixstr */
    return mpack_blob(MPACK_TOKEN_STR, t & 0x1f, 0, tok);
  } else if (t < 0xe0) {
    switch (t) {
      case 0xc0:  /* nil */
        return mpack_value(MPACK_TOKEN_NIL, 0, mpack_byte(0), tok);
      case 0xc2:  /* false */
        return mpack_value(MPACK_TOKEN_BOOLEAN, 1, mpack_byte(0), tok);
      case 0xc3:  /* true */
        return mpack_value(MPACK_TOKEN_BOOLEAN, 1, mpack_byte(1), tok);
      case 0xc4:  /* bin 8 */
      case 0xc5:  /* bin 16 */
      case 0xc6:  /* bin 32 */
        return mpack_rblob(MPACK_TOKEN_BIN, TLEN(t, 0xc4), buf, buflen, tok);
      case 0xc7:  /* ext 8 */
      case 0xc8:  /* ext 16 */
      case 0xc9:  /* ext 32 */
        return mpack_rblob(MPACK_TOKEN_EXT, TLEN(t, 0xc7), buf, buflen, tok);
      case 0xca:  /* float 32 */
      case 0xcb:  /* float 64 */
        return mpack_rvalue(MPACK_TOKEN_FLOAT, TLEN(t, 0xc8), buf, buflen, tok);
      case 0xcc:  /* uint 8 */
      case 0xcd:  /* uint 16 */
      case 0xce:  /* uint 32 */
      case 0xcf:  /* uint 64 */
        return mpack_rvalue(MPACK_TOKEN_UINT, TLEN(t, 0xcc), buf, buflen, tok);
      case 0xd0:  /* int 8 */
      case 0xd1:  /* int 16 */
      case 0xd2:  /* int 32 */
      case 0xd3:  /* int 64 */
        return mpack_rvalue(MPACK_TOKEN_SINT, TLEN(t, 0xd0), buf, buflen, tok);
      case 0xd4:  /* fixext 1 */
      case 0xd5:  /* fixext 2 */
      case 0xd6:  /* fixext 4 */
      case 0xd7:  /* fixext 8 */
      case 0xd8:  /* fixext 16 */
        if (*buflen == 0) {
          /* require only one extra byte for the type code */
          tok->length = 1;
          return MPACK_EOF;
        }
        tok->length = TLEN(t, 0xd4);
        tok->type = MPACK_TOKEN_EXT;
        tok->data.ext_type = ADVANCE(buf, buflen);
        return MPACK_OK;
      case 0xd9:  /* str 8 */
      case 0xda:  /* str 16 */
      case 0xdb:  /* str 32 */
        return mpack_rblob(MPACK_TOKEN_STR, TLEN(t, 0xd9), buf, buflen, tok);
      case 0xdc:  /* array 16 */
      case 0xdd:  /* array 32 */
        return mpack_rblob(MPACK_TOKEN_ARRAY, TLEN(t, 0xdb), buf, buflen, tok);
      case 0xde:  /* map 16 */
      case 0xdf:  /* map 32 */
        return mpack_rblob(MPACK_TOKEN_MAP, TLEN(t, 0xdd), buf, buflen, tok);
      default:
        return MPACK_ERROR;
    }
  } else {
    /* negative fixint */
    return mpack_value(MPACK_TOKEN_SINT, 1, mpack_byte(t), tok);
  }
}

static int mpack_rpending(const char **buf, size_t *buflen,
    mpack_tokbuf_t *state)
{
  size_t count;
  assert(state->ppos < state->plen);
  count = MIN(state->plen - state->ppos, *buflen);
  memcpy(state->pending + state->ppos, *buf, count);
  state->ppos += count;
  if (state->ppos < state->plen) {
    /* consume buffer since no token will be parsed yet. */
    *buf += *buflen;
    *buflen = 0;
    return 0;
  }
  return 1;
}

static int mpack_rvalue(mpack_token_type_t type, mpack_uint32_t remaining,
    const char **buf, size_t *buflen, mpack_token_t *tok)
{
  if (*buflen < remaining) {
    tok->length = remaining;
    return MPACK_EOF;
  }

  mpack_value(type, remaining, mpack_byte(0), tok);

  while (remaining) {
    mpack_uint32_t byte = ADVANCE(buf, buflen), byte_idx, byte_shift;
    byte_idx = (mpack_uint32_t)--remaining;
    byte_shift = (byte_idx % 4) * 8;
    tok->data.value.lo |= byte << byte_shift;
    if (remaining == 4) {
      /* unpacked the first half of a 8-byte value, shift what was parsed to the
       * "hi" field and reset "lo" for the trailing 4 bytes. */
      tok->data.value.hi = tok->data.value.lo;
      tok->data.value.lo = 0;
    }
  }

  if (type == MPACK_TOKEN_SINT) {
    mpack_uint32_t hi = tok->data.value.hi;
    mpack_uint32_t lo = tok->data.value.lo;
    mpack_uint32_t msb = (tok->length == 8 && hi >> 31) ||
                         (tok->length == 4 && lo >> 31) ||
                         (tok->length == 2 && lo >> 15) ||
                         (tok->length == 1 && lo >> 7);
    if (!msb) {
      tok->type = MPACK_TOKEN_UINT;
    }
  }

  return MPACK_OK;
}

static int mpack_rblob(mpack_token_type_t type, mpack_uint32_t tlen,
    const char **buf, size_t *buflen, mpack_token_t *tok)
{
  mpack_token_t l;
  mpack_uint32_t required = tlen + (type == MPACK_TOKEN_EXT ? 1 : 0);

  if (*buflen < required) {
    tok->length = required;
    return MPACK_EOF;
  }

  l.data.value.lo = 0;
  mpack_rvalue(MPACK_TOKEN_UINT, tlen, buf, buflen, &l);
  tok->type = type;
  tok->length = l.data.value.lo;

  if (type == MPACK_TOKEN_EXT) {
    tok->data.ext_type = ADVANCE(buf, buflen);
  }

  return MPACK_OK;
}

static int mpack_wtoken(const mpack_token_t *tok, char **buf,
    size_t *buflen)
{
  switch (tok->type) {
    case MPACK_TOKEN_NIL:
      return mpack_w1(buf, buflen, 0xc0);
    case MPACK_TOKEN_BOOLEAN:
      return mpack_w1(buf, buflen, tok->data.value.lo ? 0xc3 : 0xc2);
    case MPACK_TOKEN_UINT:
      return mpack_wpint(buf, buflen, tok->data.value);
    case MPACK_TOKEN_SINT:
      return mpack_wnint(buf, buflen, tok->data.value);
    case MPACK_TOKEN_FLOAT:
      return mpack_wfloat(buf, buflen, tok);
    case MPACK_TOKEN_BIN:
      return mpack_wbin(buf, buflen, tok->length);
    case MPACK_TOKEN_STR:
      return mpack_wstr(buf, buflen, tok->length);
    case MPACK_TOKEN_EXT:
      return mpack_wext(buf, buflen, tok->data.ext_type, tok->length);
    case MPACK_TOKEN_ARRAY:
      return mpack_warray(buf, buflen, tok->length);
    case MPACK_TOKEN_MAP:
      return mpack_wmap(buf, buflen, tok->length);
    default:
      return MPACK_ERROR;
  }
}

static int mpack_wpending(char **buf, size_t *buflen, mpack_tokbuf_t *state)
{
  size_t count;
  assert(state->ppos < state->plen);
  count = MIN(state->plen - state->ppos, *buflen);
  memcpy(*buf, state->pending + state->ppos, count);
  state->ppos += count;
  *buf += count;
  *buflen -= count;
  if (state->ppos == state->plen) {
    state->plen = 0;
    return MPACK_OK;
  }
  return MPACK_EOF;
}

static int mpack_wpint(char **buf, size_t *buflen, mpack_value_t val)
{
  mpack_uint32_t hi = val.hi;
  mpack_uint32_t lo = val.lo;

  if (hi) {
    /* uint 64 */
    return mpack_w1(buf, buflen, 0xcf) ||
           mpack_w4(buf, buflen, hi)   ||
           mpack_w4(buf, buflen, lo);
  } else if (lo > 0xffff) {
    /* uint 32 */
    return mpack_w1(buf, buflen, 0xce) ||
           mpack_w4(buf, buflen, lo);
  } else if (lo > 0xff) {
    /* uint 16 */
    return mpack_w1(buf, buflen, 0xcd) ||
           mpack_w2(buf, buflen, lo);
  } else if (lo > 0x7f) {
    /* uint 8 */
    return mpack_w1(buf, buflen, 0xcc) ||
           mpack_w1(buf, buflen, lo);
  } else {
    return mpack_w1(buf, buflen, lo);
  }
}

static int mpack_wnint(char **buf, size_t *buflen, mpack_value_t val)
{
  mpack_uint32_t hi = val.hi;
  mpack_uint32_t lo = val.lo;

  if (lo < 0x80000000) {
    /* int 64 */
    return mpack_w1(buf, buflen, 0xd3) ||
           mpack_w4(buf, buflen, hi)   ||
           mpack_w4(buf, buflen, lo);
  } else if (lo < 0xffff7fff) {
    /* int 32 */
    return mpack_w1(buf, buflen, 0xd2) ||
           mpack_w4(buf, buflen, lo);
  } else if (lo < 0xffffff7f) {
    /* int 16 */
    return mpack_w1(buf, buflen, 0xd1) ||
           mpack_w2(buf, buflen, lo);
  } else if (lo < 0xffffffe0) {
    /* int 8 */
    return mpack_w1(buf, buflen, 0xd0) ||
           mpack_w1(buf, buflen, lo);
  } else {
    /* negative fixint */
    return mpack_w1(buf, buflen, (mpack_uint32_t)(0x100 + lo));
  }
}

static int mpack_wfloat(char **buf, size_t *buflen,
    const mpack_token_t *tok)
{
  if (tok->length == 4) {
    return mpack_w1(buf, buflen, 0xca) ||
           mpack_w4(buf, buflen, tok->data.value.lo);
  } else if (tok->length == 8) {
    return mpack_w1(buf, buflen, 0xcb) ||
           mpack_w4(buf, buflen, tok->data.value.hi) ||
           mpack_w4(buf, buflen, tok->data.value.lo);
  } else {
    return MPACK_ERROR;
  }
}

static int mpack_wstr(char **buf, size_t *buflen, mpack_uint32_t len)
{
  if (len < 0x20) {
    return mpack_w1(buf, buflen, 0xa0 | len);
  } else if (len < 0x100) {
    return mpack_w1(buf, buflen, 0xd9) ||
           mpack_w1(buf, buflen, len);
  } else if (len < 0x10000) {
    return mpack_w1(buf, buflen, 0xda) ||
           mpack_w2(buf, buflen, len);
  } else {
    return mpack_w1(buf, buflen, 0xdb) ||
           mpack_w4(buf, buflen, len);
  }
}

static int mpack_wbin(char **buf, size_t *buflen, mpack_uint32_t len)
{
  if (len < 0x100) {
    return mpack_w1(buf, buflen, 0xc4) ||
           mpack_w1(buf, buflen, len);
  } else if (len < 0x10000) {
    return mpack_w1(buf, buflen, 0xc5) ||
           mpack_w2(buf, buflen, len);
  } else {
    return mpack_w1(buf, buflen, 0xc6) ||
           mpack_w4(buf, buflen, len);
  }
}

static int mpack_wext(char **buf, size_t *buflen, int type,
    mpack_uint32_t len)
{
  mpack_uint32_t t;
  assert(type >= 0 && type < 0x80);
  t = (mpack_uint32_t)type;
  switch (len) {
    case 1: mpack_w1(buf, buflen, 0xd4); return mpack_w1(buf, buflen, t);
    case 2: mpack_w1(buf, buflen, 0xd5); return mpack_w1(buf, buflen, t);
    case 4: mpack_w1(buf, buflen, 0xd6); return mpack_w1(buf, buflen, t);
    case 8: mpack_w1(buf, buflen, 0xd7); return mpack_w1(buf, buflen, t);
    case 16: mpack_w1(buf, buflen, 0xd8); return mpack_w1(buf, buflen, t);
    default:
      if (len < 0x100) {
        return mpack_w1(buf, buflen, 0xc7) ||
               mpack_w1(buf, buflen, len)  ||
               mpack_w1(buf, buflen, t);
      } else if (len < 0x10000) {
        return mpack_w1(buf, buflen, 0xc8) ||
               mpack_w2(buf, buflen, len)  ||
               mpack_w1(buf, buflen, t);
      } else {
        return mpack_w1(buf, buflen, 0xc9) ||
               mpack_w4(buf, buflen, len)  ||
               mpack_w1(buf, buflen, t);
      }
  }
}

static int mpack_warray(char **buf, size_t *buflen, mpack_uint32_t len)
{
  if (len < 0x10) {
    return mpack_w1(buf, buflen, 0x90 | len);
  } else if (len < 0x10000) {
    return mpack_w1(buf, buflen, 0xdc) ||
           mpack_w2(buf, buflen, len);
  } else {
    return mpack_w1(buf, buflen, 0xdd) ||
           mpack_w4(buf, buflen, len);
  }
}

static int mpack_wmap(char **buf, size_t *buflen, mpack_uint32_t len)
{
  if (len < 0x10) {
    return mpack_w1(buf, buflen, 0x80 | len);
  } else if (len < 0x10000) {
    return mpack_w1(buf, buflen, 0xde) ||
           mpack_w2(buf, buflen, len);
  } else {
    return mpack_w1(buf, buflen, 0xdf) ||
           mpack_w4(buf, buflen, len);
  }
}

static int mpack_w1(char **b, size_t *bl, mpack_uint32_t v)
{
  (*bl)--;
  *(*b)++ = (char)(v & 0xff);
  return MPACK_OK;
}

static int mpack_w2(char **b, size_t *bl, mpack_uint32_t v)
{
  *bl -= 2;
  *(*b)++ = (char)((v >> 8) & 0xff);
  *(*b)++ = (char)(v & 0xff);
  return MPACK_OK;
}

static int mpack_w4(char **b, size_t *bl, mpack_uint32_t v)
{
  *bl -= 4;
  *(*b)++ = (char)((v >> 24) & 0xff);
  *(*b)++ = (char)((v >> 16) & 0xff);
  *(*b)++ = (char)((v >> 8) & 0xff);
  *(*b)++ = (char)(v & 0xff);
  return MPACK_OK;
}

static int mpack_value(mpack_token_type_t type, mpack_uint32_t length,
    mpack_value_t value, mpack_token_t *tok)
{
  tok->type = type;
  tok->length = length;
  tok->data.value = value;
  return MPACK_OK;
}

static int mpack_blob(mpack_token_type_t type, mpack_uint32_t length,
    int ext_type, mpack_token_t *tok)
{
  tok->type = type;
  tok->length = length;
  tok->data.ext_type = ext_type;
  return MPACK_OK;
}

static mpack_value_t mpack_byte(unsigned char byte)
{
  mpack_value_t rv;
  rv.lo = byte;
  rv.hi = 0;
  return rv;
}

static int mpack_fits_single(double v);
static mpack_value_t mpack_pack_ieee754(double v, unsigned m, unsigned e);
static int mpack_is_be(void) FPURE;
static double mpack_fmod_pow2_32(double a);


#define POW2(n) \
  ((double)(1 << (n / 2)) * (double)(1 << (n / 2)) * (double)(1 << (n % 2)))

#define MPACK_SWAP_VALUE(val)                                  \
  do {                                                         \
    mpack_uint32_t lo = val.lo;                                \
    val.lo = val.hi;                                           \
    val.hi = lo;                                               \
  } while (0)

MPACK_API mpack_token_t mpack_pack_nil(void)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_NIL;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_boolean(unsigned v)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_BOOLEAN;
  rv.data.value.lo = v ? 1 : 0;
  rv.data.value.hi = 0;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_uint(mpack_uintmax_t v)
{
  mpack_token_t rv;
  rv.data.value.lo = v & 0xffffffff;
  rv.data.value.hi = (mpack_uint32_t)((v >> 31) >> 1);
  rv.type = MPACK_TOKEN_UINT;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_sint(mpack_sintmax_t v)
{
  if (v < 0) {
    mpack_token_t rv;
    mpack_uintmax_t tc = -((mpack_uintmax_t)(v + 1)) + 1;
    tc = ~tc + 1;
    rv = mpack_pack_uint(tc);
    rv.type = MPACK_TOKEN_SINT;
    return rv;
  }

  return mpack_pack_uint((mpack_uintmax_t)v);
}

MPACK_API mpack_token_t mpack_pack_float_compat(double v)
{
  /* ieee754 single-precision limits to determine if "v" can be fully
   * represented in 4 bytes */
  mpack_token_t rv;

  if (mpack_fits_single(v)) {
    rv.length = 4;
    rv.data.value = mpack_pack_ieee754(v, 23, 8);
  } else {
    rv.length = 8;
    rv.data.value = mpack_pack_ieee754(v, 52, 11);
  }

  rv.type = MPACK_TOKEN_FLOAT;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_float_fast(double v)
{
  /* ieee754 single-precision limits to determine if "v" can be fully
   * represented in 4 bytes */
  mpack_token_t rv;

  if (mpack_fits_single(v)) {
    union {
      float f;
      mpack_uint32_t m;
    } conv;
    conv.f = (float)v;
    rv.length = 4;
    rv.data.value.lo = conv.m;
    rv.data.value.hi = 0;
  } else {
    union {
      double d;
      mpack_value_t m;
    } conv;
    conv.d = v;
    rv.length = 8;
    rv.data.value = conv.m;
    if (mpack_is_be()) {
      MPACK_SWAP_VALUE(rv.data.value);
    }
  }

  rv.type = MPACK_TOKEN_FLOAT;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_number(double v)
{
  mpack_token_t tok;
  double vabs;
  vabs = v < 0 ? -v : v;
  assert(v <= 9007199254740991. && v >= -9007199254740991.);
  tok.data.value.hi = (mpack_uint32_t)(vabs / POW2(32));
  tok.data.value.lo = (mpack_uint32_t)mpack_fmod_pow2_32(vabs);

  if (v < 0) {
    /* Compute the two's complement */
    tok.type = MPACK_TOKEN_SINT;
    tok.data.value.hi = ~tok.data.value.hi;
    tok.data.value.lo = ~tok.data.value.lo + 1;
    if (!tok.data.value.lo) tok.data.value.hi++;
    if (tok.data.value.lo == 0 && tok.data.value.hi == 0) tok.length = 1;
    else if (tok.data.value.lo < 0x80000000) tok.length = 8;
    else if (tok.data.value.lo < 0xffff7fff) tok.length = 4;
    else if (tok.data.value.lo < 0xffffff7f) tok.length = 2;
    else tok.length = 1;
  } else {
    tok.type = MPACK_TOKEN_UINT;
    if (tok.data.value.hi) tok.length = 8;
    else if (tok.data.value.lo > 0xffff) tok.length = 4;
    else if (tok.data.value.lo > 0xff) tok.length = 2;
    else tok.length = 1;
  }

  if (mpack_unpack_number(tok) != v) {
    return mpack_pack_float(v);
  }

  return tok;
}

MPACK_API mpack_token_t mpack_pack_chunk(const char *p, mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_CHUNK;
  rv.data.chunk_ptr = p;
  rv.length = l;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_str(mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_STR;
  rv.length = l;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_bin(mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_BIN;
  rv.length = l;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_ext(int t, mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_EXT;
  rv.length = l;
  rv.data.ext_type = t;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_array(mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_ARRAY;
  rv.length = l;
  return rv;
}

MPACK_API mpack_token_t mpack_pack_map(mpack_uint32_t l)
{
  mpack_token_t rv;
  rv.type = MPACK_TOKEN_MAP;
  rv.length = l;
  return rv;
}

MPACK_API bool mpack_unpack_boolean(mpack_token_t t)
{
  return t.data.value.lo || t.data.value.hi;
}

MPACK_API mpack_uintmax_t mpack_unpack_uint(mpack_token_t t)
{
  return (((mpack_uintmax_t)t.data.value.hi << 31) << 1) | t.data.value.lo;
}

/* unpack signed integer without relying on two's complement as internal
 * representation */
MPACK_API mpack_sintmax_t mpack_unpack_sint(mpack_token_t t)
{
  mpack_uint32_t hi = t.data.value.hi;
  mpack_uint32_t lo = t.data.value.lo;
  mpack_uintmax_t rv = lo;
  assert(t.length <= sizeof(mpack_sintmax_t));

  if (t.length == 8) {
    rv |= (((mpack_uintmax_t)hi) << 31) << 1;
  }
  /* reverse the two's complement so that lo/hi contain the absolute value.
   * note that we have to mask ~rv so that it reflects the two's complement
   * of the appropriate byte length */
  rv = (~rv & (((mpack_uintmax_t)1 << ((t.length * 8) - 1)) - 1)) + 1;
  /* negate and return the absolute value, making sure mpack_sintmax_t can
   * represent the positive cast. */
  return -((mpack_sintmax_t)(rv - 1)) - 1;
}

MPACK_API double mpack_unpack_float_compat(mpack_token_t t)
{
  mpack_uint32_t sign;
  mpack_sint32_t exponent, bias;
  unsigned mantbits;
  unsigned expbits;
  double mant;

  if (t.data.value.lo == 0 && t.data.value.hi == 0)
    /* nothing to do */
    return 0;

  if (t.length == 4) mantbits = 23, expbits = 8;
  else mantbits = 52, expbits = 11;
  bias = (1 << (expbits - 1)) - 1;

  /* restore sign/exponent/mantissa */
  if (mantbits == 52) {
    sign = t.data.value.hi >> 31;
    exponent = (t.data.value.hi >> 20) & ((1 << 11) - 1);
    mant = (t.data.value.hi & ((1 << 20) - 1)) * POW2(32);
    mant += t.data.value.lo;
  } else {
    sign = t.data.value.lo >> 31;
    exponent = (t.data.value.lo >> 23) & ((1 << 8) - 1);
    mant = t.data.value.lo & ((1 << 23) - 1);
  }

  mant /= POW2(mantbits);
  if (exponent) mant += 1.0; /* restore leading 1 */
  else exponent = 1; /* subnormal */
  exponent -= bias;

  /* restore original value */
  while (exponent > 0) mant *= 2.0, exponent--;
  while (exponent < 0) mant /= 2.0, exponent++;
  return mant * (sign ? -1 : 1);
}

MPACK_API double mpack_unpack_float_fast(mpack_token_t t)
{
  if (t.length == 4) {
    union {
      float f;
      mpack_uint32_t m;
    } conv;
    conv.m = t.data.value.lo;
    return conv.f;
  } else {
    union {
      double d;
      mpack_value_t m;
    } conv;
    conv.m = t.data.value;

    if (mpack_is_be()) {
      MPACK_SWAP_VALUE(conv.m);
    }

    return conv.d;
  }
}

MPACK_API double mpack_unpack_number(mpack_token_t t)
{
  double rv;
  mpack_uint32_t hi, lo;
  if (t.type == MPACK_TOKEN_FLOAT) return mpack_unpack_float(t);
  assert(t.type == MPACK_TOKEN_UINT || t.type == MPACK_TOKEN_SINT);
  hi = t.data.value.hi;
  lo = t.data.value.lo;
  if (t.type == MPACK_TOKEN_SINT) {
    /* same idea as mpack_unpack_sint, except here we shouldn't rely on
     * mpack_uintmax_t having 64-bits, operating on the 32-bit words separately.
     */
    if (!hi) {
      assert(t.length <= 4);
      hi = 0;
      lo = (~lo & (((mpack_uint32_t)1 << ((t.length * 8) - 1)) - 1));
    } else {
      hi = ~hi;
      lo = ~lo;
    }
    lo++;
    if (!lo) hi++;
  }
  rv = (double)lo + POW2(32) * hi;
  return t.type == MPACK_TOKEN_SINT ? -rv : rv;
}

static int mpack_fits_single(double v)
{
  return (float)v == v;
}

static mpack_value_t mpack_pack_ieee754(double v, unsigned mantbits,
    unsigned expbits)
{
  mpack_value_t rv = {0, 0};
  mpack_sint32_t exponent, bias = (1 << (expbits - 1)) - 1;
  mpack_uint32_t sign;
  double mant;

  if (v == 0) {
    rv.lo = 0;
    rv.hi = 0;
    goto end;
  }

  if (v < 0) sign = 1, mant = -v;
  else sign = 0, mant = v;

  exponent = 0;
  while (mant >= 2.0) mant /= 2.0, exponent++;
  while (mant < 1.0 && exponent > -(bias - 1)) mant *= 2.0, exponent--;

  if (mant < 1.0) exponent = -bias; /* subnormal value */
  else mant = mant - 1.0; /* remove leading 1 */
  exponent += bias;
  mant *= POW2(mantbits);

  if (mantbits == 52) {
    rv.hi = (mpack_uint32_t)(mant / POW2(32));
    rv.lo = (mpack_uint32_t)(mant - rv.hi * POW2(32));
    rv.hi |= ((mpack_uint32_t)exponent << 20) | (sign << 31);
  } else if (mantbits == 23) {
    rv.hi = 0;
    rv.lo = (mpack_uint32_t)mant;
    rv.lo |= ((mpack_uint32_t)exponent << 23) | (sign << 31);
  }

end:
  return rv;
}

static int mpack_is_be(void)
{
  union {
    mpack_uint32_t i;
    char c[sizeof(mpack_uint32_t)];
  } test;

  test.i = 1;
  return test.c[0] == 0;
}

/* this simplified version of `fmod` that returns the remainder of double
 * division by 0xffffffff, which is enough for our purposes */
static double mpack_fmod_pow2_32(double a)
{
  return a - ((double)(mpack_uint32_t)(a / POW2(32)) * POW2(32));
}
#include <string.h>


static int mpack_parser_full(mpack_parser_t *w);
static mpack_node_t *mpack_parser_push(mpack_parser_t *w);
static mpack_node_t *mpack_parser_pop(mpack_parser_t *w);

MPACK_API void mpack_parser_init(mpack_parser_t *parser,
    mpack_uint32_t capacity)
{
  mpack_tokbuf_init(&parser->tokbuf);
  parser->data.p = NULL;
  parser->capacity = capacity ? capacity : MPACK_MAX_OBJECT_DEPTH;
  parser->size = 0;
  parser->exiting = 0;
  memset(parser->items, 0, sizeof(mpack_node_t) * (parser->capacity + 1));
  parser->items[0].pos = (size_t)-1;
  parser->status = 0;
}

#define MPACK_EXCEPTION_CHECK(parser)                                           \
  do {                                                                      \
    if (parser->status == MPACK_EXCEPTION) {                                    \
      return MPACK_EXCEPTION;                                                   \
    }                                                                       \
  } while (0)

#define MPACK_WALK(action)                                                  \
  do {                                                                      \
    mpack_node_t *n;                                                        \
                                                                            \
    if (parser->exiting) goto exit;                                         \
    if (mpack_parser_full(parser)) return MPACK_NOMEM;                      \
    n = mpack_parser_push(parser);                                          \
    action;                                                                 \
    MPACK_EXCEPTION_CHECK(parser);                                              \
    parser->exiting = 1;                                                    \
    return MPACK_EOF;                                                       \
                                                                            \
exit:                                                                       \
    parser->exiting = 0;                                                    \
    while ((n = mpack_parser_pop(parser))) {                                \
      exit_cb(parser, n);                                                   \
      MPACK_EXCEPTION_CHECK(parser);                                            \
      if (!parser->size) return MPACK_OK;                                   \
    }                                                                       \
                                                                            \
    return MPACK_EOF;                                                       \
  } while (0)

MPACK_API int mpack_parse_tok(mpack_parser_t *parser, mpack_token_t tok,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
{
  MPACK_EXCEPTION_CHECK(parser);
  MPACK_WALK({n->tok = tok; enter_cb(parser, n);});
}

MPACK_API int mpack_unparse_tok(mpack_parser_t *parser, mpack_token_t *tok,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
{
  MPACK_EXCEPTION_CHECK(parser);
  MPACK_WALK({enter_cb(parser, n); *tok = n->tok;});
}

MPACK_API int mpack_parse(mpack_parser_t *parser, const char **buf,
    size_t *buflen, mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
{
  int status = MPACK_EOF;
  MPACK_EXCEPTION_CHECK(parser);

  while (*buflen && status) {
    mpack_token_t tok;
    mpack_tokbuf_t *tb = &parser->tokbuf;
    const char *buf_save = *buf;
    size_t buflen_save = *buflen;

    if ((status = mpack_read(tb, buf, buflen, &tok)) == MPACK_EOF) continue;
    else if (status == MPACK_ERROR) goto rollback;

    do {
      status = mpack_parse_tok(parser, tok, enter_cb, exit_cb);
      MPACK_EXCEPTION_CHECK(parser);
    } while (parser->exiting);

    if (status != MPACK_NOMEM) continue;

rollback:
    /* restore buf/buflen so the next call will try to read the same token */
    *buf = buf_save;
    *buflen = buflen_save;
    break;
  }

  return status;
}

MPACK_API int mpack_unparse(mpack_parser_t *parser, char **buf, size_t *buflen,
    mpack_walk_cb enter_cb, mpack_walk_cb exit_cb)
{
  int status = MPACK_EOF;
  MPACK_EXCEPTION_CHECK(parser);

  while (*buflen && status) {
    int write_status;
    mpack_token_t tok;
    mpack_tokbuf_t *tb = &parser->tokbuf;

    if (!tb->plen)
      parser->status = mpack_unparse_tok(parser, &tok, enter_cb, exit_cb);

    MPACK_EXCEPTION_CHECK(parser);

    status = parser->status;

    if (status == MPACK_NOMEM)
      break;

    if (parser->exiting) {
      write_status = mpack_write(tb, buf, buflen, &tok);
      status = write_status ? write_status : status;
    }
  }

  return status;
}

MPACK_API void mpack_parser_copy(mpack_parser_t *dst, mpack_parser_t *src)
{
  mpack_uint32_t i;
  mpack_uint32_t dst_capacity = dst->capacity;
  assert(src->capacity <= dst_capacity);
  /* copy all fields except the stack */
  memcpy(dst, src, sizeof(mpack_one_parser_t) - sizeof(mpack_node_t));
  /* reset capacity */
  dst->capacity = dst_capacity;
  /* copy the stack */
  for (i = 0; i <= src->capacity; i++) {
    dst->items[i] = src->items[i];
  }
}

static int mpack_parser_full(mpack_parser_t *parser)
{
  return parser->size == parser->capacity;
}

static mpack_node_t *mpack_parser_push(mpack_parser_t *parser)
{
  mpack_node_t *top;
  assert(parser->size < parser->capacity);
  top = parser->items + parser->size + 1;
  top->data[0].p = NULL;
  top->data[1].p = NULL;
  top->pos = 0;
  top->key_visited = 0;
  /* increase size and invoke callback, passing parent node if any */
  parser->size++;
  return top;
}

static mpack_node_t *mpack_parser_pop(mpack_parser_t *parser)
{
  mpack_node_t *top, *parent;
  assert(parser->size);
  top = parser->items + parser->size;

  if (top->tok.type > MPACK_TOKEN_CHUNK && top->pos < top->tok.length) {
    /* continue processing children */
    return NULL;
  }

  parent = MPACK_PARENT_NODE(top);
  if (parent) {
    /* we use parent->tok.length to keep track of how many children remain.
     * update it to reflect the processed node. */
    if (top->tok.type == MPACK_TOKEN_CHUNK) {
      parent->pos += top->tok.length;
    } else if (parent->tok.type == MPACK_TOKEN_MAP) {
      /* maps allow up to 2^32 - 1 pairs, so to allow this many items in a
       * 32-bit length variable we use an additional flag to determine if the
       * key of a certain position was visited */
      if (parent->key_visited) {
        parent->pos++;
      }
      parent->key_visited = !parent->key_visited;
    } else {
      parent->pos++;
    }
  }

  parser->size--;
  return top;
}

#include <string.h>


enum {
  MPACK_RPC_RECEIVE_ARRAY = 1,
  MPACK_RPC_RECEIVE_TYPE,
  MPACK_RPC_RECEIVE_ID
};

static mpack_rpc_header_t mpack_rpc_request_hdr(void);
static mpack_rpc_header_t mpack_rpc_reply_hdr(void);
static mpack_rpc_header_t mpack_rpc_notify_hdr(void);
static int mpack_rpc_put(mpack_rpc_session_t *s, mpack_rpc_message_t m);
static int mpack_rpc_pop(mpack_rpc_session_t *s, mpack_rpc_message_t *m);
static void mpack_rpc_reset_hdr(mpack_rpc_header_t *hdr);

MPACK_API void mpack_rpc_session_init(mpack_rpc_session_t *session,
    mpack_uint32_t capacity)
{
  session->capacity = capacity ? capacity : MPACK_RPC_MAX_REQUESTS;
  session->request_id = 0;
  mpack_tokbuf_init(&session->reader);
  mpack_tokbuf_init(&session->writer);
  mpack_rpc_reset_hdr(&session->receive);
  mpack_rpc_reset_hdr(&session->send);
  memset(session->slots, 0,
      sizeof(struct mpack_rpc_slot_s) * session->capacity);
}

MPACK_API int mpack_rpc_receive_tok(mpack_rpc_session_t *session,
    mpack_token_t tok, mpack_rpc_message_t *msg)
{
  int type;

  if (session->receive.index == 0) {
    if (tok.type != MPACK_TOKEN_ARRAY)
      /* not an array */
      return MPACK_RPC_EARRAY;

    if (tok.length < 3 || tok.length > 4)
      /* invalid array length */
      return MPACK_RPC_EARRAYL;

    session->receive.toks[0] = tok;
    session->receive.index++;
    return MPACK_EOF;  /* get the type */
  }

  if (session->receive.index == 1) {

    if (tok.type != MPACK_TOKEN_UINT || tok.length > 1 || tok.data.value.lo > 2)
      /* invalid type */
      return MPACK_RPC_ETYPE;

    if (tok.data.value.lo < 2 && session->receive.toks[0].length != 4)
      /* request or response with array length != 4 */
      return MPACK_RPC_EARRAYL;

    if (tok.data.value.lo == 2 && session->receive.toks[0].length != 3)
      /* notification with array length != 3 */
      return MPACK_RPC_EARRAYL;

    session->receive.toks[1] = tok;
    session->receive.index++;

    if (tok.data.value.lo < 2) return MPACK_EOF;

    type = MPACK_RPC_NOTIFICATION;
    goto end;
  }

  assert(session->receive.index == 2);

  if (tok.type != MPACK_TOKEN_UINT || tok.length > 4)
    /* invalid request/response id */
    return MPACK_RPC_EMSGID;

  msg->id = tok.data.value.lo;
  msg->data.p = NULL;
  type = (int)session->receive.toks[1].data.value.lo + MPACK_RPC_REQUEST;

  if (type == MPACK_RPC_RESPONSE && !mpack_rpc_pop(session, msg))
    /* response with invalid id */
    return MPACK_RPC_ERESPID;

end:
  mpack_rpc_reset_hdr(&session->receive);
  return type;
}

MPACK_API int mpack_rpc_request_tok(mpack_rpc_session_t *session,
    mpack_token_t *tok, mpack_data_t data)
{
  if (session->send.index == 0) {
    int status;
    mpack_rpc_message_t msg;
    do {
      msg.id = session->request_id;
      msg.data = data;
      session->send = mpack_rpc_request_hdr();
      session->send.toks[2].type = MPACK_TOKEN_UINT;
      session->send.toks[2].data.value.lo = msg.id;
      session->send.toks[2].data.value.hi = 0;
      *tok = session->send.toks[0];
      status = mpack_rpc_put(session, msg);
      if (status == -1) return MPACK_NOMEM;
      session->request_id = (session->request_id + 1) % 0xffffffff;
    } while (!status);
    session->send.index++;
    return MPACK_EOF;
  }

  if (session->send.index == 1) {
    *tok = session->send.toks[1];
    session->send.index++;
    return MPACK_EOF;
  }

  assert(session->send.index == 2);
  *tok = session->send.toks[2];
  mpack_rpc_reset_hdr(&session->send);
  return MPACK_OK;
}

MPACK_API int mpack_rpc_reply_tok(mpack_rpc_session_t *session,
    mpack_token_t *tok, mpack_uint32_t id)
{
  if (session->send.index == 0) {
    session->send = mpack_rpc_reply_hdr();
    session->send.toks[2].type = MPACK_TOKEN_UINT;
    session->send.toks[2].data.value.lo = id;
    session->send.toks[2].data.value.hi = 0;
    *tok = session->send.toks[0];
    session->send.index++;
    return MPACK_EOF;
  }

  if (session->send.index == 1) {
    *tok = session->send.toks[1];
    session->send.index++;
    return MPACK_EOF;
  }

  assert(session->send.index == 2);
  *tok = session->send.toks[2];
  mpack_rpc_reset_hdr(&session->send);
  return MPACK_OK;
}

MPACK_API int mpack_rpc_notify_tok(mpack_rpc_session_t *session,
    mpack_token_t *tok)
{
  if (session->send.index == 0) {
    session->send = mpack_rpc_notify_hdr();
    *tok = session->send.toks[0];
    session->send.index++;
    return MPACK_EOF;
  }

  assert(session->send.index == 1);
  *tok = session->send.toks[1];
  mpack_rpc_reset_hdr(&session->send);
  return MPACK_OK;
}

MPACK_API int mpack_rpc_receive(mpack_rpc_session_t *session, const char **buf,
    size_t *buflen, mpack_rpc_message_t *msg)
{
  int status;

  do {
    mpack_token_t tok;
    status = mpack_read(&session->reader, buf, buflen, &tok);
    if (status) break;
    status = mpack_rpc_receive_tok(session, tok, msg);
    if (status >= MPACK_RPC_REQUEST) break;
  } while (*buflen);

  return status;
}

MPACK_API int mpack_rpc_request(mpack_rpc_session_t *session, char **buf,
    size_t *buflen, mpack_data_t data)
{
  int status = MPACK_EOF;

  while (status && *buflen) {
    int write_status;
    mpack_token_t tok;
    if (!session->writer.plen) {
      status = mpack_rpc_request_tok(session, &tok, data);
    }
    if (status == MPACK_NOMEM) break;
    write_status = mpack_write(&session->writer, buf, buflen, &tok);
    status = write_status ? write_status : status;
  }

  return status;
}

MPACK_API int mpack_rpc_reply(mpack_rpc_session_t *session, char **buf,
    size_t *buflen, mpack_uint32_t id)
{
  int status = MPACK_EOF;

  while (status && *buflen) {
    int write_status;
    mpack_token_t tok;
    if (!session->writer.plen) {
      status = mpack_rpc_reply_tok(session, &tok, id);
    }
    write_status = mpack_write(&session->writer, buf, buflen, &tok);
    status = write_status ? write_status : status;
  }

  return status;
}

MPACK_API int mpack_rpc_notify(mpack_rpc_session_t *session, char **buf,
    size_t *buflen)
{
  int status = MPACK_EOF;

  while (status && *buflen) {
    int write_status;
    mpack_token_t tok;
    if (!session->writer.plen) {
      status = mpack_rpc_notify_tok(session, &tok);
    }
    write_status = mpack_write(&session->writer, buf, buflen, &tok);
    status = write_status ? write_status : status;
  }

  return status;
}

MPACK_API void mpack_rpc_session_copy(mpack_rpc_session_t *dst,
    mpack_rpc_session_t *src)
{
  mpack_uint32_t i;
  mpack_uint32_t dst_capacity = dst->capacity;
  assert(src->capacity <= dst_capacity);
  /* copy all fields except slots */
  memcpy(dst, src, sizeof(mpack_rpc_one_session_t) -
      sizeof(struct mpack_rpc_slot_s));
  /* reset capacity */
  dst->capacity = dst_capacity;
  /* reinsert requests  */
  memset(dst->slots, 0, sizeof(struct mpack_rpc_slot_s) * dst->capacity);
  for (i = 0; i < src->capacity; i++) {
    if (src->slots[i].used) mpack_rpc_put(dst, src->slots[i].msg);
  }
}

static mpack_rpc_header_t mpack_rpc_request_hdr(void)
{
  mpack_rpc_header_t hdr;
  hdr.index = 0;
  hdr.toks[0].type = MPACK_TOKEN_ARRAY;
  hdr.toks[0].length = 4;
  hdr.toks[1].type = MPACK_TOKEN_UINT;
  hdr.toks[1].data.value.lo = 0;
  hdr.toks[1].data.value.hi = 0;
  return hdr;
}

static mpack_rpc_header_t mpack_rpc_reply_hdr(void)
{
  mpack_rpc_header_t hdr = mpack_rpc_request_hdr();
  hdr.toks[1].data.value.lo = 1;
  hdr.toks[1].data.value.hi = 0;
  return hdr;
}

static mpack_rpc_header_t mpack_rpc_notify_hdr(void)
{
  mpack_rpc_header_t hdr = mpack_rpc_request_hdr();
  hdr.toks[0].length = 3;
  hdr.toks[1].data.value.lo = 2;
  hdr.toks[1].data.value.hi = 0;
  return hdr;
}

static int mpack_rpc_put(mpack_rpc_session_t *session, mpack_rpc_message_t msg)
{
  struct mpack_rpc_slot_s *slot = NULL;
  mpack_uint32_t i;
  mpack_uint32_t hash = msg.id % session->capacity;

  for (i = 0; i < session->capacity; i++) {
    if (!session->slots[hash].used || session->slots[hash].msg.id == msg.id) {
      slot = session->slots + hash;
      break;
    }
    hash = hash > 0 ? hash - 1 : session->capacity - 1;
  }

  if (!slot) return -1; /* no space */
  if (slot->msg.id == msg.id && slot->used) return 0;  /* duplicate key */
  slot->msg = msg;
  slot->used = 1;
  return 1;
}

static int mpack_rpc_pop(mpack_rpc_session_t *session, mpack_rpc_message_t *msg)
{
  struct mpack_rpc_slot_s *slot = NULL;
  mpack_uint32_t i;
  mpack_uint32_t hash = msg->id % session->capacity;

  for (i = 0; i < session->capacity; i++) {
    if (session->slots[hash].used && session->slots[hash].msg.id == msg->id) {
      slot = session->slots + hash;
      break;
    }
    hash = hash > 0 ? hash - 1 : session->capacity - 1;
  }

  if (!slot) return 0;

  *msg = slot->msg;
  slot->used = 0;

  return 1;
}

static void mpack_rpc_reset_hdr(mpack_rpc_header_t *hdr)
{
  hdr->index = 0;
}
