#!/bin/bash

SRCDIR=test
VYMTESTDIR=$(mktemp -d /tmp/vym-test-XXXX)

DEFAULTMAP=$SRCDIR/default.vym
TESTMAP=$VYMTESTDIR/testmap.vym

cp $DEFAULTMAP $TESTMAP

vym  -l -t -n test  -geometry 768x576-0+0 &

PID=$!

$SRCDIR/vym-test.rb -d $VYMTESTDIR $TESTMAP

kill -s 15 $PID
echo To clean up, do: rm -rf $VYMTESTDIR
