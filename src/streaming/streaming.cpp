#include "dosbox.h"
#include "setup.h"
#include "server.h"
#include "test.h"
#include "encoder.h"
#include "SDL.h"

#define APP_FRAME_BUFFER_SIZE (1024*1024)

typedef enum {
	jsmpeg_frame_type_video = 0xFA010000,
	jsmpeg_frame_type_audio = 0xFB010000
} jsmpeg_trame_type_t;

typedef struct {
	jsmpeg_trame_type_t type;
	int size;
	char data[0];
} jsmpeg_frame_t;

typedef struct {
	unsigned char magic[4];
	unsigned short width;
	unsigned short height;
} jsmpeg_header_t;

int swap_int32(int in) {
	return ((in>>24)&0xff) |
		((in<<8)&0xff0000) |
		((in>>8)&0xff00) |
		((in<<24)&0xff000000);
}

int swap_int16(int in) {
	return ((in>>8)&0xff) | ((in<<8)&0xff00);
}

static struct {
  Bit8u* framebuffer;
  jsmpeg_frame_t *frame;
  encoder_t* encoder = NULL;
  struct server_t* server;
} streaming;

void STREAMING_on_connect(server_t *server, lws *socket) {
	printf("\nclient connected: %s\n", server_get_client_address(streaming.server, socket));

	jsmpeg_header_t header = {		
		{'j','s','m','p'}, 
		swap_int16(streaming.encoder->out_width), swap_int16(streaming.encoder->out_height)
	};
	server_send(server, socket, &header, sizeof(header), server_type_binary);
}

int STREAMING_on_http_req(server_t *server, struct lws *socket, char *request) {
	LOG_MSG("STREAMING_on_http_req");
	//printf("http request: %s\n", request);
	if( strcmp(request, "/") == 0 ) {
		lws_serve_http_file( socket, "client/index.html", "text/html; charset=utf-8", NULL, 0);
		return true;
	}
	else if( strcmp(request, "/jsmpg.js") == 0 ) {
		lws_serve_http_file( socket, "client/jsmpg.js", "text/javascript; charset=utf-8", NULL, 0);
		return true;
	}
	else if( strcmp(request, "/jsmpg-vnc.js") == 0 ) {
		lws_serve_http_file( socket, "client/jsmpg-vnc.js", "text/javascript; charset=utf-8", NULL, 0);
		return true;
	}
	return false;
}

void STREAMING_AddImage(Bitu width, Bitu height, Bitu bpp, Bitu pitch, Bit8u* data, Bit8u* pal) {
	// Adds an image to buffer
	//
	//void *encoded_data;
	//encoder_encode(streaming.encoder, framebuffer, )
	// if bpp == 8
	// for each byte in data
	// with coordinates x, y

    // Decode VGA framebuffer into format readable by encoder
 //    jsmpeg_header_t header = {		
	// 	{'j','s','m','p'}, 
	// 	swap_int16(streaming.encoder->out_width), swap_int16(streaming.encoder->out_height)
	// };
	// server_broadcast(streaming.server, &header, sizeof(header), server_type_binary);

	if (bpp == 8)
	{
		for (int y = 0; y < height; ++y)
		{
			Bit8u* srcLine = data + y*pitch;
			for (int x = 0; x < width; ++x)
			{
				//int pixel = srcLine[x];//((int*)pal)[srcLine[x]]; // 0x00ff0000;//
				//pixel = (pixel) & 0xffff;
				//pixel |= 0xffff00;
				Bit8u* pixel = streaming.framebuffer + y*width*4+x*4;
				Bit8u* color = pal + srcLine[x]*4;
				pixel[0] = color[2];
				pixel[1] = color[1];
				pixel[2] = color[0];
				//memcpy(pixel, color, 4);
			}
		}
	}

	size_t encoded_size = APP_FRAME_BUFFER_SIZE - sizeof(jsmpeg_frame_t);
	encoder_encode(streaming.encoder, streaming.framebuffer, streaming.frame->data, &encoded_size);

	if( encoded_size ) {
		streaming.frame->size = swap_int32(sizeof(jsmpeg_frame_t) + encoded_size);
		server_broadcast(streaming.server, streaming.frame, sizeof(jsmpeg_frame_t) + encoded_size, server_type_binary);
	}

	server_update(streaming.server);
	//LOG_MSG("STREAMING_AddImage: %d", pitch);
}

void STREAMING_OnSetSize(Bitu width, Bitu height) {
	printf("%s\n", "STREAMING_OnSetSize");
	// Notify clients that the video size have changed
	if(streaming.encoder) {
		encoder_destroy(streaming.encoder);
	}
	streaming.encoder = encoder_create(width, height, width, height, width*2*1500);

	jsmpeg_header_t header = {		
		{'j','s','m','p'}, 
		swap_int16(streaming.encoder->out_width), swap_int16(streaming.encoder->out_height)
	};
	server_broadcast(streaming.server, &header, sizeof(header), server_type_binary);
	server_update(streaming.server);
}

void STREAMING_Destroy(Section * sec) {
	//delete test;
	LOG_MSG("STREAMING_Destroy");
}

int STREAMING_ServerLoop(void *ptr) {
	while(1) {
		server_update(streaming.server);
		SDL_Delay(1);
	}
}

void STREAMING_Init(Section * sec) {
	//test = new HARDWARE(sec);
	streaming.frame = (jsmpeg_frame_t *)malloc(APP_FRAME_BUFFER_SIZE);
	streaming.frame->type = jsmpeg_frame_type_video;
	streaming.frame->size = 0;

	sec->AddDestroyFunction(&STREAMING_Destroy,true);
	LOG_MSG("STREAMING_Init");
	jsmpeg_frame_t *frame = (jsmpeg_frame_t *)malloc(APP_FRAME_BUFFER_SIZE);
	frame->type = jsmpeg_frame_type_video;
	frame->size = 0;

	streaming.server = server_create(8080, 200*1500);
	streaming.server->on_http_req = STREAMING_on_http_req;
    streaming.server->on_connect = STREAMING_on_connect;

	streaming.encoder = encoder_create(320, 200, 320, 200, 320*1500);
	streaming.framebuffer = (Bit8u* )malloc(1024*1024*4); // Allocate a buffer large enough for any mode
	//SDL_CreateThread(STREAMING_ServerLoop, NULL);
	//test3();
	//test4(1);
}
