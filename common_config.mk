PROJECT_ROOT_DIR := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
PLATFORMS_DIR := $(PROJECT_ROOT_DIR)/Platform
PLATFORM_LINUX_DIR := $(PLATFORMS_DIR)/Linux
LIB_DIR := $(PROJECT_ROOT_DIR)/Library
INCLUDE_DIR := $(PROJECT_ROOT_DIR)/Include
INSTALL_OUTPUT_DIR := $(PROJECT_ROOT_DIR)/Release/AgentService
LIB_3RD_DIR := $(PROJECT_ROOT_DIR)/Library3rdParty
SUSI_DIR := $(LIB_3RD_DIR)/SUSI4/include
SQFLASH_DIR := $(LIB_3RD_DIR)/SQFlash/include

application_DIR := Application/CAgent Application/ScreenshotHelperL \
		   Application/AgentEncrypt \
		   Application/WatchdogL 
		   
shared_lib_NAME := libSAConfig libSAGatherInfo libSAClient libSAManager libSAHandlerLoader libSAGeneralHandler libmqtthelper

program_3rdPARTY_LIBRARIES := mosquitto xml2 curl pthread

application_LIBSDIR := $(LIB_DIR)/Log \
		       $(LIB_DIR)/MD5 \
		       $(LIB_DIR)/SQFlashHelper \
                       $(LIB_DIR)/cJSON \
                       $(LIB_DIR)/SUSIHelper \
                       $(LIB_DIR)/AdvCareHelper \
                       $(LIB_3RD_DIR)/Smart \
                       $(LIB_DIR)/FtpHelper \
                       $(LIB_DIR)/Base64 \
                       $(LIB_DIR)/DES \
		       		   $(LIB_DIR)/MQTTHelper \
                       $(LIB_DIR)/SAClient \
                       $(LIB_DIR)/SAConfig \
                       $(LIB_DIR)/SAGatherInfo \
                       $(LIB_DIR)/SAManager \
                       $(LIB_DIR)/SAHandlerLoader \
                       $(LIB_DIR)/SAGeneralHandler \
                       $(LIB_DIR)/SQLite
                 
MODULE_DIR := $(PROJECT_ROOT_DIR)/Modules
module_LIBDIR := $(MODULE_DIR)/MonitoringHandler \
                 $(MODULE_DIR)/ProcessMonitorHandler \
                 $(MODULE_DIR)/SoftwareMonitorHandler \
                 $(MODULE_DIR)/PowerOnOffHandler \
                 $(MODULE_DIR)/RemoteKVMHandler \
                 $(MODULE_DIR)/TerminalHandler \
                 $(MODULE_DIR)/ScreenshotHandler \
                 $(MODULE_DIR)/NetMonitorHandler \
		 $(MODULE_DIR)/HDDHandler \
		 $(MODULE_DIR)/RecoveryHandler \
		 $(MODULE_DIR)/ProtectHandler \
                 $(MODULE_DIR)/SUSIControlHandler \
                 $(MODULE_DIR)/ServerRedundancyHandler \
		 $(MODULE_DIR)/IoTGWHandler
                 
shared_LIBDIR := $(LIB_3RD_DIR)/Smart


