/*  
Andreas Wilm <wilma@gis.a-star.edu.sg>
MIT license

Based on htslib test/test_view.c.
(cram removed, because of compilation problems with htslib-1.3)

Original copyright:
    Copyright (C) 2012 Broad Institute.
    Copyright (C) 2013-2014 Genome Research Ltd.
    Author: Heng Li <lh3@sanger.ac.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "htslib/sam.h"

#define MYNAME "split_bam_by_chr"

void usage()
{
  fprintf(stderr, "Usage: %s [-bSI] [-N num_reads] [-l level] [-Z hdr_nuls] -o outprefix <in.[bam|sam]>\n", MYNAME);
  fprintf(stderr, "Options: \n");
  fprintf(stderr, "  -b : output is BAM\n");
  fprintf(stderr, "  -S : input is SAM\n");
  fprintf(stderr, "  -I : ignore SAM error\n");
  fprintf(stderr, "  -N : limit to this many reads\n");
  fprintf(stderr, "  -l : BAM compression level (0-9) (forces BAM output)\n");
  fprintf(stderr, "  -Z : extra header text\n");
  fprintf(stderr, "  -o : output prefix (final: prefix.no.bam)\n");/* FIXME which doesn't make sense if SAM */
}


int main(int argc, char *argv[])
{
    int i=0;
    samFile *in;
    int flag = 0, c, clevel = -1, ignore_sam_err = 0;
    char moder[8];
    bam_hdr_t *h;
    bam1_t *b;
    htsFile **out;
    char *outprefix = 0;
    char modew[800];
    int r = 0, exit_code = 0;
    int nreads = 0;
    int extra_hdr_nuls = 0;

    while ((c = getopt(argc, argv, "bSIN:l:Z:o:")) >= 0) {
      switch (c) {
        case 'b': flag |= 2; break;/* bam out */
        case 'S': flag |= 1; break;/* sam in */
        case 'I': ignore_sam_err = 1; break;
        case 'N': nreads = atoi(optarg); break;
        case 'l': clevel = atoi(optarg); flag |= 2; break;/* compression level */
        case 'Z': extra_hdr_nuls = atoi(optarg); break;
        case 'o': outprefix = strdup(optarg); break;
        }
    }
    if (argc == optind) {
      usage();
      return 1;
    }

    if (! outprefix) {
      fprintf(stderr, "Need output prefix\n");
        return EXIT_FAILURE;
    }

    /* setting up input
     */
    strcpy(moder, "r");
    if (flag&4) strcat(moder, "c");
    else if ((flag&1) == 0) strcat(moder, "b");

    in = sam_open(argv[optind], moder);
    if (in == NULL) {
        fprintf(stderr, "Error opening \"%s\"\n", argv[optind]);
        return EXIT_FAILURE;
    }
    h = sam_hdr_read(in);
    if (h == NULL) {
        fprintf(stderr, "Couldn't read header for \"%s\"\n", argv[optind]);
        return EXIT_FAILURE;
    }
    h->ignore_sam_err = ignore_sam_err;
    if (extra_hdr_nuls) {
        char *new_text = realloc(h->text, h->l_text + extra_hdr_nuls);
        if (new_text == NULL) {
            fprintf(stderr, "Error reallocing header text\n");
            return EXIT_FAILURE;
        }
        h->text = new_text;
        memset(&h->text[h->l_text], 0, extra_hdr_nuls);
        h->l_text += extra_hdr_nuls;
    }
    b = bam_init1();

    /* setting up output
     */
    strcpy(modew, "w");
    if (clevel >= 0 && clevel <= 9) sprintf(modew + 1, "%d", clevel);
    if (flag&8) strcat(modew, "c");
    else if (flag&2) strcat(modew, "b");
    out = calloc(h->n_targets+1, sizeof(htsFile*));/* +1 for unaligned reads */
    for (i=0; i<h->n_targets+1; i++) {
      char *buffer;
      /* FIXME ugly output filename construction. also, allocated outside */
      /* 6: max int 999999, 5: ".bam.", 1: \0 */
      if (i>=1000000) {exit(2);}      
      buffer = malloc((strlen(outprefix) + 6 + 5 + 1) * sizeof(char));
      sprintf(buffer, "%s.%d.bam", outprefix, i);
      out[i] = hts_open(buffer, modew);
      if (out[i] == NULL) {
        fprintf(stderr, "Error opening %s\n", buffer);
        return EXIT_FAILURE;
      }
      free(buffer);
    }


    /* writing 
     */
    for (i=0; i<h->n_targets+1; i++) {
      sam_hdr_write(out[i], h);
    }
    while ((r = sam_read1(in, h, b)) >= 0) {
      /* if unaligned tid is -1. so use +1. then unaligned go to 0 */
        if (sam_write1(out[b->core.tid+1], h, b) < 0) {
            fprintf(stderr, "Error writing output.\n");
            exit_code = 1;
            break;
        }
        if (nreads && --nreads == 0)
            break;
    }
    if (r < -1) {
        fprintf(stderr, "Error parsing input.\n");
        exit_code = 1;
    }

    for (i=0; i<h->n_targets+1; i++) {
      r = sam_close(out[i]);
      if (r < 0) {
        fprintf(stderr, "Error closing output.\n");
        exit_code = 1;
      }
    }

    bam_destroy1(b);
    bam_hdr_destroy(h);

    r = sam_close(in);
    if (r < 0) {
        fprintf(stderr, "Error closing input.\n");
        exit_code = 1;
    }

    free(out);
    free(outprefix);

    return exit_code;
}
