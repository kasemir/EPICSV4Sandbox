EPICSV4Sandbox
==============

Explorational EPICS V4 tests, demos, trials, experiments

To compile, this needs to be checked out into a directory parallel to the V4 CPP sources:

    EPICSV4Sandbox/
    pvCommonCPP/
    pvDataCPP/
    pvAccessCPP/
    pvaSrv/
    pvDatabaseCPP/

In that same directory, a file    
 
    RELEASE.local
    
which needs to list paths of the above directories:

    EPICS_BASE = ...
    PVCOMMON=...
    PVDATA=...
    PVACCESS=...
    PVASRV=...
    PVDATABASE=...


neutronsDemoServer
------------------
Generates data similar to the SNS neutron events as channel 'neutrons'.

Can be started as a standalone server:

    neutronServerMain -e 200000

Command-line arguments for delay between updates and number of events within each update.

Can also run as an IOC:

    iocBoot/neutrons/st.cmd

Can be monitored via

    pvget -m neutrons
    
or

    neutronClientMain -m -q

