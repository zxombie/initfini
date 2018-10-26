#!/bin/sh

BASEDIR=`dirname $0`

export LD_LIBRARY_PATH=${BASEDIR}
${BASEDIR}/test/libinitfini_test
