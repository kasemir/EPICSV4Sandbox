TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += neutrons.dbd

PROD_IOC += neutrons

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
neutrons_SRCS += devNeutrons.cpp
neutrons_SRCS += neutrons_registerRecordDeviceDriver.cpp
neutrons_SRCS_DEFAULT += neutronsMain.cpp
neutrons_SRCS_vxWorks += -nil-

# The following adds support from base/src/vxWorks
neutrons_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

neutrons_LIBS += neutronServer
neutrons_LIBS += pvDatabase

neutrons_LIBS += qsrv

neutrons_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
neutrons_LIBS += $(EPICS_BASE_IOC_LIBS)


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

