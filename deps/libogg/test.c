#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ogg/ogg.h"

struct stream {
  ogg_stream_state os;
  int serialno;
  struct stream *next;
};

int main () {
  ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
  ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  struct stream *st = NULL;

  int r;
  char *buffer;
  int  bytes;
  int eos = 0;
  int i;
  int bufferSize = 4096;

  r = ogg_sync_init(&oy); /* Now we can read pages */
  assert(r == 0);

  while (1) { /* we repeat if the bitstream is chained */

    /* submit a 4k block to the libogg sync layer */
    buffer = ogg_sync_buffer(&oy, bufferSize);
    bytes = fread(buffer, 1, bufferSize, stdin);
    if (bytes == 0) {
      fprintf(stderr, "Got stdin EOF.\n");
      r = 0; break;
    }

    r = ogg_sync_wrote(&oy, bytes);
    assert(r == 0);

    /* Get the first page. */
    r = ogg_sync_pageout(&oy, &og);
    if (r == 0) {
      fprintf(stderr, "Need more data.\n");
      continue;
    } else if (r < 1) {
      /* have we simply run out of data?  If so, we're done. */
      if (bytes < bufferSize) break;

      /* error case.  Must not be Vorbis data */
      fprintf(stderr, "Input does not appear to be an Ogg bitstream.\n");
      r = 1; break;
    }
    assert(r == 1);

    /* Get the serial number and set up the rest of decode. */
    /* serialno first; use it to set up a logical stream */
    int serialno = ogg_page_serialno(&og);
    int packets = ogg_page_packets(&og);
    fprintf(stderr, "version: %d\n", ogg_page_version(&og));
    fprintf(stderr, "continued: %d\n", ogg_page_continued(&og));
    fprintf(stderr, "pageno: %ld\n", ogg_page_pageno(&og));
    fprintf(stderr, "serialno: %d\n", serialno);
    fprintf(stderr, "packets: %d\n", packets);
    fprintf(stderr, "granulepos: %lld\n", ogg_page_granulepos(&og));
    fprintf(stderr, "eos: %d\n", ogg_page_eos(&og));
    fprintf(stderr, "bos: %d\n", ogg_page_bos(&og));


    /* we need to get the correct "ogg_stream_state" struct based on the serialno */
    struct stream *s = st;
    while (1) {
      if (s == NULL) {
        fprintf(stderr, "creating struct stream for %d\n", serialno);
        s = malloc(sizeof(struct stream));
        s->next = NULL;
        s->serialno = serialno;
        ogg_stream_init(&s->os, serialno);
        if (st == NULL) {
          st = s;
        } else {
          /* have to set "s" to the last element of the "st" linked list */
          struct stream *t = st;
          while (1) {
            if (t->next == NULL) {
              t->next = s;
              break;
            } else {
              t = t->next;
            }
          }
        }
        break;
      }
      if (s->serialno == serialno) {
        break;
      }
      s = s->next;
    }
    assert(s->serialno == serialno);
    fprintf(stderr, "using struct stream %d\n", s->serialno);
    /* holy shit that sucked... */


    if (ogg_stream_pagein(&s->os, &og) < 0) {
      /* error; stream version mismatch perhaps */
      fprintf(stderr, "Error reading page of Ogg bitstream data.\n");
      r = 1; break;
    }

    /* iterate though the "packets" in the page */
    for (i=0; i<packets; i++) {
      fprintf(stderr, "  Reading packet %d.\n", i);
      r = ogg_stream_packetout(&s->os, &op);
      fprintf(stderr, "  Reading packet result %d.\n", r);
      if (r != 1) {
        /* no page? must not be vorbis */
        fprintf(stderr, "  Error reading packet.\n");
        r = 1; break;
      }

      /**
       * At this point, you'd pass the raw packet data to the vorbis decoder or
       * whatever your destination is....
       */
      fprintf(stderr, "    bytes: %ld\n", op.bytes);
      fprintf(stderr, "    b_o_s: %ld\n", op.b_o_s);
      fprintf(stderr, "    e_o_s: %ld\n", op.e_o_s);
      fprintf(stderr, "    granulepos: %lld\n", op.granulepos);
      fprintf(stderr, "    packetno: %lld\n", op.packetno);
    }

    fprintf(stderr, "\n");
  }

  /* OK, clean up the framer */
  ogg_sync_clear(&oy);

  fprintf(stderr,"\nDone.\n");
  return r;
}
