Split BAM by Chromosome
=======================

Splits your BAM/SAM into one file per SQ, plus one for unaligned reads.

Similar to bamUtils splitBam
(http://genome.sph.umich.edu/wiki/BamUtil:_splitBam) but no read group
dependency

Main purpose is to take aligner output and split it into different
files, which can be processed in parallel by tools that work on the
same reference.

# Todo

- Implement bed input

# Compilation

- You will have to set HTSDIR. 
- Default target is 'static'. The other valid target is 'dynamic'
- Example: `HTSDIR=/opt/local/htslib-1.3/ make dynamic`
