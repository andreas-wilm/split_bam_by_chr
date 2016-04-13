 # user args
CC?=gcc
HTSDIR?=/mnt/projects/rpd/apps.testing/htslib-1.3/;
#HTSDIR?=/Users/wilma/local/dotkit-inst/htslib-1.3/

CFLAGS = -Wall -O3 $(IFLAGS) -I $(HTSDIR)/include/
LFLAGS_STATIC = -lz -pthread
LFLAGS_DYN = -lz -pthread -lhts -L$(HTSDIR)/lib

static: split_bam_by_chr.c $(HTSDIR)/lib/libhts.a
	$(CC) $(CFLAGS) split_bam_by_chr.c -o split_bam_by_chr $($IFLAGS) $(HTSDIR)/lib/libhts.a $(LFLAGS_STATIC)

dynamic: split_bam_by_chr.c
	$(CC) $(CFLAGS) split_bam_by_chr.c -o split_bam_by_chr $($IFLAGS) $(LFLAGS_DYN)

all: static
