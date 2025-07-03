#!/bin/bash

SRCDIR=$PWD
VYMTESTDIR=$(mktemp -d /tmp/vym-test-XXXX)

DEFAULTMAP=$SRCDIR/test/maps/test-default.vym
TESTMAP=$VYMTESTDIR/testmap.vym

echo Copying $DEFAULTMAP to $TESTMAP
cp $DEFAULTMAP $TESTMAP

echo "Copy ok. $PWD"
vym  -l -t -n test -R test/vym-selftest.vys $TESTMAP #-geometry 768x576-0+0 &
#vym  -l -t -n test -R test/vym-selftest.vys $TESTMAP -geometry 768x576-0+0 &

#PID=$!

#$SRCDIR/vym-test.rb -d $VYMTESTDIR $TESTMAP

#kill -s 15 $PID
echo To clean up, do: rm -rf $VYMTESTDIR
