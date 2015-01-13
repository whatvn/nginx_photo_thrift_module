
typedef struct return_value_ return_value;

struct return_value_ {
    char *content;
    unsigned long size;
    char *etag;
    unsigned long etag_size;
    char *contentType;
    unsigned long content_type_size;
};

#ifdef __cplusplus
extern "C" {
#endif

    void init_thrift_connection_pool(unsigned int max_client, char* meta_server_address,
            int meta_server_port, char* content_server_address, int content_server_port);
    return_value *kv_up_get(unsigned long key_get, unsigned long size);
    //    return_value *kv_up_get(unsigned long key_get, char *etag);
    void destroy_ConnectionPool(void);

#ifdef __cplusplus
}
#endif
