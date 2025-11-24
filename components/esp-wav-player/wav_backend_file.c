#include "wav_handle.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *path;
    FILE       *f;
} wav_file_ctx_t;

static int file_open(wav_handle_t *h)
{
    wav_file_ctx_t *c = h->ctx;

    if (!c)
        return -1;

    if (c->f) {
        // already open
        return 0;
    }

    c->f = fopen(c->path, "rb");
    return c->f ? 0 : -1;
}

static size_t file_read(wav_handle_t *h, void *buf, size_t len)
{
    wav_file_ctx_t *c = h->ctx;

    if (!c || !c->f)
        return 0;

    return fread(buf, 1, len, c->f);
}

static int file_seek(wav_handle_t *h, size_t offset)
{
    wav_file_ctx_t *c = h->ctx;

    if (!c || !c->f)
        return -1;

    return fseek(c->f, offset, SEEK_SET);
}

static void file_close(wav_handle_t *h)
{
    wav_file_ctx_t *c = h->ctx;

    if (c && c->f) {
        fclose(c->f);
        c->f = NULL;
    }
}

static void file_cleanup(wav_handle_t *h)
{
    if (!h)
        return;

    free(h->ctx);
    h->ctx = NULL;
}

wav_handle_t *wav_backend_file_create(const char *path)
{
    wav_handle_t   *h = calloc(1, sizeof(*h));
    wav_file_ctx_t *ctx = calloc(1, sizeof(*ctx));

    if (!h || !ctx) {
        free(h);
        free(ctx);
        return NULL;
    }

    ctx->path = path;
    ctx->f = NULL;

    h->ctx = ctx;
    h->open = file_open;
    h->read = file_read;
    h->seek = file_seek;
    h->close = file_close;
    h->clean_ctx = file_cleanup;
    return h;
}
