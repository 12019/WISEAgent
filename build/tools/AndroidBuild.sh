#!/bin/bash
#----------------------------------------------------------
# Arg1: BSP home
# Arg2: Target product, Ex: mio5250-eng
# Arg3: Source path
#----------------------------------------------------------

# Check number of parameters
[ "$#" -eq 3 ] || exit 2;

# cd to BSP root
cd $1

# Initialize build environment
source build/envsetup.sh
lunch $2

# Rebuild Android.mk by source path
mmm -B $3

