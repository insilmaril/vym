#!/bin/bash

SRCDIR=test
OLDDIR=$(pwd)

VYMTESTDIR=$(mktemp -d /tmp/vym-test-XXXX)

echo Created $VYMTESTDIR

vym  -l -t -n test  -geometry 768x576-0+0 &

PID=$!

cd $SRCDIR

vym-test-legacy.rb -d $VYMTESTDIR

kill -s 15 $PID
echo To clean up, do: rm -rf $VYMTESTDIR

cd $OLDDIR
