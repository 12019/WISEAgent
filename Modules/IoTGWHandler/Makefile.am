



libIoTSensor_la_LIBADD = ../lib/libBasicFun_Tool.a

libIoTSensor_la_SOURCES = IoTGWHandler.c IoTGWHandler.h IoTGWFunction.c IoTGWFunction.h LoadSNManagerAPI.c ./Format/SnMJSONParser.c SnGwParser.c

libIoTSensor_la_LDFLAGS = -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Library/SUSIHelper -lsusihelper -L$(top_srcdir)/Library/SQFlashHelper -lsqflashhelper -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module

include_HEADERS = $(top_srcdir)/../inc/SensorNetwork_BaseDef.h \
									$(top_srcdir)/../inc/SensorNetwork_Manager_API.h


AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/../inc -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SUSIHelper -I$(top_srcdir)/Library/SQFlashHelper
lib_LTLIBRARIES = libIoTSensor.la
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)

                  
