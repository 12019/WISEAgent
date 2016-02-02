APP_BUILD_SCRIPT := ./Android.mk 

APP_ABI := x86
APP_PLATFORM := android-19

APP_CFLAGS += -DANDROID 
APP_CFLAGS += -Wno-error=format-security 

APP_STL := gnustl_static

