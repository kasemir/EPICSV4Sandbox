TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_HOST += neutronServerMain
neutronServerMain_SRCS += neutronServerMain.cpp
neutronServerMain_SRCS += neutronServer.cpp
neutronServerMain_LIBS += pvDatabase
neutronServerMain_LIBS += pvAccess
neutronServerMain_LIBS += pvData
neutronServerMain_LIBS += Com

#PROD_HOST += neutronClientMain
neutronClientMain_SRCS += neutronClientMain.cpp
neutronClientMain_LIBS += pvAccess
neutronClientMain_LIBS += pvData

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

