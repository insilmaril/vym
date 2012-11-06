#!/bin/bash

TESTDIR=test
DEFAULTMAP=$TESTDIR/default.vym
TESTMAP=$TESTDIR/testmap.vym

cp $DEFAULTMAP $TESTMAP

vym  -l -t -n test $TESTMAP -geometry 768x576-0+0 &
PID=$!

sleep 1

test/vym-test.rb

kill $PID
rm $TESTMAP
