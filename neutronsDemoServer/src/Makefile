TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Library for IOC
INC += neutronServer.h
DBD += neutronServer.dbd
LIBRARY_IOC += neutronServer
neutronServer_SRCS += neutronServer.cpp
neutronServer_SRCS += workerRunnable.cpp
neutronServer_SRCS += neutronServerRegister.cpp

# Standalone demo server
PROD_HOST += neutronServerMain
neutronServerMain_SRCS += neutronServerMain.cpp
neutronServerMain_SRCS += neutronServer.cpp
neutronServerMain_SRCS += workerRunnable.cpp
neutronServerMain_LIBS += pvDatabase
neutronServerMain_LIBS += pvAccess
neutronServerMain_LIBS += pvData
neutronServerMain_LIBS += Com

# Standalone client that checks sequence of events from demo server
PROD_HOST += neutronClientMain
neutronClientMain_SRCS += neutronClientMain.cpp
neutronClientMain_LIBS += pvAccess
neutronClientMain_LIBS += pvData
neutronClientMain_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

