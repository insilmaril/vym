#!/bin/bash

SRCDIR=test
VYMTESTDIR=$(mktemp -d /tmp/vym-test-XXXX)

#echo Created $VYMTESTDIR

# Copy test data to temporary test directory
cp $SRCDIR/*.vym $VYMTESTDIR
cp $SRCDIR/*.xml $VYMTESTDIR

vym  -l -t -n test  -geometry 768x576-0+0 &

PID=$!

$SRCDIR/vym-test.rb -d $VYMTESTDIR

kill -s 15 $PID
echo To clean up, do: rm -rf $VYMTESTDIR
