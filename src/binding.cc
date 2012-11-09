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

#include <v8.h>
#include <node.h>
#include <string.h>

#include "node_pointer.h"
#include "ogg/ogg.h"

using namespace v8;
using namespace node;

namespace nodeogg {

struct write_req {
  uv_work_t req;
  ogg_sync_state *oy;
  char *buffer;
  long size;
  int rtn;
  Persistent<Function> callback;
};

struct pageout_req {
  uv_work_t req;
  ogg_sync_state *oy;
  ogg_page *page;
  int serialno;
  int rtn;
  Persistent<Function> callback;
};

Handle<Value> node_ogg_sync_init (const Arguments& args) {
  HandleScope scope;
  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  Handle<Value> rtn = Integer::New(ogg_sync_init(oy));
  return scope.Close(rtn);
}

/* combination of "ogg_sync_buffer", "memcpy", and "ogg_sync_wrote" on the thread
 * pool.
 */
void node_ogg_sync_write_async (uv_work_t *);
void node_ogg_sync_write_after (uv_work_t *);

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
  printf("writing %ld bytes\n", size);
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

/* Reads out an `ogg_page` struct.
 */
void node_ogg_sync_pageout_async (uv_work_t *);
void node_ogg_sync_pageout_after (uv_work_t *);

Handle<Value> node_ogg_sync_pageout (const Arguments& args) {
  HandleScope scope;
  Local<Function> callback = Local<Function>::Cast(args[2]);

  pageout_req *req = new pageout_req;
  req->oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  req->rtn = 0;
  req->serialno = -1;
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
  }
}

void node_ogg_sync_pageout_after (uv_work_t *req) {
  HandleScope scope;
  pageout_req *preq = reinterpret_cast<pageout_req *>(req->data);

  Handle<Value> argv[2] = {
    Integer::New(preq->rtn),
    Integer::New(preq->serialno)
  };

  TryCatch try_catch;

  preq->callback->Call(Context::GetCurrent()->Global(), 2, argv);

  // cleanup
  preq->callback.Dispose();
  delete preq;

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

void Initialize(Handle<Object> target) {
  HandleScope scope;

  target->Set(String::NewSymbol("sizeof_ogg_sync_state"), Integer::New(sizeof(ogg_sync_state)));
  target->Set(String::NewSymbol("sizeof_ogg_stream_state"), Integer::New(sizeof(ogg_stream_state)));
  target->Set(String::NewSymbol("sizeof_ogg_page"), Integer::New(sizeof(ogg_page)));

  NODE_SET_METHOD(target, "ogg_sync_init", node_ogg_sync_init);
  NODE_SET_METHOD(target, "ogg_sync_write", node_ogg_sync_write);
  NODE_SET_METHOD(target, "ogg_sync_pageout", node_ogg_sync_pageout);
}

} // nodeogg namespace

NODE_MODULE(binding, nodeogg::Initialize);
