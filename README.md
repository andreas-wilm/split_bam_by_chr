split_bam_by_chr
================

Similar to bamUtils splitBam
(http://genome.sph.umich.edu/wiki/BamUtil:_splitBam) but no read group
dependency

Main purpose is to take aligner output and split it into different
files, which can be processed in parallel by tools that work on the
same reference.

* Todo
- Implement bed input

