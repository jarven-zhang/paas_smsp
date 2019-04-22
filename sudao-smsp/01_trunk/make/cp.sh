#!/bin/sh

DEST_LIB="../build/smsp_plugin/lib/libXHR.so"
#DEST_LIB="../build/smsp_plugin/lib/libTX.so"
cp $DEST_LIB /data/paas/smsp/smsp_report/lib/
cp $DEST_LIB /data/paas/smsp/smsp_send/lib/
