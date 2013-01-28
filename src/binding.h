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

typedef struct pagein_req pageout_stream_req;

struct packetout_req {
  uv_work_t req;
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
  v8::Persistent<v8::Function> callback;
};

typedef struct packetout_req packetin_req;

/* decoding */
void node_ogg_sync_write_async (uv_work_t *);
void node_ogg_sync_write_after (uv_work_t *);

void node_ogg_sync_pageout_async (uv_work_t *);
void node_ogg_sync_pageout_after (uv_work_t *);

void node_ogg_stream_pagein_async (uv_work_t *);
void node_ogg_stream_pagein_after (uv_work_t *);

void node_ogg_stream_packetout_async (uv_work_t *);
void node_ogg_stream_packetout_after (uv_work_t *);

/* encoding */
void node_ogg_stream_packetin_async (uv_work_t *);
void node_ogg_stream_packetin_after (uv_work_t *);

void node_ogg_stream_pageout_async (uv_work_t *);
void node_ogg_stream_pageout_after (uv_work_t *);

void node_ogg_stream_flush_async (uv_work_t *);
/* flush_after() is really just pageout_after() */

} // nodeogg namespace
