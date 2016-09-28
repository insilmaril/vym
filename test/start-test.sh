#!/bin/bash

SRCDIR=test
VYMTESTDIR=$(mktemp -d /tmp/vym-test-XXXX)

DEFAULTMAP=$SRCDIR/default.vym
TESTMAP=$VYMTESTDIR/testmap.vym

cp $DEFAULTMAP $TESTMAP

vym  -l -t -n test $TESTMAP  & #-geometry 768x576-0+0 &

PID=$!

sleep 1

$SRCDIR/vym-test.rb -d $VYMTESTDIR

kill -s 15 $PID
echo To clean up, do: rm -rf $VYMTESTDIR
