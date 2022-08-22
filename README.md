EPICSV4Sandbox
==============

Explorational EPICS V4/7 tests, demos, trials, experiments

To compile, this needs to be checked out into a directory that has a file RELEASE.local:

    EPICSV4Sandbox/
    RELEASE.local

That RELEASE.local file needs to list paths to EPICS 7:

    # Example RELEASE.local file
    EPICS_BASE=/home/training/epics-train/tools/base-7.0.1.1
    
    # If you're using PVXS, that needs to defined as well
    PVXS=/path/to/pvxs

neutronsDemoServer
------------------

Generates data similar to the SNS neutron events as channel 'neutrons'.

Can be started as a standalone server:

    neutronServerMain -e 200000

Run with '-h' to see command-line arguments for delay between updates
and number of events within each update.


The neutrons demo server can be compiled against the older pvDatabaseCPP
library or the newer [PVXS](https://github.com/mdavidsaver/pvxs) library.
Check the Makefile for 'PVXS' and add/remove comments as necessary.
Note that when using PVXS, you may have to compile everything,
and that includes EPICS base, with `USR_CXXFLAGS = -std=c++11`.
There are several ways to accomplish that, one is setting it in
`base/configure/CONFIG_COMMON`.

If you're NOT using PVXS but pvDatabaseCPP, the code
can also run as an IOC:

    iocBoot/neutrons/st.cmd

Either way it be monitored via

    pvget -m -r "field()" neutrons
    
or

    neutronClientMain -m -q
    
If IOC includes pvaSrv, which it does by default for EPICS 7,
all V3 records can also be reached via pvAccess.
See srcIoc/src/neutronsInclude.dbd

ntndarrayServer
---------------
Creates animated image.
From V4 training material by David Hickin, https://github.com/dhickin/epicsv4Training.

Runs as standalone server:
    
    ntndarrayServerMain IMAGE

Then try:

    pvinfo IMAGE
    pvget -m -r dimension IMAGE
    pvget -m -r value IMAGE

Can be used as PV for the Display Builder Image widget.

