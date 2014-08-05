TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += neutrons.dbd
neutrons_DBD += base.dbd
neutrons_DBD += PVAClientRegister.dbd
neutrons_DBD += PVAServerRegister.dbd
neutrons_DBD += registerChannelProviderLocal.dbd
neutrons_DBD += dbPv.dbd
neutrons_DBD += neutronServer.dbd

PROD_IOC += neutrons

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
neutrons_SRCS += neutrons_registerRecordDeviceDriver.cpp
neutrons_SRCS_DEFAULT += neutronsMain.cpp
neutrons_SRCS_vxWorks += -nil-

# The following adds support from base/src/vxWorks
neutrons_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

neutrons_LIBS += neutronServer
neutrons_LIBS += pvDatabase
neutrons_LIBS += pvaSrv
neutrons_LIBS += pvAccess
neutrons_LIBS += pvData
neutrons_LIBS += $(EPICS_BASE_IOC_LIBS)


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

