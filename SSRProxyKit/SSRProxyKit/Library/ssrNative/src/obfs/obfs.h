/*
 * obfs.h - Define shadowsocksR server's buffers and callbacks
 *
 * Copyright (C) 2015 - 2016, Break Wa11 <mmgac001@gmail.com>
 */

#ifndef _OBFS_OBFS_H
#define _OBFS_OBFS_H

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))
#endif

#define OBFS_HMAC_SHA1_LEN 10

#ifndef SSR_BUFF_SIZE
#define SSR_BUFF_SIZE 2048
#endif // !SSR_BUFF_SIZE

struct buffer_t;
struct cipher_env_t;

struct server_info_t {
    char host[256];
    uint16_t port;
    char *param;
    void *g_data;
    const uint8_t *iv;
    size_t iv_len;
    uint8_t recv_iv[256];
    size_t recv_iv_len;
    uint8_t *key;
    uint16_t key_len;
    size_t head_len;
    uint16_t tcp_mss;
    uint16_t overhead;
    uint32_t buffer_size;
    struct cipher_env_t *cipher_env;
};

struct obfs_t {
    struct server_info_t server;
    void *l_data;

    void * (*init_data)(void);
    size_t (*get_overhead)(struct obfs_t *obfs);
    bool (*need_feedback)(struct obfs_t *obfs);
    struct server_info_t * (*get_server_info)(struct obfs_t *obfs);
    void (*set_server_info)(struct obfs_t *obfs, struct server_info_t *server);
    void (*dispose)(struct obfs_t *obfs);

    size_t (*client_pre_encrypt)(struct obfs_t *obfs, char **pplaindata, size_t datalength, size_t* capacity);
    ssize_t (*client_post_decrypt)(struct obfs_t *obfs, char **pplaindata, int datalength, size_t* capacity);

    struct buffer_t * (*client_encode)(struct obfs_t *obfs, const struct buffer_t *buf);
    struct buffer_t * (*client_decode)(struct obfs_t *obfs, const struct buffer_t *buf, bool *needsendback);

    ssize_t (*client_udp_pre_encrypt)(struct obfs_t *obfs, char **pplaindata, size_t datalength, size_t* capacity);
    ssize_t (*client_udp_post_decrypt)(struct obfs_t *obfs, char **pplaindata, size_t datalength, size_t* capacity);

    struct buffer_t * (*server_pre_encrypt)(struct obfs_t *obfs, const struct buffer_t *buf);
    struct buffer_t * (*server_post_decrypt)(struct obfs_t *obfs, struct buffer_t *buf, bool *need_feedback);

    struct buffer_t * (*server_encode)(struct obfs_t *obfs, const struct buffer_t *buf);
    struct buffer_t * (*server_decode)(struct obfs_t *obfs, const struct buffer_t *buf, bool *need_decrypt, bool *need_feedback);

    bool (*server_udp_pre_encrypt)(struct obfs_t *obfs, struct buffer_t *buf);
    bool (*server_udp_post_decrypt)(struct obfs_t *obfs, struct buffer_t *buf, uint32_t *uid);
};

void * init_data(void);
size_t get_overhead(struct obfs_t *obfs);
bool need_feedback_false(struct obfs_t *obfs);
bool need_feedback_true(struct obfs_t *obfs);

struct obfs_t * new_obfs_instance(const char *plugin_name);
void free_obfs_instance(struct obfs_t *plugin);

void set_server_info(struct obfs_t *obfs, struct server_info_t *server);
struct server_info_t * get_server_info(struct obfs_t *obfs);
void dispose_obfs(struct obfs_t *obfs);

struct buffer_t * generic_server_pre_encrypt(struct obfs_t *obfs, const struct buffer_t *buf);
struct buffer_t * generic_server_encode(struct obfs_t *obfs, const struct buffer_t *buf);
struct buffer_t * generic_server_decode(struct obfs_t *obfs, const struct buffer_t *buf, bool *need_decrypt, bool *need_feedback);
struct buffer_t * generic_server_post_decrypt(struct obfs_t *obfs, struct buffer_t *buf, bool *need_feedback);
bool generic_server_udp_pre_encrypt(struct obfs_t *obfs, struct buffer_t *buf);
bool generic_server_udp_post_decrypt(struct obfs_t *obfs, struct buffer_t *buf, uint32_t *uid);

#if (defined(_MSC_VER) && (_MSC_VER < 1800))
#include <stdio.h>
#if !defined(snprintf)
#define snprintf(dst, size, fmt, ...) _snprintf_s((dst), (size), _TRUNCATE, (fmt), __VA_ARGS__)
#endif // !defined(snprintf)
#endif // (defined(_MSC_VER) && (_MSC_VER < 1800))

#endif // _OBFS_OBFS_H
