#include "ogg/ogg.h"

namespace nodeogg {

struct write_req {
  uv_work_t req;
  ogg_sync_state *oy;
  char *buffer;
  long size;
  int rtn;
  v8::Persistent<v8::Function> callback;
};

struct pageout_req {
  uv_work_t req;
  ogg_sync_state *oy;
  ogg_page *page;
  int serialno;
  int packets;
  int rtn;
  v8::Persistent<v8::Function> callback;
};

struct pagein_req {
  uv_work_t req;
  ogg_stream_state *os;
  ogg_page *page;
  int rtn;
  v8::Persistent<v8::Function> callback;
};

struct packetout_req {
  uv_work_t req;
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
  v8::Persistent<v8::Function> callback;
};

void node_ogg_sync_write_async (uv_work_t *);
void node_ogg_sync_write_after (uv_work_t *);

void node_ogg_sync_pageout_async (uv_work_t *);
void node_ogg_sync_pageout_after (uv_work_t *);

void node_ogg_stream_pagein_async (uv_work_t *);
void node_ogg_stream_pagein_after (uv_work_t *);

void node_ogg_stream_packetout_async (uv_work_t *);
void node_ogg_stream_packetout_after (uv_work_t *);

} // nodeogg namespace
