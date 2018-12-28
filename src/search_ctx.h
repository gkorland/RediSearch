#ifndef __SEARCH_CTX_H
#define __SEARCH_CTX_H

#include <sched.h>

#include "redismodule.h"
#include "spec.h"
#include "concurrent_ctx.h"
#include "trie/trie_type.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RedisSearchCtx {
  RedisModuleCtx *redisCtx;
  RedisModuleKey *key_;
  IndexSpec *spec;
  uint32_t refcount;
  int isStatic;
} RedisSearchCtx;

#define SEARCH_CTX_STATIC(ctx, sp) \
  { ctx, NULL, sp, 0, 1 }

#define SEARCH_CTX_SORTABLES(ctx) ((ctx && ctx->spec) ? ctx->spec->sortables : NULL)
// Create a string context on the heap
RedisSearchCtx *NewSearchCtx(RedisModuleCtx *ctx, RedisModuleString *indexName);
RedisSearchCtx *NewSearchCtxDefault(RedisModuleCtx *ctx);

RedisSearchCtx *SearchCtx_Refresh(RedisSearchCtx *sctx, RedisModuleString *keyName);

// Same as above, only from c string (null terminated)
RedisSearchCtx *NewSearchCtxC(RedisModuleCtx *ctx, const char *indexName);

#define SearchCtx_Incref(sctx) \
  ({                           \
    (sctx)->refcount++;        \
    sctx;                      \
  })

#define SearchCtx_Decref(sctx) \
  if (!--((sctx)->refcount)) { \
    SearchCtx_Free(sctx);      \
  }

void SearchCtx_Free(RedisSearchCtx *sctx);
#ifdef __cplusplus
}
#endif
#endif
