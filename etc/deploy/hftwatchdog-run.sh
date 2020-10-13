#!/bin/bash -e

SWITCH_STATUS=$(< /var/lib/hft/hft-watchdog-switch)

if [ "x$SWITCH_STATUS" = "xenabled" ] ; then
    SERVER_RUNNING=$(ps aux | grep "hft server" | grep -v grep || true)
    if [ "x$SERVER_RUNNING" = "x" ] ; then
        # Server zdechł.
        SMSMSG=$(< /var/lib/hft/hft-watchdog-lastrun)
        SMSMSG="hft server is dead, last runned ${SMSMSG}"
        # Wysłanie sms-a.
        echo $SMSMSG
        /usr/bin/caans --client-message="${SMSMSG}" && echo "disabled" > /var/lib/hft/hft-watchdog-switch
     else
        # Serwer żyje, zapisz date ostatniego sprawdzenia.
        LAST_CHECK=$(date)
        echo $LAST_CHECK > /var/lib/hft/hft-watchdog-lastrun
     fi
else
    echo "`date` : Watchdog wyłączony, nie ma co sprawdzać"
fi
