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

# Uncomment next two lines to build against PVXS
#USR_CXXFLAGS += -DUSE_PVX
#neutronServerMain_LIBS += pvxs
# pvxs headers require this C++11 setting.
# If so, note that EPICS base (pvData etc.)
# also need to be compiled with the same C++11 setting!
USR_CXXFLAGS += -std=c++11

# Standalone client that checks sequence of events from demo server
PROD_HOST += neutronClientMain
neutronClientMain_SRCS += neutronClientMain.cpp
neutronClientMain_LIBS += pvAccess
neutronClientMain_LIBS += pvData
neutronClientMain_LIBS += Com

# Uncomment next three lines to build against PVXS
#USR_CXXFLAGS += -DUSE_PVXS
#neutronServerMain_LIBS += pvxs
#neutronClientMain_LIBS += pvxs

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

