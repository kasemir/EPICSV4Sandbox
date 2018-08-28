EPICSV4Sandbox
==============

Explorational EPICS V4/7 tests, demos, trials, experiments

To compile, this needs to be checked out into a directory that has a file RELEASE.local:

    EPICSV4Sandbox/
    RELEASE.local

That RELEASE.local file needs to list paths to EPICS 7:

    # Example RELEASE.local file
    EPICS_BASE=/home/training/epics-train/tools/base-7.0.1.1

neutronsDemoServer
------------------
Generates data similar to the SNS neutron events as channel 'neutrons'.

Can be started as a standalone server:

    neutronServerMain -e 200000

Run with '-h' to see command-line arguments for delay between updates
and number of events within each update.

Can also run as an IOC:

    iocBoot/neutrons/st.cmd

Can be monitored via

    pvget -m -r "field()" neutrons
    
or

    neutronClientMain -m -q
    
If IOC includes pvaSrv, its V3 records can also be reached via pvAccess.
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

