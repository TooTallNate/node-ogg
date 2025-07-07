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

#include <node.h>
#include <nan.h>
#include <string.h>

#include "node_buffer.h"
#include "node_pointer.h"

#include "ogg/ogg.h"

using namespace v8;
using namespace node;

namespace nodeogg {

NAN_METHOD(node_ogg_sync_init) {
  Nan::HandleScope scope;
  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(info[0]));
  Local<Value> rtn = Nan::New<Integer>(ogg_sync_init(oy));
  info.GetReturnValue().Set(rtn);
}

/* combination of "ogg_sync_buffer", "memcpy", and "ogg_sync_wrote" on the thread
 * pool.
 */
class OggSyncWriteWorker : public Nan::AsyncWorker {
 public:
  OggSyncWriteWorker(ogg_sync_state *oy, char *buffer, long size,
    Nan::Callback *callback)
    : Nan::AsyncWorker(callback), oy(oy), buffer(buffer), size(size), rtn(0) { }
  ~OggSyncWriteWorker () { }
  void Execute() {
    char *localBuffer = ogg_sync_buffer(oy, size);
    memcpy(localBuffer, buffer, size);
    rtn = ogg_sync_wrote(oy, size);
  }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[1] = { Nan::New<Integer>(rtn) };
    Nan::AsyncResource resource("ogg:sync_write");
    callback->Call(1, argv, &resource);
  }
 private:
  ogg_sync_state *oy;
  char *buffer;
  long size;
  int rtn;
};

NAN_METHOD(node_ogg_sync_write) {
  Nan::HandleScope scope;

  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(info[0]));
  char *buffer = reinterpret_cast<char *>(UnwrapPointer(info[1]));
  long size = static_cast<long>(info[2]->NumberValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked());
  Nan::Callback *callback = new Nan::Callback(info[3].As<Function>());

  Nan::AsyncQueueWorker(new OggSyncWriteWorker(oy, buffer, size, callback));
}

/* Reads out an `ogg_page` struct. */
class OggSyncPageoutWorker : public Nan::AsyncWorker {
 public:
  OggSyncPageoutWorker (ogg_sync_state *oy, ogg_page *page, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), oy(oy), page(page), serialno(-1), packets(-1), rtn(0)
    { }
  ~OggSyncPageoutWorker () { }
  void Execute () {
    rtn = ogg_sync_pageout(oy, page);
    if (rtn == 1) {
      serialno = ogg_page_serialno(page);
      packets = ogg_page_packets(page);
    }
  }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[3] = {
      Nan::New<Integer>(rtn),
      Nan::New<Integer>(serialno),
      Nan::New<Integer>(packets)
    };
    Nan::AsyncResource resource("ogg:sync_pageout");
    callback->Call(3, argv, &resource);
  }
 private:
  ogg_sync_state *oy;
  ogg_page *page;
  int serialno;
  int packets;
  int rtn;
};

NAN_METHOD(node_ogg_sync_pageout) {
  Nan::HandleScope scope;

  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(info[0]));
  ogg_page *page = reinterpret_cast<ogg_page *>(UnwrapPointer(info[1]));
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(new OggSyncPageoutWorker(oy, page, callback));
}

NAN_METHOD(node_ogg_stream_init) {
  Nan::HandleScope scope;
  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0]));
  int serialno = static_cast<int>(info[1]->IntegerValue(v8::Isolate::GetCurrent()->GetCurrentContext()).ToChecked());
  info.GetReturnValue().Set(Nan::New<Integer>(ogg_stream_init(os, serialno)));
}


/* Writes a `ogg_page` struct into a `ogg_stream_state`. */
class OggStreamPageinWorker : public Nan::AsyncWorker {
 public:
  OggStreamPageinWorker(ogg_stream_state *os, ogg_page *page, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), os(os), page(page), rtn(0) { }
  void Execute () {
    rtn = ogg_stream_pagein(os, page);
  }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[1] = { Nan::New<Integer>(rtn) };
    Nan::AsyncResource resource("ogg:stream_pagein");
    callback->Call(1, argv, &resource);
  }
 private:
  ogg_stream_state *os;
  ogg_page *page;
  int rtn;
};

NAN_METHOD(node_ogg_stream_pagein) {
  Nan::HandleScope scope;

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0]));
  ogg_page *page = reinterpret_cast<ogg_page *>(UnwrapPointer(info[1]));
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(new OggStreamPageinWorker(os, page, callback));
}


/* Reads a `ogg_packet` struct from a `ogg_stream_state`. */
class OggStreamPacketoutWorker : public Nan::AsyncWorker {
 public:
  OggStreamPacketoutWorker (ogg_stream_state *os, ogg_packet *packet, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), os(os), packet(packet), rtn(0) { }
  ~OggStreamPacketoutWorker () { }
  void Execute () {
    rtn = ogg_stream_packetout(os, packet);
  }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[6];
    argv[0] = Nan::New<Integer>(rtn);
    if (rtn == 1) {
      argv[1] = Nan::New<Number>(packet->bytes);
      argv[2] = Nan::New<Number>(packet->b_o_s);
      argv[3] = Nan::New<Number>(packet->e_o_s);
      argv[4] = Nan::New<Number>(static_cast<double>(packet->granulepos));
      argv[5] = Nan::New<Number>(static_cast<double>(packet->packetno));
    } else {
      argv[1] = Nan::Null();
      argv[2] = Nan::Null();
      argv[3] = Nan::Null();
      argv[4] = Nan::Null();
      argv[5] = Nan::Null();
    }
    Nan::AsyncResource resource("ogg:stream_packetout");
    callback->Call(6, argv, &resource);
  }
 private:
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
};

NAN_METHOD(node_ogg_stream_packetout) {
  Nan::HandleScope scope;

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0]));
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[1]));
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(new OggStreamPacketoutWorker(os, packet, callback));
}


/* Writes a `ogg_packet` struct to a `ogg_stream_state`. */
class OggStreamPacketinWorker : public Nan::AsyncWorker {
 public:
  OggStreamPacketinWorker (ogg_stream_state *os, ogg_packet *packet, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), os(os), packet(packet), rtn(0) { }
  ~OggStreamPacketinWorker () { }
  void Execute () {
    rtn = ogg_stream_packetin(os, packet);
  }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[1] = { Nan::New<Integer>(rtn) };
    Nan::AsyncResource resource("ogg:stream_packetin");
    callback->Call(1, argv, &resource);
  }
 private:
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
};

NAN_METHOD(node_ogg_stream_packetin) {
  Nan::HandleScope scope;

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0]));
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[1]));
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(new OggStreamPacketinWorker(os, packet, callback));
}

// Since both StreamPageout and StreamFlush have the same HandleOKCallback,
// this base class deals with both.
class StreamWorker : public Nan::AsyncWorker {
 public:
  StreamWorker(ogg_stream_state *os, ogg_page *page, Nan::Callback *callback)
      : Nan::AsyncWorker(callback), os(os), page(page), rtn(0) { }
  void HandleOKCallback () {
    Nan::HandleScope scope;

    v8::Local<Value> argv[4];
    argv[0] = Nan::New<Integer>(rtn);
    if (rtn == 0) {
      /* need more data */
      argv[1] = Nan::Null();
      argv[2] = Nan::Null();
      argv[3] = Nan::Null();
    } else {
      /* got a page! */
      argv[1] = Nan::New<Number>(page->header_len);
      argv[2] = Nan::New<Number>(page->body_len);
      argv[3] = Nan::New<Integer>(ogg_page_eos(page));
    }
    Nan::AsyncResource resource("ogg:stream_worker");
    callback->Call(4, argv, &resource);
  }
 protected:
  ogg_stream_state *os;
  ogg_page *page;
  int rtn;
};

class StreamPageoutWorker : public StreamWorker {
 public:
  StreamPageoutWorker(ogg_stream_state *os, ogg_page *page, Nan::Callback *callback)
    : StreamWorker(os, page, callback) { }
  ~StreamPageoutWorker() { }
  void Execute () {
    rtn = ogg_stream_pageout(os, page);
  }
};

/* Reads out a `ogg_page` struct from an `ogg_stream_state`. */
NAN_METHOD(node_ogg_stream_pageout) {
  Nan::HandleScope scope;
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(
    new StreamPageoutWorker(
      reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0])),
      reinterpret_cast<ogg_page *>(UnwrapPointer(info[1])),
      callback));
}

class StreamFlushWorker : public StreamWorker {
 public:
  StreamFlushWorker(ogg_stream_state *os, ogg_page *page, Nan::Callback *callback)
       : StreamWorker(os, page, callback) { }
  ~StreamFlushWorker() { }
  void Execute () {
    rtn = ogg_stream_flush(os, page);
  }
};

/* Forces an `ogg_page` struct to be flushed from an `ogg_stream_state`. */
NAN_METHOD(node_ogg_stream_flush) {
  Nan::HandleScope scope;
  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());

  Nan::AsyncQueueWorker(
    new StreamFlushWorker(
      reinterpret_cast<ogg_stream_state *>(UnwrapPointer(info[0])),
      reinterpret_cast<ogg_page *>(UnwrapPointer(info[1])),
      callback));
}

/* Converts an `ogg_page` instance to a node Buffer instance */
NAN_METHOD(node_ogg_page_to_buffer) {
  Nan::HandleScope scope;

  ogg_page *op = reinterpret_cast<ogg_page *>(UnwrapPointer(info[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(info[1]));
  memcpy(buf, op->header, op->header_len);
  memcpy(buf + op->header_len, op->body, op->body_len);
}


/* packet->packet = ... */
NAN_METHOD(node_ogg_packet_set_packet) {
  Nan::HandleScope scope;
  ogg_packet *packet = UnwrapPointer<ogg_packet *>(info[0]);
  packet->packet = UnwrapPointer<unsigned char *>(info[1]);

}


/* packet->packet */
NAN_METHOD(node_ogg_packet_get_packet) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  Nan::MaybeLocal<v8::Object> wrapper = WrapPointer(packet->packet, packet->bytes);
  info.GetReturnValue().Set(wrapper.ToLocalChecked());
}


/* packet->bytes */
NAN_METHOD(node_ogg_packet_bytes) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  info.GetReturnValue().Set(Nan::New<Number>(packet->bytes));
}


/* packet->b_o_s */
NAN_METHOD(node_ogg_packet_b_o_s) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  info.GetReturnValue().Set(Nan::New<Number>(packet->b_o_s));
}


/* packet->e_o_s */
NAN_METHOD(node_ogg_packet_e_o_s) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  info.GetReturnValue().Set(Nan::New<Number>(packet->e_o_s));
}


/* packet->granulepos */
NAN_METHOD(node_ogg_packet_granulepos) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  info.GetReturnValue().Set(Nan::New<Number>(static_cast<double>(packet->granulepos)));
}


/* packet->packetno */
NAN_METHOD(node_ogg_packet_packetno) {
  Nan::HandleScope scope;
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  info.GetReturnValue().Set(Nan::New<Number>(static_cast<double>(packet->packetno)));
}


/* Replaces the `ogg_packet` "packet" pointer with a Node.js buffer instance */
NAN_METHOD(node_ogg_packet_replace_buffer) {
  Nan::HandleScope scope;

  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(info[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(info[1]));
  memcpy(buf, packet->packet, packet->bytes);
  packet->packet = buf;
}


NAN_MODULE_INIT(Initialize) {
  Nan::HandleScope scope;

  /* sizeof's */
#define SIZEOF(value) \
  Nan::Set(target, Nan::New<String>("sizeof_" #value).ToLocalChecked(), \
      Nan::New<Integer>(static_cast<int32_t>(sizeof(value))))
  SIZEOF(ogg_sync_state);
  SIZEOF(ogg_stream_state);
  SIZEOF(ogg_page);
  SIZEOF(ogg_packet);

  Nan::SetMethod(target, "ogg_sync_init", node_ogg_sync_init);
  Nan::SetMethod(target, "ogg_sync_write", node_ogg_sync_write);
  Nan::SetMethod(target, "ogg_sync_pageout", node_ogg_sync_pageout);

  Nan::SetMethod(target, "ogg_stream_init", node_ogg_stream_init);
  Nan::SetMethod(target, "ogg_stream_pagein", node_ogg_stream_pagein);
  Nan::SetMethod(target, "ogg_stream_packetout", node_ogg_stream_packetout);
  Nan::SetMethod(target, "ogg_stream_packetin", node_ogg_stream_packetin);
  Nan::SetMethod(target, "ogg_stream_pageout", node_ogg_stream_pageout);
  Nan::SetMethod(target, "ogg_stream_flush", node_ogg_stream_flush);

  /* custom functions */
  Nan::SetMethod(target, "ogg_page_to_buffer", node_ogg_page_to_buffer);

  Nan::SetMethod(target, "ogg_packet_set_packet", node_ogg_packet_set_packet);
  Nan::SetMethod(target, "ogg_packet_get_packet", node_ogg_packet_get_packet);
  Nan::SetMethod(target, "ogg_packet_bytes", node_ogg_packet_bytes);
  Nan::SetMethod(target, "ogg_packet_b_o_s", node_ogg_packet_b_o_s);
  Nan::SetMethod(target, "ogg_packet_e_o_s", node_ogg_packet_e_o_s);
  Nan::SetMethod(target, "ogg_packet_granulepos", node_ogg_packet_granulepos);
  Nan::SetMethod(target, "ogg_packet_packetno", node_ogg_packet_packetno);
  Nan::Set(target, Nan::New<String>("ogg_packet_replace_buffer").ToLocalChecked(),
    Nan::New<FunctionTemplate>(node_ogg_packet_replace_buffer)->GetFunction(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked());

}

} // nodeogg namespace

NODE_MODULE(NODE_GYP_MODULE_NAME, nodeogg::Initialize)
