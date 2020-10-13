#!/bin/bash -e

declare -r SWITCH_STATUS=$(< /var/lib/hft/hftwatchdog-run.status)

if [ "x$SWITCH_STATUS" = "xenabled" ] ; then
    SERVER_RUNNING=$(ps aux | grep "hft server" | grep -v grep || true)
    if [ "x$SERVER_RUNNING" = "x" ] ; then
        # Server is dead.
        SMSMSG=$(< /var/lib/hft/hft-watchdog-lastrun)
        SMSMSG="hft server is dead, last runned ${SMSMSG}"
        # Sending SMS message..
        echo $SMSMSG
        /usr/bin/caans --client-message="${SMSMSG}" && echo "disabled" > /var/lib/hft/hft-watchdog-switch
     else
        # Server is alive, save last check time stamp.
        LAST_CHECK=$(date)
        echo $LAST_CHECK > /var/lib/hft/hft-watchdog-lastrun
     fi
else
    echo "`date` : Watchdog disabled, nothing to do."
fi
