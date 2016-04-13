#!/bin/bash

# unofficial bash strict mode
set -e;# exit if any command has non-zero exit status
set -u;# prevent reference to undefined variables
set -o pipefail;#  prevents errors in a pipeline from being masked

echoerror() {
    echo "ERROR: $@" 1>&2
}


which samtools >/dev/null
which datamash >/dev/null
which valgrind >/dev/null
majorv=$(set +e; samtools 2>&1 | grep ^Version | cut -f 2 -d ' ' | cut -f 1 -d .; set -e)
if [ $majorv == 0 ]; then
    echoerror "Need samtools version >= 1" 
    exit 1
fi

# binary
bin=./split_bam_by_chr 
test -x $bin || exit 1

successmsg="All tests successfully passed"
echo "Testing $bin"
echo "If all tests pass, exit status is 0 and the following message is printed \"$successmsg\""

# output directory
od=$(mktemp -d -t $(basename $0).XXXXXX)
log=$od/log
echo "Output directory is $od"
echo "Logging to $log"

# output prefix
op=$od/test

# input bam
ib=tests/test_input_1_a.bam
test -e $ib || exit 1
# number of input sequences
nireads=$(samtools view -c $ib 2>$log)
niseqs=$(samtools view -H $ib 2>$log | grep -c '^@SQ')
echo "Testing $ib with $nireads reads and $niseqs seqs"


# default run
#
cmd="$bin -o $op $ib"
echo "Running 'default': $cmd"
eval $cmd >/dev/null
obams=$(find $od -type f | grep -v 'log$')
#echo "DEBUG obams=$obams" 1>&2
echo "Checking whether output is all SAM"
echo $obams | xargs -n 1 file | sed '/empty$/d' | grep -q 'ASCII';# fails if not ASCII i.e. SAM
echo "Checking number of output reads"
noreads=$(echo $obams | xargs -n 1 samtools view -Sc 2>$log | datamash sum 1)
if [ $noreads != $nireads ]; then
    echoerror "Number of input reads ($nireads) and output reads ($noreads) differ"
    exit 1
fi
rm $obams


cmd="cat $ib | $bin -o $op -"
echo "Running 'streaming input': $cmd"
eval $cmd >/dev/null
obams=$(find $od -type f | grep -v 'log$')
#echo "DEBUG obams=$obams" 1>&2
echo "Checking whether output is all SAM"
echo $obams | xargs -n 1 file | sed '/empty$/d' | grep -q 'ASCII';# fails if not ASCII i.e. SAM
echo "Checking number of output reads"
noreads=$(echo $obams | xargs -n 1 samtools view -Sc 2>$log | datamash sum 1)
if [ $noreads != $nireads ]; then
    echoerror "Number of input reads ($nireads) and output reads ($noreads) differ"
    exit 1
fi
rm $obams


cmd="samtools view -h $ib | $bin -b -o $op -"
echo "Running 'streaming input as SAM and output as BAM': $cmd"
eval $cmd >/dev/null
obams=$(find $od -type f | grep -v 'log$')
#echo "DEBUG obams=$obams" 1>&2
echo "Checking whether output is all BAM"
echo $obams | xargs -n 1 file | sed '/empty$/d' | grep -q 'gzip compressed data, extra field';# fails if not ASCII i.e. SAM
echo "Checking number of output reads"
noreads=$(echo $obams | xargs -n 1 samtools view -Sc 2>$log | datamash sum 1)
if [ $noreads != $nireads ]; then
    echoerror "Number of input reads ($nireads) and output reads ($noreads) differ"
    exit 1
fi
rm $obams


# default run
#
cmd="$bin -o $op $ib"
echo "Running valgrind with 'default': $cmd"
vlog=$od/valgrind.log
eval valgrind --log-file=$vlog --tool=memcheck $cmd >/dev/null

echo "Checking for errors reported by valgrind"
num_err=$(set +e; grep 'ERROR SUMMARY' $vlog | grep -cv ': 0 errors'; set -e;)
if [ "$num_err" -ne 0 ]; then
    echoerror "Found errors in Valgrind output $vlog"
    exit 1
fi

echo "Checking for lost bytes"
lost_bytes=$(set +e; grep 'lost' $vlog | grep -cv ': 0 bytes in 0 blocks'; set -e;)
if [ "$lost_bytes" -ne 0 ]; then
    echoerror "Found lost bytes in Valgrind output $vlog" || exit 1
    exit 1
fi

echo
echo $successmsg
echo
rm -rf $od

