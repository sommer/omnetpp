//=========================================================================
//  CSCHEDULER.CC - part of
//
//                  OMNeT++/OMNEST
//           Discrete System Simulation in C++
//
//  Author: Andras Varga, 2003
//
//=========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 2003-2008 Andras Varga
  Copyright (C) 2006-2008 OpenSim Ltd.
  Monash University, Dept. of Electrical and Computer Systems Eng.
  Melbourne, Australia

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include "cscheduler.h"
#include "cevent.h"
#include "csimulation.h"
#include "cmessageheap.h"
#include "globals.h"
#include "cenvir.h"
#include "cconfiguration.h"
#include "cconfigoption.h"
#include "platmisc.h" // usleep

USING_NAMESPACE

Register_GlobalConfigOption(CFGID_REALTIMESCHEDULER_SCALING, "realtimescheduler-scaling", CFG_DOUBLE, NULL, "When cRealTimeScheduler is selected as scheduler class: ratio of simulation time to real time. For example, scaling=2 will cause simulation time to progress twice as fast as runtime.");

cScheduler::cScheduler()
{
    sim = NULL;
}

cScheduler::~cScheduler()
{
}

void cScheduler::setSimulation(cSimulation *_sim)
{
    sim = _sim;
}

void cScheduler::lifetimeEvent(SimulationLifetimeEventType eventType, cObject *details)
{
    switch (eventType) {
        case LF_PRE_NETWORK_INITIALIZE: startRun(); break;
        case LF_ON_RUN_END: endRun(); break;
        case LF_ON_SIMULATION_RESUME: executionResumed(); break;
    }
}

//-----

Register_Class(cSequentialScheduler);

cEvent *cSequentialScheduler::guessNextEvent()
{
    return sim->msgQueue.peekFirst();
}

cEvent *cSequentialScheduler::takeNextEvent()
{
    for (;;)
    {
        cEvent *event = sim->msgQueue.removeFirst();
        if (!event)
            throw cTerminationException(eENDEDOK);
        if (event->isStale())
            delete event;
        else
            return event;
    }
}

//-----

Register_Class(cRealTimeScheduler);

cRealTimeScheduler::cRealTimeScheduler() : cScheduler()
{
}

cRealTimeScheduler::~cRealTimeScheduler()
{
}

void cRealTimeScheduler::startRun()
{
    factor = ev.getConfig()->getAsDouble(CFGID_REALTIMESCHEDULER_SCALING);
    if (factor!=0)
        factor = 1/factor;
    doScaling = (factor!=0);

    gettimeofday(&baseTime, NULL);
}

void cRealTimeScheduler::executionResumed()
{
    gettimeofday(&baseTime, NULL);
    baseTime = timeval_substract(baseTime, SIMTIME_DBL(doScaling ? factor*sim->getSimTime() : sim->getSimTime()));
}

bool cRealTimeScheduler::waitUntil(const timeval& targetTime)
{
    // if there's more than 200ms to wait, wait in 100ms chunks
    // in order to keep UI responsiveness by invoking ev.idle()
    timeval curTime;
    gettimeofday(&curTime, NULL);
    while (targetTime.tv_sec-curTime.tv_sec >=2 ||
           timeval_diff_usec(targetTime, curTime) >= 200000)
    {
        usleep(100000); // 100ms
        if (ev.idle())
            return false;
        gettimeofday(&curTime, NULL);
    }

    // difference is now at most 100ms, do it at once
    long usec = timeval_diff_usec(targetTime, curTime);
    if (usec>0)
        usleep(usec);
    return true;
}

cEvent *cRealTimeScheduler::guessNextEvent()
{
    return sim->msgQueue.peekFirst();
}

cEvent *cRealTimeScheduler::takeNextEvent()
{
    cEvent *event = sim->msgQueue.peekFirst();
    if (!event)
        throw cTerminationException(eENDEDOK);

    // calculate target time
    simtime_t eventSimtime = event->getArrivalTime();
    timeval targetTime = timeval_add(baseTime, SIMTIME_DBL(doScaling ? factor*eventSimtime : eventSimtime));

    // if needed, wait until that time arrives
    timeval curTime;
    gettimeofday(&curTime, NULL);
    if (timeval_greater(targetTime, curTime))
    {
        if (!waitUntil(targetTime))
            return NULL; // user break
    }
    else
    {
        // we're behind -- customized versions of this class may alert
        // if we're too much behind, or modify basetime to accept the skew
    }

    // remove event from FES and return it
    cEvent *tmp = sim->msgQueue.removeFirst();
    ASSERT(tmp==event);
    return event;
}



