#!../../bin/linux-x86_64/neutrons

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/neutrons.dbd")
neutrons_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/neutrons.db","P=demo")

cd ${TOP}/iocBoot/${IOC}
iocInit()

dbl
epicsThreadSleep(1.0)

neutronServerCreateRecord("neutrons", 0.001, 10)

startPVAServer

pvdbl

