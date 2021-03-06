#!/bin/sh

# defines
CONF_FILE=/etc/zabbix/externalscripts/zbx_chk_gfarm.conf

# Chekc Config File
if [ -f $CONF_FILE ];
    then
    . $CONF_FILE
else
    echo  -1;
    exit 0;
fi

# exec check command
if [ X"$MY_HOST" = X ]; then
    MY_HOST=`hostname`
fi

if [ -f $MDS_LIST_PATH ]; then
    CHK=`grep '^+ master' $MDS_LIST_PATH | awk '{ print $6 }' | grep $MY_HOST`
    if [ X$CHK != X ]; then
       RESULT="master"
    else
       RESULT="slave"
    fi
else
    RESULT="error: $MDS_LIST_PATH not found."
fi

echo $RESULT
exit 0
