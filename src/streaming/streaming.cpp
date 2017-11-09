#include "dosbox.h"
#include "setup.h"
#include "server.h"
#include "test.h"

static struct {
  struct server_t* server;
} streaming;

void STREAMING_Destroy(Section * sec) {
	//delete test;
	LOG_MSG("STREAMING_Destroy");
}

void STREAMING_Init(Section * sec) {
	//test = new HARDWARE(sec);
	sec->AddDestroyFunction(&STREAMING_Destroy,true);
	LOG_MSG("STREAMING_Init");
	streaming.server = server_create(8081, 1500);
	//test3();
	//test4(1);
}
