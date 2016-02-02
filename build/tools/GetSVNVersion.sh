#!/bin/bash
#----------------------------------------------------------
# Arg1: SUSI4 SourceCode Path
#----------------------------------------------------------

HOME=${0%/*}
REVISION=`svnversion $1`

# Remove M/S/P characters
REVISION=`echo $REVISION | sed -e "s/M//g"`
REVISION=`echo $REVISION | sed -e "s/S//g"`
REVISION=`echo $REVISION | sed -e "s/P//g"`

# Parse a mixed-revision, Ex: 4123:4168
REVISION=${REVISION##*:}

# Save revision to svnrevision.txt
echo ${REVISION}
