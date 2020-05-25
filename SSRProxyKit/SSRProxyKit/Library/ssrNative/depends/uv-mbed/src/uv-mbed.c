//
// Created by eugene on 3/14/19.
// Modified by ssrlive
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/certs.h>
#include <assert.h>
#include <stdbool.h>
#include "uv-mbed/uv-mbed.h"
#include "bio.h"

struct uv_mbed_s {
    uv_tcp_t socket;
    void *user_data;
    mbedtls_ssl_config ssl_config;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_pk_context pkey;

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    uv_mbed_connect_cb connect_cb;
    void *connect_cb_p;

    uv_mbed_alloc_cb alloc_cb;
    uv_mbed_read_cb read_cb;
    void *read_cb_p;

    uv_mbed_close_cb close_cb;
    void *close_cb_p;

    struct bio *ssl_in;
    struct bio *ssl_out;
};

#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif /* container_of */

static void tls_debug_f(void *ctx, int level, const char *file, int line, const char *str);
static void init_ssl(uv_mbed_t *mbed, int dump_level);
static void _uv_dns_resolve_done_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);

static void _uv_tcp_connect_established_cb(uv_connect_t* req, int status);
static void _uv_tcp_shutdown_cb(uv_shutdown_t* req, int status) ;

static void mbed_ssl_process_in(uv_mbed_t *mbed);

struct tcp_write_ctx {
    uint8_t *buf;
    uv_mbed_write_cb cb;
    void *cb_p;
};

static void mbed_ssl_process_out(uv_mbed_t *mbed, uv_mbed_write_cb cb, void *p);
static int _mbed_ssl_recv(void* ctx, uint8_t *buf, size_t len);
static int _mbed_ssl_send(void* ctx, const uint8_t *buf, size_t len);

static void mbed_continue_handshake(uv_mbed_t *mbed);

uv_mbed_t * uv_mbed_init(uv_loop_t *loop, void *user_data, int dump_level) {
    uv_mbed_t *mbed = (uv_mbed_t *) calloc(1, sizeof(*mbed));
    mbed->user_data = user_data;
    uv_tcp_init(loop, &mbed->socket);
    init_ssl(mbed, dump_level);
    return mbed;
}

void * uv_mbed_user_data(uv_mbed_t *mbed) {
    return mbed->user_data;
}

int uv_mbed_set_ca(uv_mbed_t *mbed, const char *root_cert_file) {
    if (mbedtls_x509_crt_parse_file(&mbed->cacert, root_cert_file) == 0) {
        mbedtls_ssl_conf_ca_chain(&mbed->ssl_config, &mbed->cacert, NULL);
    }
    return 0;
}

int uv_mbed_set_cert(uv_mbed_t *mbed, mbedtls_x509_crt *cert, mbedtls_pk_context *privkey) {
    mbedtls_ssl_conf_own_cert(&mbed->ssl_config, cert, privkey);
    return 0;
}

static void _write_and_close_cb(uv_mbed_t *mbed, int status, void *p) {
    uv_shutdown_t *sr = (uv_shutdown_t *)calloc(1, sizeof(uv_shutdown_t));
    sr->data = mbed;
    uv_shutdown(sr, (uv_stream_t *) &mbed->socket, _uv_tcp_shutdown_cb);
}

int uv_mbed_close(uv_mbed_t *mbed, uv_mbed_close_cb close_cb, void *p) {
    int rc;
    mbed->close_cb = close_cb;
    mbed->close_cb_p = p;
    rc = mbedtls_ssl_close_notify(&mbed->ssl);

    mbed_ssl_process_out(mbed, &_write_and_close_cb, p);
    return 0;
}

int uv_mbed_keepalive(uv_mbed_t *mbed, int keepalive, unsigned int delay) {
    return uv_tcp_keepalive(&mbed->socket, keepalive, delay);
}

int uv_mbed_nodelay(uv_mbed_t *mbed, int nodelay) {
    return uv_tcp_nodelay(&mbed->socket, nodelay);
}

int uv_mbed_connect(uv_mbed_t *mbed, const char *host, int port, uv_mbed_connect_cb cb, void *p) {
    char portstr[6] = { 0 };
    uv_loop_t *loop = mbed->socket.loop;
    uv_getaddrinfo_t *req = (uv_getaddrinfo_t *)calloc(1, sizeof(*req));

    mbed->connect_cb = cb;
    mbed->connect_cb_p = p;

    req->data = mbed;
    sprintf(portstr, "%d", port);
    return uv_getaddrinfo(loop, req, _uv_dns_resolve_done_cb, host, portstr, NULL);
}

int uv_mbed_set_blocking(uv_mbed_t *um, int blocking) {
    return uv_stream_set_blocking((uv_stream_t *) &um->socket, blocking);
}

int uv_mbed_read(uv_mbed_t *mbed, uv_mbed_alloc_cb alloc_cb, uv_mbed_read_cb read_cb, void *p) {
    mbed->alloc_cb = alloc_cb;
    mbed->read_cb = read_cb;
    mbed->read_cb_p = p;
    return 0;
}

int uv_mbed_write(uv_mbed_t *mbed, const uv_buf_t *buf, uv_mbed_write_cb cb, void *p) {
    int rc = 0;
    int sent = 0;

    while (sent < (int) buf->len) {
        rc = mbedtls_ssl_write(&mbed->ssl, (unsigned char *)(buf->base + sent), buf->len - sent);

        if (rc >= 0) {
            sent += rc;
        }

        if (rc < 0) {
            break;
        }
    }

    if (sent > 0) {
        mbed_ssl_process_out(mbed, cb, p);
        rc = 0;
    }
    else {
        cb(mbed, rc, p);
    }
    return rc;
}

static void init_ssl(uv_mbed_t *mbed, int dump_level) {
    unsigned char *seed;

    mbedtls_debug_set_threshold(dump_level);

    mbedtls_ssl_config_init(&mbed->ssl_config);
    mbedtls_ssl_conf_dbg(&mbed->ssl_config, tls_debug_f, stdout);
    mbedtls_ssl_config_defaults(&mbed->ssl_config,
                                 MBEDTLS_SSL_IS_CLIENT,
                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                 MBEDTLS_SSL_PRESET_DEFAULT );
    mbedtls_ssl_conf_authmode(&mbed->ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ctr_drbg_init(&mbed->ctr_drbg);
    mbedtls_entropy_init(&mbed->entropy);
    seed = (unsigned char *) malloc(MBEDTLS_ENTROPY_MAX_SEED_SIZE); // uninitialized memory
    mbedtls_ctr_drbg_seed(&mbed->ctr_drbg, mbedtls_entropy_func, &mbed->entropy, seed, MBEDTLS_ENTROPY_MAX_SEED_SIZE);
    mbedtls_ssl_conf_rng(&mbed->ssl_config, mbedtls_ctr_drbg_random, &mbed->ctr_drbg);

    mbedtls_x509_crt_init(&mbed->cacert);
    mbedtls_ssl_conf_ca_chain( &mbed->ssl_config, &mbed->cacert, NULL );

    mbedtls_x509_crt_init(&mbed->clicert);
    mbedtls_x509_crt_parse( &mbed->clicert,
        (const unsigned char *) mbedtls_test_cli_crt,
        mbedtls_test_cli_crt_len );

    mbedtls_pk_init(&mbed->pkey);
    mbedtls_pk_parse_key(&mbed->pkey,
        (const unsigned char *)mbedtls_test_cli_key,
        mbedtls_test_cli_key_len, NULL, 0 );

    mbedtls_ssl_conf_own_cert(&mbed->ssl_config, &mbed->clicert, &mbed->pkey);

    mbedtls_ssl_conf_authmode(&mbed->ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);

    mbedtls_ssl_init(&mbed->ssl);
    mbedtls_ssl_setup(&mbed->ssl, &mbed->ssl_config);

    mbed->ssl_in = bio_new(true);
    mbed->ssl_out = bio_new(false);
    mbedtls_ssl_set_bio(&mbed->ssl, mbed, _mbed_ssl_send, _mbed_ssl_recv, NULL);

    free(seed);
}

int uv_mbed_free(uv_mbed_t *mbed) {
    mbedtls_ctr_drbg_context *rng;
    mbedtls_entropy_context *ctx;
    bio_free(mbed->ssl_in);
    bio_free(mbed->ssl_out);
    mbedtls_ssl_free(&mbed->ssl);

    rng = (mbedtls_ctr_drbg_context *) mbed->ssl_config.p_rng;
    ctx = (mbedtls_entropy_context *)rng->p_entropy;
    mbedtls_entropy_free(ctx);
    assert(ctx == &mbed->entropy);
    mbedtls_ctr_drbg_free(rng);
    assert(rng == &mbed->ctr_drbg);

    mbedtls_ssl_config_free(&mbed->ssl_config);

    mbedtls_x509_crt_free(&mbed->cacert);
    mbedtls_x509_crt_free(&mbed->clicert);
    mbedtls_pk_free(&mbed->pkey);

    free(mbed);

    return 0;
}

static void tls_debug_f(void *ctx, int level, const char *file, int line, const char *str)
{
    ((void) level);
    printf("%s:%04d: %s", file, line, str );
    fflush(  stdout );
}

static void _uv_tcp_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    char *base = (char*) calloc(suggested_size, sizeof(*base));
    *buf = uv_buf_init(base, suggested_size);
}

static void _do_uv_mbeb_connect_cb(uv_mbed_t *mbed, int status) {
    if (mbed && mbed->connect_cb) {
        mbed->connect_cb(mbed, status, mbed->connect_cb_p);
        mbed->connect_cb = NULL;
        mbed->connect_cb_p = NULL;
    }
}

static void _uv_dns_resolve_done_cb(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    uv_mbed_t *mbed = (uv_mbed_t *) req->data;

    if (status < 0) {
        _do_uv_mbeb_connect_cb(mbed, status);
    }
    else {
        uv_connect_t *tcp_cr = (uv_connect_t *) calloc(1, sizeof(uv_connect_t));
        tcp_cr->data = mbed;
        uv_tcp_connect(tcp_cr, &mbed->socket, res->ai_addr, _uv_tcp_connect_established_cb);
    }
    uv_freeaddrinfo(res);
    free(req);
}

static void _uv_tcp_read_done_cb (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uv_mbed_t *mbed = (uv_mbed_t *) stream->data;
    bool release_buf = true;
    assert(stream == (uv_stream_t *) &mbed->socket);
    if (nread > 0) {
        struct bio *in = mbed->ssl_in;
        bool rc = bio_put(in, (uint8_t *)buf->base, (size_t) nread);
        mbed_ssl_process_in(mbed);
        if (in->zerocopy && rc) {
            release_buf = false;
        }
    }

    if (release_buf) {
        free((void *)buf->base);
    }

    if (nread < 0) {
        assert(mbed->connect_cb == NULL);
        if (mbed->read_cb) {
            uv_buf_t b = uv_buf_init(NULL, 0);
            mbed->read_cb(mbed, nread, &b, mbed->read_cb_p);
        }
    }
}

static void _uv_tcp_connect_established_cb(uv_connect_t *req, int status) {
    uv_mbed_t *mbed = (uv_mbed_t *) req->data;
    // uv_mbed_t *mbed = container_of((uv_tcp_t*)req->handle, uv_mbed_t, socket);
    if (status < 0) {
        _do_uv_mbeb_connect_cb(mbed, status);
    }
    else {
        //uv_stream_t* socket = req->handle;
        uv_stream_t *socket = (uv_stream_t*)&mbed->socket;
        socket->data = mbed;
        uv_read_start(socket, _uv_tcp_alloc_cb, _uv_tcp_read_done_cb);
        mbed_ssl_process_in(mbed);
    }
    free(req);
}

static void _uv_tcp_close_done_cb (uv_handle_t *h) {
    uv_mbed_t *mbed = container_of((uv_tcp_t*)h, uv_mbed_t, socket);
    assert(mbed);
    if (mbed->close_cb) {
        mbed->close_cb(mbed, mbed->close_cb_p);
    }
}

static void _uv_tcp_shutdown_cb(uv_shutdown_t* req, int status) {
    uv_mbed_t *mbed = (uv_mbed_t *) req->data;
    assert(req->handle == (uv_stream_t *)&mbed->socket);
    uv_close((uv_handle_t *) &mbed->socket, _uv_tcp_close_done_cb);
    free(req);
}

static void _uv_mbed_tcp_write_done_cb(uv_write_t *req, int status) {
    struct tcp_write_ctx *ctx = (struct tcp_write_ctx *) req->data;
    uv_mbed_t *mbed = container_of((uv_tcp_t*)req->handle, uv_mbed_t, socket);

    assert(mbed);
    assert(ctx);
    assert(ctx->cb);

    ctx->cb(mbed, status, ctx->cb_p);

    free(ctx->buf);
    free(ctx);
    free(req);
}

static int _mbed_ssl_recv(void* ctx, uint8_t *buf, size_t len) {
    uv_mbed_t *mbed = (uv_mbed_t *) ctx;
    struct bio *in = mbed->ssl_in;
    if (bio_available(in) == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }
    return (int) bio_read(in, buf, len);
}

static int _mbed_ssl_send(void* ctx, const uint8_t *buf, size_t len) {
    uv_mbed_t *mbed = (uv_mbed_t *) ctx;
    struct bio *out = mbed->ssl_out;
    bio_put(out, buf, len);
    return (int) len;
}

static void _mbed_handshake_write_cb(uv_mbed_t *mbed, int status, void *p) {
    if (status != MBEDTLS_ERR_SSL_WANT_WRITE) {
        mbed_continue_handshake(mbed);
    }
}

static void mbed_ssl_process_in(uv_mbed_t *mbed) {
    do {
        struct bio *in = mbed->ssl_in;
        ssize_t recv = 0;
        uv_buf_t buf = uv_buf_init(NULL, 0);

        if (mbed->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
            mbed_continue_handshake(mbed);
            break;
        }
        if (mbed->read_cb == NULL) {
            break;
        }
        if (bio_available(in) == 0) {
            break;
        }
        mbed->alloc_cb(mbed, 64 * 1024, &buf, mbed->read_cb_p);
        if (buf.base == NULL || buf.len == 0) {
            recv = UV_ENOBUFS;
        } else {
            while (bio_available(in) > 0 && (buf.len - (size_t)recv) > 0) {
                int read = mbedtls_ssl_read(&mbed->ssl, (uint8_t *) buf.base + (size_t)recv, buf.len - (size_t)recv);
                if (read < 0) {
                    break;
                }
                recv += (ssize_t)read;
            }
        }
        mbed->read_cb(mbed, recv, &buf, mbed->read_cb_p);
    } while(0);
}

static void mbed_continue_handshake(uv_mbed_t *mbed) {
    int rc = mbedtls_ssl_handshake(&mbed->ssl);
    switch (rc) {
    case 0:
        _do_uv_mbeb_connect_cb(mbed, rc);
        break;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
    case MBEDTLS_ERR_SSL_WANT_READ:
        mbed_ssl_process_out(mbed, &_mbed_handshake_write_cb, NULL);
        break;
    default:
        (void)mbed;
    }
}

static void mbed_ssl_process_out(uv_mbed_t *mbed, uv_mbed_write_cb cb, void *p) {
    struct bio *out = mbed->ssl_out;
    size_t avail = bio_available(out);

    if (avail <= 0) {
        // how did we get here?
        cb(mbed, MBEDTLS_ERR_SSL_WANT_WRITE, p);
    }
    else {
        size_t len;
        uv_write_t *tcp_wr;
        uv_buf_t wb;
        struct tcp_write_ctx *ctx;

        ctx = (struct tcp_write_ctx *) calloc(1, sizeof(*ctx));
        ctx->buf = (uint8_t *)calloc(avail, sizeof(uint8_t));
        ctx->cb = cb;
        ctx->cb_p = p;

        len = bio_read(out, ctx->buf, avail);

        tcp_wr = (uv_write_t *) calloc(1, sizeof(uv_write_t));
        tcp_wr->data = ctx;
        wb = uv_buf_init((char *) ctx->buf, (unsigned int) len);
        uv_write(tcp_wr, (uv_stream_t *) &mbed->socket, &wb, 1, _uv_mbed_tcp_write_done_cb);
        assert((avail = bio_available(out)) == 0);
    }
}

