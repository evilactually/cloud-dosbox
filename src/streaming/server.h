#ifndef SERVER_H
#define SERVER_H

#include "libwebsockets.h"

struct client_t {
	struct lws *socket;
	struct client_t *next;
};

struct client_t *client_insert(struct client_t **head, struct lws *socket);
void client_remove(struct client_t **head, struct lws *socket);
#define client_foreach(HEAD, CLIENT) for(struct client_t *CLIENT = HEAD; CLIENT; CLIENT = CLIENT->next)

typedef enum {
	server_type_text = LWS_WRITE_TEXT,
	server_type_binary = LWS_WRITE_BINARY
} server_data_type_t;

struct server_t {
	struct lws_context *context;
	size_t buffer_size;
	unsigned char *send_buffer_with_padding;
	unsigned char *send_buffer;
	void *user;
	
	int port;
	struct client_t *clients;
	
	void (*on_connect)(struct server_t *server, struct lws *wsi);
	void (*on_message)(struct server_t *server, struct lws *wsi, void *in, size_t len);
	void (*on_close)(struct server_t *server, struct lws *wsi);
	int (*on_http_req)(struct server_t *server, struct lws *wsi, char *request);
};


struct server_t *server_create(int port, size_t buffer_size);
void server_destroy(struct server_t *self);
char *server_get_host_address(struct server_t *self);
char *server_get_client_address(struct server_t *self, struct lws *wsi);
void server_update(struct server_t *self);
void server_send(struct server_t *self, struct lws *socket, void *data, size_t size, server_data_type_t type);
void server_broadcast(struct server_t *self, void *data, size_t size, server_data_type_t type);
//int test5(int port);
#endif
