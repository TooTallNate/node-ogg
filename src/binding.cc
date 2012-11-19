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
#include <string.h>

#include "node_buffer.h"
#include "node_pointer.h"
#include "binding.h"
#include "ogg/ogg.h"

using namespace v8;
using namespace node;

namespace nodeogg {

Handle<Value> node_ogg_sync_init (const Arguments& args) {
  HandleScope scope;
  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  Handle<Value> rtn = Integer::New(ogg_sync_init(oy));
  return scope.Close(rtn);
}

/* combination of "ogg_sync_buffer", "memcpy", and "ogg_sync_wrote" on the thread
 * pool.
 */

Handle<Value> node_ogg_sync_write (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[3]);

  write_req *req = new write_req;
  req->oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  req->buffer = reinterpret_cast<char *>(UnwrapPointer(args[1]));
  req->size = static_cast<long>(args[2]->NumberValue());
  req->rtn = 0;
  req->callback = Persistent<Function>::New(callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(), &req->req, node_ogg_sync_write_async, node_ogg_sync_write_after);
  return Undefined();
}

void node_ogg_sync_write_async (uv_work_t *req) {
  write_req *wreq = reinterpret_cast<write_req *>(req->data);
  long size = wreq->size;
  char *buffer = ogg_sync_buffer(wreq->oy, size);
  memcpy(buffer, wreq->buffer, size);
  wreq->rtn = ogg_sync_wrote(wreq->oy, size);
}

void node_ogg_sync_write_after (uv_work_t *req) {
  HandleScope scope;
  write_req *wreq = reinterpret_cast<write_req *>(req->data);

  Handle<Value> argv[1] = { Integer::New(wreq->rtn) };

  TryCatch try_catch;

  wreq->callback->Call(Context::GetCurrent()->Global(), 1, argv);

  // cleanup
  wreq->callback.Dispose();
  delete wreq;

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

/* Reads out an `ogg_page` struct. */

Handle<Value> node_ogg_sync_pageout (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pageout_req *req = new pageout_req;
  req->oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->serialno = -1;
  req->packets = -1;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback = Persistent<Function>::New(callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(), &req->req, node_ogg_sync_pageout_async, node_ogg_sync_pageout_after);
  return Undefined();
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
  HandleScope scope;
  pageout_req *preq = reinterpret_cast<pageout_req *>(req->data);

  Handle<Value> argv[3] = {
    Integer::New(preq->rtn),
    Integer::New(preq->serialno),
    Integer::New(preq->packets)
  };

  TryCatch try_catch;

  preq->callback->Call(Context::GetCurrent()->Global(), 3, argv);

  // cleanup
  preq->callback.Dispose();
  delete preq;

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}


Handle<Value> node_ogg_stream_init (const Arguments& args) {
  HandleScope scope;
  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  int serialno = args[1]->IntegerValue();
  Handle<Value> rtn = Integer::New(ogg_stream_init(os, serialno));
  return scope.Close(rtn);
}


/* Writes a `ogg_page` struct into a `ogg_stream_state`. */

Handle<Value> node_ogg_stream_pagein (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pagein_req *req = new pagein_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  req->callback = Persistent<Function>::New(callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(), &req->req, node_ogg_stream_pagein_async, node_ogg_stream_pagein_after);
  return Undefined();
}

void node_ogg_stream_pagein_async (uv_work_t *req) {
  pagein_req *preq = reinterpret_cast<pagein_req *>(req->data);
  preq->rtn = ogg_stream_pagein(preq->os, preq->page);
}

void node_ogg_stream_pagein_after (uv_work_t *req) {
  HandleScope scope;
  pagein_req *preq = reinterpret_cast<pagein_req *>(req->data);
  Handle<Value> argv[1] = { Integer::New(preq->rtn) };

  TryCatch try_catch;
  preq->callback->Call(Context::GetCurrent()->Global(), 1, argv);

  // cleanup
  preq->callback.Dispose();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Reads a `ogg_packet` struct from a `ogg_stream_state`. */
Handle<Value> node_ogg_stream_packetout (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[2]);

  packetout_req *req = new packetout_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  req->callback = Persistent<Function>::New(callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(), &req->req, node_ogg_stream_packetout_async, node_ogg_stream_packetout_after);
  return Undefined();
}

void node_ogg_stream_packetout_async (uv_work_t *req) {
  packetout_req *preq = reinterpret_cast<packetout_req *>(req->data);
  preq->rtn = ogg_stream_packetout(preq->os, preq->packet);
}

void node_ogg_stream_packetout_after (uv_work_t *req) {
  HandleScope scope;
  packetout_req *preq = reinterpret_cast<packetout_req *>(req->data);
  ogg_packet *p = preq->packet;
  Handle<Value> argv[6];
  argv[0] = Integer::New(preq->rtn);
  if (preq->rtn == 1) {
    argv[1] = Number::New(p->bytes);
    argv[2] = Number::New(p->b_o_s);
    argv[3] = Number::New(p->e_o_s);
    argv[4] = Number::New(p->granulepos);
    argv[5] = Number::New(p->packetno);
  } else {
    argv[1] = Null();
    argv[2] = Null();
    argv[3] = Null();
    argv[4] = Null();
    argv[5] = Null();
  }

  TryCatch try_catch;
  preq->callback->Call(Context::GetCurrent()->Global(), 6, argv);

  // cleanup
  preq->callback.Dispose();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


/* Writes a `ogg_packet` struct to a `ogg_stream_state`. */
Handle<Value> node_ogg_stream_packetin (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[2]);

  packetin_req *req = new packetin_req;
  req->os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  req->callback = Persistent<Function>::New(callback);
  req->req.data = req;

  uv_queue_work(uv_default_loop(), &req->req, node_ogg_stream_packetin_async, node_ogg_stream_packetin_after);
  return Undefined();
}

void node_ogg_stream_packetin_async (uv_work_t *req) {
  packetin_req *preq = reinterpret_cast<packetin_req *>(req->data);
  preq->rtn = ogg_stream_packetin(preq->os, preq->packet);
}

void node_ogg_stream_packetin_after (uv_work_t *req) {
  HandleScope scope;
  packetin_req *preq = reinterpret_cast<packetin_req *>(req->data);

  Handle<Value> argv[1] = { Integer::New(preq->rtn) };

  TryCatch try_catch;
  preq->callback->Call(Context::GetCurrent()->Global(), 1, argv);

  // cleanup
  preq->callback.Dispose();
  delete preq;

  if (try_catch.HasCaught()) FatalException(try_catch);
}


Handle<Value> node_ogg_stream_eos (const Arguments& args) {
  HandleScope scope;
  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  return scope.Close(Integer::New(ogg_stream_eos(os)));
}


Handle<Value> node_ogg_packet_create (const Arguments& args) {
  HandleScope scope;
  Buffer *b = Buffer::New(sizeof(ogg_packet));
  ogg_packet *p = reinterpret_cast<ogg_packet *>(Buffer::Data(b));
  Local<Object> o = args[0]->ToObject();
  Local<Object> buf = o->Get(String::NewSymbol("data"))->ToObject();
  p->packet = reinterpret_cast<unsigned char *>(UnwrapPointer(buf));
  p->bytes = Buffer::Length(buf);
  p->b_o_s = o->Get(String::NewSymbol("b_o_s"))->IntegerValue();
  p->e_o_s = o->Get(String::NewSymbol("e_o_s"))->IntegerValue();
  p->granulepos = o->Get(String::NewSymbol("granulepos"))->IntegerValue();
  p->packetno = o->Get(String::NewSymbol("packetno"))->IntegerValue();

  /* save a ref to the Object so that the "data" buffer doesn't get GC'd */
  b->handle_->Set(String::NewSymbol("o"), o);

  return scope.Close(b->handle_);
}


void Initialize(Handle<Object> target) {
  HandleScope scope;

  /* sizeof's */
#define SIZEOF(value) \
  target->Set(String::NewSymbol("sizeof_" #value), Integer::New(sizeof(value)), \
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
  NODE_SET_METHOD(target, "ogg_stream_eos", node_ogg_stream_eos);

  /* for Encoder testing purposes... */
  NODE_SET_METHOD(target, "ogg_packet_create", node_ogg_packet_create);

}

} // nodeogg namespace

NODE_MODULE(ogg, nodeogg::Initialize);
