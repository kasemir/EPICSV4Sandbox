EPICSV4Sandbox
==============

Explorational EPICS V4 tests, demos, trials, experiments


neutronsDemoServer
------------------
Generates data similar to the SNS neutron events as channel 'neutrons'.

Can be monitored via

    pvget -m neutrons

Command-line arguments for delay between updates and number of events within each update.