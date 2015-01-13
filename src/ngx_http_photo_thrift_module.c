/* 
 * File:   ngx_http_photo_thrift_module.c
 * Author: hungnguyen
 *
 * Created on January 9, 2015, 10:05 AM
 */


#include "ngx_http_photo_thrift_module.h"
#include "photodb_client.h"
#include "murmur3.h"


#ifdef DISABLE_INLINE_FUNCTIONS
#define ALWAYS_INLINE
#else
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#ifdef DISABLE_BRANCH_PREDICTION
#define LIKELY_IS(x,y) (x)
#else
#define LIKELY_IS(x,y)	__builtin_expect((x), (y))
#endif

#define LIKELY(x)	LIKELY_IS(!!(x), 1)
#define UNLIKELY(x)	LIKELY_IS((x), 0)

#define PARSE_NAME_SIZE(file_name_size) \
  do { \
        char *end; \
        if (LIKELY(end = strchr(file_name_size, '/'))) { \
            *end = '\0'; \
            request_info->file_name = file_name_size; \
            request_size = end + 1; \
        } else { \
            request_info->file_name = file_name_size; \
            request_size = NULL; \
        } \
  } while (0)




static void *ngx_photo_thrift_create_conf(ngx_conf_t *cf) {
    photo_thrift_conf_t *conf;
    conf = ngx_palloc(cf->pool, sizeof (photo_thrift_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    //    conf->content_thrift_server = NGX_CONF_UN;
    //    conf->content_thrift_port = NGX_CONF_UNSET_UINT;
    //    conf->meta_thrift_server = NGX_CONF_UNSET;
    //    conf->meta_thrift_port = NGX_CONF_UNSET_UINT;
    return conf;
}

static char *ngx_photo_thrift_merge_conf(ngx_conf_t *cf, void *parent, void *child) {
    photo_thrift_conf_t *prev = parent;
    photo_thrift_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->content_thrift_server, prev->content_thrift_server, "127.0.0.1");
    ngx_conf_merge_str_value(conf->content_thrift_port, prev->content_thrift_port, "10021");
    ngx_conf_merge_str_value(conf->meta_thrift_server, prev->meta_thrift_server, "127.0.0.1");
    ngx_conf_merge_str_value(conf->meta_thrift_port, prev->meta_thrift_port, "10022");
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_photo_thrift_handler(ngx_http_request_t *req) {
    ngx_int_t rc;
    photo_thrift_conf_t *clcf;
    clcf = ngx_http_get_module_loc_conf(req, ngx_photo_thrift_module);
    char content_server_address[20];
    char meta_server_address[20];
    ngx_int_t content_server_port, meta_server_port;
    content_server_port = ngx_atoi(clcf->content_thrift_port.data, clcf->content_thrift_port.len);
    meta_server_port = ngx_atoi(clcf->meta_thrift_port.data, clcf->meta_thrift_port.len);
    strncpy(content_server_address, (const char *) clcf->content_thrift_server.data, clcf->content_thrift_server.len);
    strncpy(meta_server_address, (const char *) clcf->meta_thrift_server.data, clcf->meta_thrift_server.len);
    init_thrift_connection_pool(30, meta_server_address, (int) meta_server_port, content_server_address, (int) content_server_port);

    if (!(req->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
        return NGX_HTTP_NOT_ALLOWED;

    if (req->uri.data[req->uri.len - 1] == '/')
        return NGX_DECLINED;

    rc = ngx_http_discard_request_body(req);

    if (rc != NGX_OK)
        return rc;
    if (req->headers_in.if_match != NULL || req->headers_in.if_none_match != NULL)
        return NGX_HTTP_NOT_MODIFIED;
    ngx_log_t *nlog = req->connection->log;
    ngx_chain_t bucket;
    ngx_str_t request_uri;
    request_uri.len = req->uri.len + 1;
    request_uri.data = ngx_pcalloc(req->pool, req->uri.len + 1);
    if (!request_uri.data) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    ngx_cpystrn(request_uri.data, req->uri.data, request_uri.len);
    request_info_ *request_info;
    request_info = ngx_pcalloc(req->pool, sizeof (request_info_));
    if (!request_info) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    char *request_size;
    PARSE_NAME_SIZE((char *) request_uri.data + 1);

    if (request_size == NULL) {
        request_info->size = 0; // When user don't give size
    } else {
        char *tmp = NULL; // to check input size correct or not
        request_info->size = strtoul(request_size, &tmp, 0);
        if ((request_info->size == 0) || (*tmp != 0)) { // When user gives WRONG size 
            ngx_pfree(req->pool, request_info);
            ngx_pfree(req->pool, request_uri.data);
            return NGX_HTTP_BAD_REQUEST;
        }
    }
    long unsigned int hash_key = murmur3_simple((const void*) request_info->file_name);
    return_value* full_photo_info;
    full_photo_info = ngx_pcalloc(req->pool, sizeof (return_value));
    if (!full_photo_info) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    full_photo_info = kv_up_get(hash_key, request_info->size);
    if (!full_photo_info) return NGX_HTTP_NOT_FOUND;
    ngx_buf_t *b = ngx_pcalloc(req->pool, sizeof (ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    bucket.buf = b;
    bucket.next = NULL;
    b->pos = (u_char *) full_photo_info->content;
    b->last = (u_char *) full_photo_info->content + (full_photo_info->size * sizeof (unsigned char));
    b->memory = 1;
    b->last_buf = 1;
    req->root_tested = !req->error_page;
    if (bucket.buf != NULL) {
        req->allow_ranges = 1;
        nlog->action = "sending photo to client";
        req->headers_out.status = NGX_HTTP_OK;
        req->headers_out.content_length_n = (off_t) full_photo_info->size;
        if (full_photo_info->contentType == NULL) { // upload did not put content type field
            req->headers_out.content_type.len = sizeof ("image/jpg") - 1;
            req->headers_out.content_type.data = (u_char *) "image/jpg";
        } else {
            req->headers_out.content_type.len = (size_t) full_photo_info->content_type_size;
            req->headers_out.content_type.data = (u_char *) full_photo_info->contentType;
        }
        if (ngx_http_set_content_type(req) != NGX_OK) return NGX_HTTP_INTERNAL_SERVER_ERROR;
        ngx_table_elt_t *h = ngx_list_push(&req->headers_out.headers);
        if (h == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
        h->hash = 1;
        h->key.len = sizeof ("Etag") - 1;
        h->key.data = (u_char *) "Etag";
        h->value.len = (size_t) full_photo_info->etag_size;
        h->value.data = (u_char *) full_photo_info->etag;
        rc = ngx_http_send_header(req);
        if (rc == NGX_ERROR || rc > NGX_OK || req->header_only) {
            ngx_log_error(NGX_LOG_ALERT, nlog, ngx_errno, ngx_close_file_n "ngx_http_send_header failed");
            return rc;
        }
        ngx_pfree(req->pool, full_photo_info);
        ngx_pfree(req->pool, request_info);
        if (request_uri.data) ngx_pfree(req->pool, request_uri.data);
        return ngx_http_output_filter(req, &bucket);
    } else return NGX_HTTP_UNSUPPORTED_MEDIA_TYPE;
    return NGX_HTTP_OK;
}

static char *ngx_photo_thrift(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf =
            ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_photo_thrift_handler;
    return NGX_CONF_OK;
}



