#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include "server.h"

#pragma comment(lib, "websockets_static.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")

struct client_t *client_insert(struct client_t **head, struct libwebsocket *socket) {
	struct client_t *client = (struct client_t *)malloc(sizeof(struct client_t));
	client->socket = socket;
	client->next = *head;
	*head = client;
	return client;
}

void client_remove(struct client_t **head, struct libwebsocket *socket) {
	for( struct client_t **current = head; *current; current = &(*current)->next ) {
		if( (*current)->socket == socket ) {
			struct client_t* next = (*current)->next;
			free(*current);
			*current = next;
			break;
		}
	}
}


static int callback_http(struct libwebsocket_context *, struct libwebsocket *, enum libwebsocket_callback_reasons, void *, void *, size_t);
static int callback_websockets(struct libwebsocket_context *, struct libwebsocket *, enum libwebsocket_callback_reasons,void *, void *, size_t);

static struct libwebsocket_protocols server_protocols[] = {
	{ "http-only", callback_http, 0 },
	{ NULL, callback_websockets, sizeof(int), 1024*1024 },
	{ NULL, NULL, 0 /* End of list */ }
};

struct server_t *server_create(int port, size_t buffer_size) {
	struct server_t *self = (struct server_t *)malloc(sizeof(struct server_t));
	memset(self, 0, sizeof(struct server_t));
	
	self->buffer_size = buffer_size;
	size_t full_buffer_size = LWS_SEND_BUFFER_PRE_PADDING + buffer_size + LWS_SEND_BUFFER_POST_PADDING;
	self->send_buffer_with_padding = (unsigned char *)malloc(full_buffer_size);
	self->send_buffer = &self->send_buffer_with_padding[LWS_SEND_BUFFER_PRE_PADDING];

	self->port = port;
	self->clients = NULL;

	lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

	struct lws_context_creation_info info = {0};
	info.port = port;
	info.gid = -1;
	info.uid = -1;
	info.user = (void *)self;
	info.protocols = server_protocols;
	self->context = libwebsocket_create_context(&info);

	if( !self->context ) {
		server_destroy(self);
		return NULL;
	}

	return self;
}

void server_destroy(struct server_t *self) {
	if( self == NULL ) { return; }

	if( self->context ) {
		libwebsocket_context_destroy(self->context);
	}
	
	free(self->send_buffer_with_padding);
	free(self);
}

char *server_get_host_address(struct server_t *self) {
	// TODO: Get the ip of an interface that was used to create connection, not just some default interface
	// Try lws_get_vhost_*() family of functions?
	return "127.0.0.1";
	// char host_name[80];
	// struct hostent *host;
	// if( 
	// 	gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR ||
	// 	!(host = gethostbyname(host_name))
	// ) {
	// 	return "127.0.0.1";
	// }

	// return inet_ntoa( *(struct IN_ADDR *)(host->h_addr_list[0]) );
}

char *server_get_client_address(struct server_t *self, struct libwebsocket *wsi) {
	static char ip_buffer[32];
	static char name_buffer[32];

	libwebsockets_get_peer_addresses(
		self->context, wsi, libwebsocket_get_socket_fd(wsi), 
		name_buffer, sizeof(name_buffer), 
		ip_buffer, sizeof(ip_buffer)
	);
	return ip_buffer;
}

void server_update(struct server_t *self) {
	libwebsocket_callback_on_writable_all_protocol(&(server_protocols[1])); // Calls either callback_websockets or callback_http
	libwebsocket_service(self->context, 0); // "Service" as in "service" a car: update the server, process requests, etc...
}

// Send to only particular client(socket)
void server_send(struct server_t *self, struct libwebsocket *socket, void *data, size_t size, server_data_type_t type) {
	// Caution, this may explode! The libwebsocket docs advise against ever calling libwebsocket_write()
	// outside of LWS_CALLBACK_SERVER_WRITEABLE. Honoring this advise would complicate things quite
	// a bit - and it seems to work just fine on my system as it is anyway.
	// This won't work reliably on mobile systems where network buffers are typically much smaller.
	// ¯\_(ツ)_/¯

	if( size > self->buffer_size ) {
		printf("Cant send %d bytes; exceeds buffer size (%d bytes)\n", size, self->buffer_size);
		return;
	}
	memcpy(self->send_buffer, data, size);
	libwebsocket_write(socket, self->send_buffer, size, (enum libwebsocket_write_protocol)type);
}

// Broadcast to every client(called socket)
void server_broadcast(struct server_t *self, void *data, size_t size, server_data_type_t type) {
	if( size > self->buffer_size ) {
		printf("Cant send %d bytes; exceeds buffer size (%d bytes)\n", size, self->buffer_size);
		return;
	}
	memcpy(self->send_buffer, data, size);

	client_foreach(self->clients, client) {
		libwebsocket_write(client->socket, self->send_buffer, size, (enum libwebsocket_write_protocol)type);
	}
	libwebsocket_callback_on_writable_all_protocol(&(server_protocols[1]));
}

static int callback_websockets(
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len
) {
	struct server_t *self = (struct server_t *)libwebsocket_context_user(context);

	switch( reason ) {
		case LWS_CALLBACK_ESTABLISHED:
			client_insert(&self->clients, wsi);

			if( self->on_connect ) {
				self->on_connect(self, wsi);
			}
			break;

		case LWS_CALLBACK_RECEIVE:
			if( self->on_message ) {
				self->on_message(self, wsi, in, len);
			}
			break;

		case LWS_CALLBACK_CLOSED:
			client_remove(&self->clients, wsi);		
		
			if( self->on_close ) {
				self->on_close(self, wsi);
			}
			break;
	}
	
	return 0;
}

static int callback_http(
	struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason, void *user,
	void *in, size_t len
) {
	struct server_t *self = (struct server_t *)libwebsocket_context_user(context); // Get user data
	
	if( reason == LWS_CALLBACK_HTTP ) {
		if( self->on_http_req && self->on_http_req(self, wsi, (char *)in) ) { // Call http handler and check response code
			return 0;
		}
		libwebsockets_return_http_status(context, wsi, HTTP_STATUS_NOT_FOUND, NULL); // Handle error response code
		return 0;
	}
	
	return 0;
}

