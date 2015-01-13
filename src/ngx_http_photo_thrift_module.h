/* 
 * File:   ngx_http_photo_thrift_module.h
 * Author: hungnguyen
 *
 * Created on January 9, 2015, 9:51 AM
 */

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <inttypes.h>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#define DIR_SEPARATOR '\\'
#define strdup _strdup
#define open _open
#define close _close
#define write _write
#define lseek _lseeki64
#define stat _stat64
#else
#define DIR_SEPARATOR '/'
#include <unistd.h>
#include <sys/mman.h>
#endif
#define MAX_TRACKS 8

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

typedef struct {
    ngx_str_t content_thrift_server;
    ngx_str_t content_thrift_port;
    ngx_str_t meta_thrift_server;
    ngx_str_t meta_thrift_port;
} photo_thrift_conf_t;

typedef struct { // Newly added
    //    char *etag;
    char *file_name;
    unsigned long size;
} request_info_;

struct bucket_t {
    ngx_http_request_t *r;
    ngx_chain_t **chain;
    uint64_t content_length;
    ngx_chain_t *first;
};
typedef struct bucket_t bucket_t;

extern bucket_t *bucket_init(ngx_http_request_t *r) {
    bucket_t *bucket = (bucket_t *) ngx_pcalloc(r->pool, sizeof (bucket_t));
    bucket->r = r;
    bucket->first = 0;
    bucket->chain = &bucket->first;
    bucket->content_length = 0;

    return bucket;
}


static char *ngx_photo_thrift(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_photo_thrift_create_conf(ngx_conf_t *cf);
static char *ngx_photo_thrift_merge_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_command_t ngx_photo_thrift_commands[] = {
    { ngx_string("thrift"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_photo_thrift,
        0,
        0,
        NULL},

    { ngx_string("content_server_address"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(photo_thrift_conf_t, content_thrift_server),
        NULL},
    { ngx_string("content_server_port"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(photo_thrift_conf_t, content_thrift_port),
        NULL},
    { ngx_string("meta_server_address"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(photo_thrift_conf_t, meta_thrift_server),
        NULL},
    { ngx_string("meta_server_port"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(photo_thrift_conf_t, meta_thrift_port),
        NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_photo_thrift_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_photo_thrift_create_conf, /* create location configuration */
    ngx_photo_thrift_merge_conf /* merge location configuration */
};

ngx_module_t ngx_photo_thrift_module = {
    NGX_MODULE_V1,
    &ngx_photo_thrift_module_ctx, /* module context */
    ngx_photo_thrift_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

