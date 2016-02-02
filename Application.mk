APP_BUILD_SCRIPT := ./Android.mk 

#===============================================================#
#							ABIs								#
#---------------------------------------------------------------#
#	armeabi 			No hard float.
#	armeabi-v7a			Hard float supported.
#	arm64-v8a
#	x86					No support for MOVBE or SSE4.
#	x86_64
#	mips
#	mips64
#---------------------------------------------------------------#
APP_ABI := x86

#================================================================#
#  NDK-supported API levels and corresponding Android releases   #
#================================================================#
#  NDK-supported API level  |    Android release                |
#----------------------------------------------------------------#
#|      android-3           |        1.5                        |
#|      android-4           |        1.6                        |
#|      android-5           |        2.0                        |
#|      android-8           |        2.2                        |
#|      android-9           |        2.3 through 3.0.x          |
#|      android-12          |        3.1.x                      |
#|      android-13          |        3.2                        |
#|      android-14          |        4.0 through 4.0.2          |
#|      android-15          |        4.0.3 and 4.0.4            |
#|      android-16          |        4.1 and 4.1.1              |
#|      android-17          |        4.2 and 4.2.2              |
#|      android-18          |        4.3                        |
#|      android-19          |        4.4                        |
#|      android-21          |        4.4W and 5.0               |
#----------------------------------------------------------------#
APP_PLATFORM := android-19

APP_CFLAGS += -DANDROID 
APP_CFLAGS += -Wno-error=format-security 

APP_STL := gnustl_static

