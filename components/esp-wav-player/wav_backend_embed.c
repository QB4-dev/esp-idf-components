#include "wav_handle.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
   Embedded WAVs are stored in flash using EMBED_FILES:
   extern const uint8_t _binary_audio1_wav_start[] asm("_binary_audio1_wav_start");
*/

typedef struct {
    const uint8_t *data; // pointer to WAV in flash
    size_t         pos;  // current read offset
} wav_embed_ctx_t;

static int embed_open(wav_handle_t *h)
{
    wav_embed_ctx_t *c = h->ctx;

    if (!c || !c->data)
        return -1;

    c->pos = 0; // reset internal pointer

    return 0;
}

static size_t embed_read(wav_handle_t *h, void *buf, size_t len)
{
    wav_embed_ctx_t *c = h->ctx;

    if (!c || !c->data)
        return 0;

    memcpy(buf, c->data + c->pos, len);
    c->pos += len;

    return len;
}

static int embed_seek(wav_handle_t *h, size_t offset)
{
    wav_embed_ctx_t *c = h->ctx;

    if (!c || !c->data)
        return -1;

    c->pos = offset;
    return 0;
}

static void embed_close(wav_handle_t *h)
{
    // nothing to do for embedded data
}

static void embed_cleanup(wav_handle_t *h)
{
    if (!h)
        return;

    free(h->ctx);
    h->ctx = NULL;
}

wav_handle_t *wav_backend_embed_create(const uint8_t *data)
{
    if (!data)
        return NULL;

    wav_handle_t    *h = calloc(1, sizeof(*h));
    wav_embed_ctx_t *ctx = calloc(1, sizeof(*ctx));

    if (!h || !ctx) {
        free(h);
        free(ctx);
        return NULL;
    }

    ctx->data = data;
    ctx->pos = 0;

    h->ctx = ctx;
    h->open = embed_open;
    h->read = embed_read;
    h->seek = embed_seek;
    h->close = embed_close;
    h->clean_ctx = embed_cleanup;
    return h;
}
