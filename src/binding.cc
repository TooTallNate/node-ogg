/*
 * Copyright (c) 2012, Nathan Rajlich <nathan@tootallnate.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * libogg API reference:
 * http://xiph.org/ogg/doc/libogg/reference.html
 */

#include <v8.h>
#include <node.h>
// I'm unclear if we're supposed to include uv.h directly now or if we're just
// not supposed to touch it?
#include <uv.h>
#include <string.h>

#include "node_buffer.h"
#include "node_pointer.h"
#include "binding.h"
#include "ogg/ogg.h"

using namespace v8;
using namespace node;

namespace nodeogg {

void node_ogg_sync_init (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  Local<Value> rtn = Integer::New(isolate, ogg_sync_init(oy));
  args.GetReturnValue().Set(rtn);
}

/* combination of "ogg_sync_buffer", "memcpy", and "ogg_sync_wrote" on the thread
 * pool.
 */

void node_ogg_sync_write (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[3]);

  write_req *req = new write_req;
  req->oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  req->buffer = reinterpret_cast<char *>(UnwrapPointer(args[1]));
  req->size = static_cast<long>(args[2]->NumberValue());
  req->rtn = 0;
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_sync_write_async,
                (uv_after_work_cb)node_ogg_sync_write_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_sync_write_async (uv_work_t *req) {
  write_req *wreq = reinterpret_cast<write_req *>(req->data);
  long size = wreq->size;
  char *buffer = ogg_sync_buffer(wreq->oy, size);
  memcpy(buffer, wreq->buffer, size);
  wreq->rtn = ogg_sync_wrote(wreq->oy, size);
}

void node_ogg_sync_write_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  write_req *wreq = reinterpret_cast<write_req *>(req->data);

  Handle<Value> argv[1] = { Integer::New(isolate, wreq->rtn) };

  TryCatch try_catch;

  Local<Function> callback = Local<Function>::New(isolate, wreq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  // cleanup
  wreq->callback.Reset();
  delete wreq;

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

/* Reads out an `ogg_page` struct. */

void node_ogg_sync_pageout (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pageout_req *req = new pageout_req;
  req->oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->serialno = -1;
  req->packets = -1;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_sync_pageout_async,
                (uv_after_work_cb)node_ogg_sync_pageout_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_sync_pageout_async (uv_work_t *req) {
  pageout_req *preq = reinterpret_cast<pageout_req *>(req->data);
  preq->rtn = ogg_sync_pageout(preq->oy, preq->page);
  if (preq->rtn == 1) {
    preq->serialno = ogg_page_serialno(preq->page);
    preq->packets = ogg_page_packets(preq->page);
  }
}

void node_ogg_sync_pageout_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  pageout_req *preq = reinterpret_cast<pageout_req *>(req->data);

  Handle<Value> argv[3] = {
    Integer::New(isolate, preq->rtn),
    Integer::New(isolate, preq->serialno),
    Integer::New(isolate, preq->packets)
  };

  TryCatch try_catch;

  Local<Function> callback = Local<Function>::New(isolate, preq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 3, argv);

  // cleanup
  preq->callback.Reset();
  delete preq;

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}


void node_ogg_stream_init (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  int serialno = args[1]->IntegerValue();
  args.GetReturnValue().Set(ogg_stream_init(os, serialno));
}


/* Writes a `ogg_page` struct into a `ogg_stream_state`. */

void node_ogg_stream_pagein (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pagein_req *req = new pagein_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_stream_pagein_async,
                (uv_after_work_cb)node_ogg_stream_pagein_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_stream_pagein_async (uv_work_t *req) {
  pagein_req *preq = reinterpret_cast<pagein_req *>(req->data);
  preq->rtn = ogg_stream_pagein(preq->os, preq->page);
}

void node_ogg_stream_pagein_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  pagein_req *preq = reinterpret_cast<pagein_req *>(req->data);
  Handle<Value> argv[1] = { Integer::New(isolate, preq->rtn) };

  TryCatch try_catch;
  Local<Function> callback = Local<Function>::New(isolate, preq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  // cleanup
  preq->callback.Reset();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Reads a `ogg_packet` struct from a `ogg_stream_state`. */
void node_ogg_stream_packetout (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  packetout_req *req = new packetout_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_stream_packetout_async,
                (uv_after_work_cb)node_ogg_stream_packetout_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_stream_packetout_async (uv_work_t *req) {
  packetout_req *preq = reinterpret_cast<packetout_req *>(req->data);
  preq->rtn = ogg_stream_packetout(preq->os, preq->packet);
}

void node_ogg_stream_packetout_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  packetout_req *preq = reinterpret_cast<packetout_req *>(req->data);
  ogg_packet *p = preq->packet;
  Handle<Value> argv[6];
  argv[0] = Integer::New(isolate, preq->rtn);
  if (preq->rtn == 1) {
    argv[1] = Number::New(isolate, p->bytes);
    argv[2] = Number::New(isolate, p->b_o_s);
    argv[3] = Number::New(isolate, p->e_o_s);
    argv[4] = Number::New(isolate, p->granulepos);
    argv[5] = Number::New(isolate, p->packetno);
  } else {
    argv[1] = Null(isolate);
    argv[2] = Null(isolate);
    argv[3] = Null(isolate);
    argv[4] = Null(isolate);
    argv[5] = Null(isolate);
  }

  TryCatch try_catch;
  Local<Function> callback = Local<Function>::New(isolate, preq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 6, argv);

  // cleanup
  preq->callback.Reset();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Writes a `ogg_packet` struct to a `ogg_stream_state`. */
void node_ogg_stream_packetin (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  packetin_req *req = new packetin_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_stream_packetin_async,
                (uv_after_work_cb)node_ogg_stream_packetin_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_stream_packetin_async (uv_work_t *req) {
  packetin_req *preq = reinterpret_cast<packetin_req *>(req->data);
  preq->rtn = ogg_stream_packetin(preq->os, preq->packet);
}

void node_ogg_stream_packetin_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  packetin_req *preq = reinterpret_cast<packetin_req *>(req->data);

  Handle<Value> argv[1] = { Integer::New(isolate, preq->rtn) };

  TryCatch try_catch;
  Local<Function> callback = Local<Function>::New(isolate, preq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  // cleanup
  preq->callback.Reset();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Reads out a `ogg_page` struct from an `ogg_stream_state`. */
void node_ogg_stream_pageout (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pageout_stream_req *req = new pageout_stream_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_stream_pageout_async,
                (uv_after_work_cb)node_ogg_stream_pageout_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_stream_pageout_async (uv_work_t *req) {
  pageout_stream_req *preq = reinterpret_cast<pageout_stream_req *>(req->data);
  preq->rtn = ogg_stream_pageout(preq->os, preq->page);
}

void node_ogg_stream_pageout_after (uv_work_t *req) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  pageout_stream_req *preq = reinterpret_cast<pageout_stream_req *>(req->data);

  Handle<Value> argv[4];
  argv[0] = Integer::New(isolate, preq->rtn);
  if (preq->rtn == 0) {
    /* need more data */
    argv[1] = Null(isolate);
    argv[2] = Null(isolate);
    argv[3] = Null(isolate);
  } else {
    /* got a page! */
    argv[1] = Number::New(isolate, preq->page->header_len);
    argv[2] = Number::New(isolate, preq->page->body_len);
    argv[3] = Integer::New(isolate, ogg_page_eos(preq->page));
  }

  TryCatch try_catch;
  Local<Function> callback = Local<Function>::New(isolate, preq->callback);
  callback->Call(isolate->GetCurrentContext()->Global(), 4, argv);

  // cleanup
  preq->callback.Reset();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Forces an `ogg_page` struct to be flushed from an `ogg_stream_state`. */
void node_ogg_stream_flush (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pageout_stream_req *req = new pageout_stream_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback.Reset(isolate, callback);
  req->req.data = req;

  /* reusing the pageout_after() function since the logic is identical... */
  uv_queue_work(uv_default_loop(),
                &req->req,
                node_ogg_stream_flush_async,
                (uv_after_work_cb)node_ogg_stream_pageout_after);
  args.GetReturnValue().SetUndefined();
}

void node_ogg_stream_flush_async (uv_work_t *req) {
  pageout_stream_req *preq = reinterpret_cast<pageout_stream_req *>(req->data);
  preq->rtn = ogg_stream_flush(preq->os, preq->page);
}


/* Converts an `ogg_page` instance to a node Buffer instance */
void node_ogg_page_to_buffer (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  ogg_page *op = reinterpret_cast<ogg_page *>(UnwrapPointer(args[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(args[1]));
  memcpy(buf, op->header, op->header_len);
  memcpy(buf + op->header_len, op->body, op->body_len);

  args.GetReturnValue().SetUndefined();
}


/* packet->packet = ... */
void node_ogg_packet_set_packet (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = UnwrapPointer<ogg_packet *>(args[0]);
  packet->packet = UnwrapPointer<unsigned char *>(args[1]);
  args.GetReturnValue().SetUndefined();
}


/* packet->packet */
void node_ogg_packet_get_packet (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(WrapPointer(packet->packet, packet->bytes));
}


/* packet->bytes */
void node_ogg_packet_bytes (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(packet->bytes);
}


/* packet->b_o_s */
void node_ogg_packet_b_o_s (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(packet->b_o_s);
}


/* packet->e_o_s */
void node_ogg_packet_e_o_s (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(packet->e_o_s);
}


/* packet->granulepos */
void node_ogg_packet_granulepos (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(static_cast<double>(packet->granulepos));
}


/* packet->packetno */
void node_ogg_packet_packetno (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  args.GetReturnValue().Set(static_cast<double>(packet->packetno));
}


/* Replaces the `ogg_packet` "packet" pointer with a Node.js buffer instance */
void node_ogg_packet_replace_buffer (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(args[1]));
  memcpy(buf, packet->packet, packet->bytes);
  packet->packet = buf;

  args.GetReturnValue().SetUndefined();
}


void Initialize(Handle<Object> target) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  /* sizeof's */
#define SIZEOF(value) \
  target->ForceSet(String::NewFromUtf8(isolate, "sizeof_" #value, String::kInternalizedString), Integer::New(isolate, sizeof(value)), \
      static_cast<PropertyAttribute>(ReadOnly|DontDelete))
  SIZEOF(ogg_sync_state);
  SIZEOF(ogg_stream_state);
  SIZEOF(ogg_page);
  SIZEOF(ogg_packet);

  NODE_SET_METHOD(target, "ogg_sync_init", node_ogg_sync_init);
  NODE_SET_METHOD(target, "ogg_sync_write", node_ogg_sync_write);
  NODE_SET_METHOD(target, "ogg_sync_pageout", node_ogg_sync_pageout);

  NODE_SET_METHOD(target, "ogg_stream_init", node_ogg_stream_init);
  NODE_SET_METHOD(target, "ogg_stream_pagein", node_ogg_stream_pagein);
  NODE_SET_METHOD(target, "ogg_stream_packetout", node_ogg_stream_packetout);
  NODE_SET_METHOD(target, "ogg_stream_packetin", node_ogg_stream_packetin);
  NODE_SET_METHOD(target, "ogg_stream_pageout", node_ogg_stream_pageout);
  NODE_SET_METHOD(target, "ogg_stream_flush", node_ogg_stream_flush);

  /* custom functions */
  NODE_SET_METHOD(target, "ogg_page_to_buffer", node_ogg_page_to_buffer);

  NODE_SET_METHOD(target, "ogg_packet_set_packet", node_ogg_packet_set_packet);
  NODE_SET_METHOD(target, "ogg_packet_get_packet", node_ogg_packet_get_packet);
  NODE_SET_METHOD(target, "ogg_packet_bytes", node_ogg_packet_bytes);
  NODE_SET_METHOD(target, "ogg_packet_b_o_s", node_ogg_packet_b_o_s);
  NODE_SET_METHOD(target, "ogg_packet_e_o_s", node_ogg_packet_e_o_s);
  NODE_SET_METHOD(target, "ogg_packet_granulepos", node_ogg_packet_granulepos);
  NODE_SET_METHOD(target, "ogg_packet_packetno", node_ogg_packet_packetno);
  NODE_SET_METHOD(target, "ogg_packet_replace_buffer", node_ogg_packet_replace_buffer);

}

} // nodeogg namespace

NODE_MODULE(ogg, nodeogg::Initialize);
