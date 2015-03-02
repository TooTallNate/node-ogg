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
  NanScope();
  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  Local<Value> rtn = NanNew<Integer>(ogg_sync_init(oy));
  NanReturnValue(rtn);
}

/* combination of "ogg_sync_buffer", "memcpy", and "ogg_sync_wrote" on the thread
 * pool.
 */
class OggSyncWriteWorker : public NanAsyncWorker {
 public:
  OggSyncWriteWorker(ogg_sync_state *oy, char *buffer, long size,
    NanCallback *callback)
    : oy(oy), buffer(buffer), size(size), rtn(0), NanAsyncWorker(callback) { }
  ~OggSyncWriteWorker () { }
  void Execute() {
    char *localBuffer = ogg_sync_buffer(oy, size);
    memcpy(localBuffer, buffer, size);
    rtn = ogg_sync_wrote(oy, size);
  }
  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[1] = { NanNew<Integer>(rtn) };

    callback->Call(1, argv);
  }
 private:
  ogg_sync_state *oy;
  char *buffer;
  long size;
  int rtn;
};

NAN_METHOD(node_ogg_sync_write) {
  NanScope();

  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  char *buffer = reinterpret_cast<char *>(UnwrapPointer(args[1]));
  long size = static_cast<long>(args[2]->NumberValue());
  NanCallback *callback = new NanCallback(args[3].As<Function>());

  NanAsyncQueueWorker(new OggSyncWriteWorker(oy, buffer, size, callback));
  NanReturnUndefined();
}

/* Reads out an `ogg_page` struct. */
class OggSyncPageoutWorker : public NanAsyncWorker {
 public:
  OggSyncPageoutWorker (ogg_sync_state *oy, ogg_page *page, NanCallback *callback)
    : oy(oy), rtn(0), serialno(-1), packets(-1), page(page), NanAsyncWorker(callback)
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
    NanScope();

    Handle<Value> argv[3] = {
      NanNew<Integer>(rtn),
      NanNew<Integer>(serialno),
      NanNew<Integer>(packets)
    };

    callback->Call(3, argv);
  }
 private:
  ogg_sync_state *oy;
  ogg_page *page;
  int serialno;
  int packets;
  int rtn;
};

NAN_METHOD(node_ogg_sync_pageout) {
  NanScope();

  ogg_sync_state *oy = reinterpret_cast<ogg_sync_state *>(UnwrapPointer(args[0]));
  ogg_page *page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(new OggSyncPageoutWorker(oy, page, callback));
  NanReturnUndefined();
}

NAN_METHOD(node_ogg_stream_init) {
  NanScope();
  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  int serialno = static_cast<int>(args[1]->IntegerValue());
  NanReturnValue(NanNew<Integer>(ogg_stream_init(os, serialno)));
}


/* Writes a `ogg_page` struct into a `ogg_stream_state`. */
class OggStreamPageinWorker : public NanAsyncWorker {
 public:
  OggStreamPageinWorker(ogg_stream_state *os, ogg_page *page, NanCallback *callback)
    : os(os), page(page), rtn(0), NanAsyncWorker(callback) { }
  void Execute () {
    rtn = ogg_stream_pagein(os, page);
  }
  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[1] = { NanNew<Integer>(rtn) };

    callback->Call(1, argv);
  }
 private:
  ogg_stream_state *os;
  ogg_page *page;
  int rtn;
};

NAN_METHOD(node_ogg_stream_pagein) {
  NanScope();

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  ogg_page *page = reinterpret_cast<ogg_page *>(UnwrapPointer(args[1]));
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(new OggStreamPageinWorker(os, page, callback));
  NanReturnUndefined();
}


/* Reads a `ogg_packet` struct from a `ogg_stream_state`. */
class OggStreamPacketoutWorker : public NanAsyncWorker {
 public:
  OggStreamPacketoutWorker (ogg_stream_state *os, ogg_packet *packet, NanCallback *callback)
    : os(os), packet(packet), rtn(0), NanAsyncWorker(callback) { }
  ~OggStreamPacketoutWorker () { }
  void Execute () {
    rtn = ogg_stream_packetout(os, packet);
  }
  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[6];
    argv[0] = NanNew<Integer>(rtn);
    if (rtn == 1) {
      argv[1] = NanNew<Number>(packet->bytes);
      argv[2] = NanNew<Number>(packet->b_o_s);
      argv[3] = NanNew<Number>(packet->e_o_s);
      argv[4] = NanNew<Number>(static_cast<double>(packet->granulepos));
      argv[5] = NanNew<Number>(static_cast<double>(packet->packetno));
    } else {
      argv[1] = NanNull();
      argv[2] = NanNull();
      argv[3] = NanNull();
      argv[4] = NanNull();
      argv[5] = NanNull();
    }

    callback->Call(6, argv);
  }
 private:
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
};

NAN_METHOD(node_ogg_stream_packetout) {
  NanScope();

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(new OggStreamPacketoutWorker(os, packet, callback));
  NanReturnUndefined();
}


/* Writes a `ogg_packet` struct to a `ogg_stream_state`. */
class OggStreamPacketinWorker : public NanAsyncWorker {
 public:
  OggStreamPacketinWorker (ogg_stream_state *os, ogg_packet *packet, NanCallback *callback)
    : os(os), packet(packet), rtn(0), NanAsyncWorker(callback) { }
  ~OggStreamPacketinWorker () { }
  void Execute () {
    rtn = ogg_stream_packetin(os, packet);
  }
  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[1] = { NanNew<Integer>(rtn) };

    callback->Call(1, argv);
  }
 private:
  ogg_stream_state *os;
  ogg_packet *packet;
  int rtn;
};

NAN_METHOD(node_ogg_stream_packetin) {
  NanScope();

  ogg_stream_state *os = reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0]));
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[1]));
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(new OggStreamPacketinWorker(os, packet, callback));
  NanReturnUndefined();
}

// Since both StreamPageout and StreamFlush have the same HandleOKCallback,
// this base class deals with both.
class StreamWorker : public NanAsyncWorker {
 public:
  StreamWorker(ogg_stream_state *os, ogg_page *page, NanCallback *callback)
      : NanAsyncWorker(callback), os(os), page(page), rtn(0) { }
  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[4];
    argv[0] = NanNew<Integer>(rtn);
    if (rtn == 0) {
      /* need more data */
      argv[1] = NanNull();
      argv[2] = NanNull();
      argv[3] = NanNull();
    } else {
      /* got a page! */
      argv[1] = NanNew<Number>(page->header_len);
      argv[2] = NanNew<Number>(page->body_len);
      argv[3] = NanNew<Integer>(ogg_page_eos(page));
    }

    callback->Call(4, argv);
  }
 protected:
  ogg_stream_state *os;
  ogg_page *page;
  int rtn;
};

class StreamPageoutWorker : public StreamWorker {
 public:
  StreamPageoutWorker(ogg_stream_state *os, ogg_page *page, NanCallback *callback)
    : StreamWorker(os, page, callback) { }
  ~StreamPageoutWorker() { }
  void Execute () {
    rtn = ogg_stream_pageout(os, page);
  }
};

/* Reads out a `ogg_page` struct from an `ogg_stream_state`. */
NAN_METHOD(node_ogg_stream_pageout) {
  NanScope();
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(
    new StreamPageoutWorker(
      reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0])),
      reinterpret_cast<ogg_page *>(UnwrapPointer(args[1])),
      callback));
  NanReturnUndefined();
}

class StreamFlushWorker : public StreamWorker {
 public:
  StreamFlushWorker(ogg_stream_state *os, ogg_page *page, NanCallback *callback)
       : StreamWorker(os, page, callback) { }
  ~StreamFlushWorker() { }
  void Execute () {
    rtn = ogg_stream_flush(os, page);
  }
};

/* Forces an `ogg_page` struct to be flushed from an `ogg_stream_state`. */
NAN_METHOD(node_ogg_stream_flush) {
  NanScope();
  NanCallback *callback = new NanCallback(args[2].As<Function>());

  NanAsyncQueueWorker(
    new StreamFlushWorker(
      reinterpret_cast<ogg_stream_state *>(UnwrapPointer(args[0])),
      reinterpret_cast<ogg_page *>(UnwrapPointer(args[1])),
      callback));
  NanReturnUndefined();
}

/* Converts an `ogg_page` instance to a node Buffer instance */
NAN_METHOD(node_ogg_page_to_buffer) {
  NanScope();

  ogg_page *op = reinterpret_cast<ogg_page *>(UnwrapPointer(args[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(args[1]));
  memcpy(buf, op->header, op->header_len);
  memcpy(buf + op->header_len, op->body, op->body_len);

  NanReturnUndefined();
}


/* packet->packet = ... */
NAN_METHOD(node_ogg_packet_set_packet) {
  NanScope();
  ogg_packet *packet = UnwrapPointer<ogg_packet *>(args[0]);
  packet->packet = UnwrapPointer<unsigned char *>(args[1]);
  NanReturnUndefined();
}


/* packet->packet */
NAN_METHOD(node_ogg_packet_get_packet) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(WrapPointer(packet->packet, packet->bytes));
}


/* packet->bytes */
NAN_METHOD(node_ogg_packet_bytes) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(NanNew<Number>(packet->bytes));
}


/* packet->b_o_s */
NAN_METHOD(node_ogg_packet_b_o_s) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(NanNew<Number>(packet->b_o_s));
}


/* packet->e_o_s */
NAN_METHOD(node_ogg_packet_e_o_s) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(NanNew<Number>(packet->e_o_s));
}


/* packet->granulepos */
NAN_METHOD(node_ogg_packet_granulepos) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(NanNew<Number>(static_cast<double>(packet->granulepos)));
}


/* packet->packetno */
NAN_METHOD(node_ogg_packet_packetno) {
  NanScope();
  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  NanReturnValue(NanNew<Number>(static_cast<double>(packet->packetno)));
}


/* Replaces the `ogg_packet` "packet" pointer with a Node.js buffer instance */
NAN_METHOD(node_ogg_packet_replace_buffer) {
  NanScope();

  ogg_packet *packet = reinterpret_cast<ogg_packet *>(UnwrapPointer(args[0]));
  unsigned char *buf = reinterpret_cast<unsigned char *>(UnwrapPointer(args[1]));
  memcpy(buf, packet->packet, packet->bytes);
  packet->packet = buf;

  NanReturnUndefined();
}


void Initialize(Handle<Object> exports) {
  NanScope();

  /* sizeof's */
#define SIZEOF(value) \
  exports->ForceSet(NanNew<String>("sizeof_" #value), NanNew<Integer>(static_cast<int32_t>(sizeof(value))), \
      static_cast<PropertyAttribute>(ReadOnly|DontDelete))
  SIZEOF(ogg_sync_state);
  SIZEOF(ogg_stream_state);
  SIZEOF(ogg_page);
  SIZEOF(ogg_packet);

  NODE_SET_METHOD(exports, "ogg_sync_init", node_ogg_sync_init);
  NODE_SET_METHOD(exports, "ogg_sync_write", node_ogg_sync_write);
  NODE_SET_METHOD(exports, "ogg_sync_pageout", node_ogg_sync_pageout);

  NODE_SET_METHOD(exports, "ogg_stream_init", node_ogg_stream_init);
  NODE_SET_METHOD(exports, "ogg_stream_pagein", node_ogg_stream_pagein);
  NODE_SET_METHOD(exports, "ogg_stream_packetout", node_ogg_stream_packetout);
  NODE_SET_METHOD(exports, "ogg_stream_packetin", node_ogg_stream_packetin);
  NODE_SET_METHOD(exports, "ogg_stream_pageout", node_ogg_stream_pageout);
  NODE_SET_METHOD(exports, "ogg_stream_flush", node_ogg_stream_flush);

  /* custom functions */
  NODE_SET_METHOD(exports, "ogg_page_to_buffer", node_ogg_page_to_buffer);

  NODE_SET_METHOD(exports, "ogg_packet_set_packet", node_ogg_packet_set_packet);
  NODE_SET_METHOD(exports, "ogg_packet_get_packet", node_ogg_packet_get_packet);
  NODE_SET_METHOD(exports, "ogg_packet_bytes", node_ogg_packet_bytes);
  NODE_SET_METHOD(exports, "ogg_packet_b_o_s", node_ogg_packet_b_o_s);
  NODE_SET_METHOD(exports, "ogg_packet_e_o_s", node_ogg_packet_e_o_s);
  NODE_SET_METHOD(exports, "ogg_packet_granulepos", node_ogg_packet_granulepos);
  NODE_SET_METHOD(exports, "ogg_packet_packetno", node_ogg_packet_packetno);
  exports->Set(NanNew<String>("ogg_packet_replace_buffer"),
    NanNew<FunctionTemplate>(node_ogg_packet_replace_buffer)->GetFunction());

}

} // nodeogg namespace

NODE_MODULE(ogg, nodeogg::Initialize);
