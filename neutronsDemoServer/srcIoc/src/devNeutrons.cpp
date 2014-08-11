/* devNeutrons.cpp
 *
 * Device support for EPICS V3 records that interface with a V4 record
 * @author Kay Kasemir
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "link.h"
#include "aoRecord.h"
#include "epicsExport.h"

#include <pv/pvDatabase.h>
#include <neutronServer.h>

using namespace std;
using namespace std::tr1;
using namespace epics::pvDatabase;
using namespace epics::neutronServer;

static NeutronPVRecord::shared_pointer neutrons_record;
static shared_ptr<FakeNeutronEventRunnable> fake_event_runnable;
static shared_ptr<epicsThread> fake_event_thread;

/** Initialize the PVA record:
 *  On pass 0, create it,
 *  on pass 1, start it.
 */
static long global_init(int pass)
{
    if (pass == 0)
    {
        string name("neutrons");
        cout << "Creating V4 '" << name << "' record" << endl;
        neutrons_record = NeutronPVRecord::create(name);
        if (! PVDatabase::getMaster()->addRecord(neutrons_record))
            cout << "Cannot create neutron record '" << neutrons_record->getRecordName() << "'" << endl;
        fake_event_runnable.reset(new FakeNeutronEventRunnable(neutrons_record, 1, 10));
    }
    else if (pass == 1)
    {
        cout << "Starting demo neutron event thread" << endl;
        fake_event_thread.reset(new epicsThread(*fake_event_runnable.get(), "FakeNeutrons", epicsThreadGetStackSize(epicsThreadStackMedium)));
        fake_event_thread->start();
    }
    return 0;
}

static long init_record(struct aoRecord	*rec)
{
    return 2; /* Don't convert */
}

static long write_delay(struct aoRecord *rec)
{
    fake_event_runnable->setDelay(rec->oval);
    return 0;
}

static long write_count(struct aoRecord *rec)
{
    fake_event_runnable->setCount((size_t) rec->rval);
    return 0;
}

extern "C" {

struct
{
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write;
    DEVSUPFUN   special_linconv;
} devAoDemoNeutronDelay =
{
    6,
    NULL,
    (DEVSUPFUN) global_init,
    (DEVSUPFUN) init_record,
    NULL,
    (DEVSUPFUN) write_delay,
    NULL
};
epicsExportAddress(dset, devAoDemoNeutronDelay);


struct
{
    long        number;
    DEVSUPFUN   report;
    DEVSUPFUN   init;
    DEVSUPFUN   init_record;
    DEVSUPFUN   get_ioint_info;
    DEVSUPFUN   write;
    DEVSUPFUN   special_linconv;
} devAoDemoNeutronCount =
{
    6,
    NULL,
    NULL,
    (DEVSUPFUN) init_record,
    NULL,
    (DEVSUPFUN) write_count,
    NULL
};
epicsExportAddress(dset, devAoDemoNeutronCount);

} // "C"
