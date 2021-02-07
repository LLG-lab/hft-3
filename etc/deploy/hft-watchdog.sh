#!/bin/bash -e

declare -r ARG="$1"

if [ "x$ARG" = "x" ] ; then
    echo "Hft Watchdog - Copyright (c) 2017-2021 by LLG Ryszard Gradowski, All Rights Reserved"
    echo ""
    echo "Usage:"
    echo "  hft-watchdog enable|disable|status"
    echo ""
elif [ "x$ARG" = "xstatus" ] ; then
    S=$(< /var/lib/hft/hftwatchdog-run.status)
    echo "Hft Watchdog is [${S}]"
elif [ "x$ARG" = "xenable" ] ; then
    echo "enabled" > /var/lib/hft/hftwatchdog-run.status
elif [ "x$ARG" = "xdisable" ] ; then
    echo "disabled" > /var/lib/hft/hftwatchdog-run.status
else
    echo "ERROR: Unrecognized option ‘$ARG’"
    exit 1
fi
