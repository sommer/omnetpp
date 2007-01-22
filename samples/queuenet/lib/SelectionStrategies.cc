//
// Copyright (C) 2006 Rudolf Hornig
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include <omnetpp.h>
#include "SelectionStrategies.h"
#include "PQueue.h"
#include "Server.h"

SelectionStrategy::SelectionStrategy(cSimpleModule *module, bool selectOnInGate)
{
    hostModule = module;
    isInputGate = selectOnInGate;
    gateSize = isInputGate ? hostModule->gateSize("in") : hostModule->gateSize("out");                
}

SelectionStrategy::~SelectionStrategy()
{
}

SelectionStrategy *SelectionStrategy::create(const char *algName, cSimpleModule *module, bool selectOnInGate)
{
    SelectionStrategy *strat = NULL;
    
    if (strcmp(algName, "priority") == 0) {
        strat = new PrioritySelectionStrategy(module, selectOnInGate);
    } else if (strcmp(algName, "random") == 0) {
        strat = new RandomSelectionStrategy(module, selectOnInGate);
    } else if (strcmp(algName, "roundRobin") == 0) {
        strat = new RoundRobinSelectionStrategy(module, selectOnInGate);
    } else if (strcmp(algName, "shortestQueue") == 0) {
        strat = new ShortestQueueSelectionStrategy(module, selectOnInGate);
    } else if (strcmp(algName, "longestQueue") == 0) {
        strat = new LongestQueueSelectionStrategy(module, selectOnInGate);
    }
    
    return strat;
}
    
cGate *SelectionStrategy::selectableGate(int i)
{
    if (isInputGate)
        return hostModule->gate("in", i)->fromGate();                
    else
        return hostModule->gate("out", i)->toGate();
        
    return NULL;                
}

bool SelectionStrategy::isSelectable(cModule *module)
{
    PQueue *pqueue = dynamic_cast<PQueue *>(module);
    if (pqueue != NULL)
    {
        return pqueue->length() > 0;
    }
    Server *server = dynamic_cast<Server *>(module);
    if (server != NULL)
    {
        return server->isIdle();
    }
    
    throw cRuntimeError("Only PQueue and Server is supported by this Strategy");
    return true;
}

// --------------------------------------------------------------------------------------------

PrioritySelectionStrategy::PrioritySelectionStrategy(cSimpleModule *module, bool selectOnInGate) :
    SelectionStrategy(module, selectOnInGate)
{
}

// return the smallest selectable index
int PrioritySelectionStrategy::select()
{
    for(int i=0; i<gateSize; i++)
        if (isSelectable(selectableGate(i)->ownerModule()))
            return i;
    // if none of them is selectable return an invalid no.
    return -1;            
}
    
// --------------------------------------------------------------------------------------------

RandomSelectionStrategy::RandomSelectionStrategy(cSimpleModule *module, bool selectOnInGate) :
    SelectionStrategy(module, selectOnInGate)
{
}

// return the smallest selectable index
int RandomSelectionStrategy::select()
{
    int result = -1;
    int noOfSelectables = 0;
    for(int i=0; i<gateSize; i++)
        if (isSelectable(selectableGate(i)->ownerModule()))
            noOfSelectables++;
    
    int rnd = intuniform(1, noOfSelectables);
    
    for(int i=0; i<gateSize; i++)
        if (isSelectable(selectableGate(i)->ownerModule()) && (--rnd == 0))
        {
            result = i;
            break;
        }
    
    return result;            
}

// --------------------------------------------------------------------------------------------

RoundRobinSelectionStrategy::RoundRobinSelectionStrategy(cSimpleModule *module, bool selectOnInGate) :
    SelectionStrategy(module, selectOnInGate)
{
    lastIndex = -1;
}

// return the smallest selectable index
int RoundRobinSelectionStrategy::select()
{
    for(int i = 0; i<gateSize; ++i)
    {
        lastIndex = (lastIndex+1) % gateSize;
        if (isSelectable(selectableGate(lastIndex)->ownerModule()))
            return lastIndex;
    }        
    // if none of them is selectable return an invalid no.
    return -1;            
}

// --------------------------------------------------------------------------------------------

ShortestQueueSelectionStrategy::ShortestQueueSelectionStrategy(cSimpleModule *module, bool selectOnInGate) :
    SelectionStrategy(module, selectOnInGate)
{
}

// return the smallest selectable index
int ShortestQueueSelectionStrategy::select()
{   
    int result = -1;            // by default none of them is selectable
    int sizeMin = 2^30;
    for(int i = 0; i<gateSize; ++i)
    {
        PQueue *queue = check_and_cast<PQueue *>(selectableGate(i)->ownerModule());
        if (isSelectable(queue) && (queue->length()<sizeMin))
        {
            sizeMin = queue->length();
            result = i;
        }
    }
    return result;            
}

// --------------------------------------------------------------------------------------------

LongestQueueSelectionStrategy::LongestQueueSelectionStrategy(cSimpleModule *module, bool selectOnInGate) :
    SelectionStrategy(module, selectOnInGate)
{
}

// return the longest selectable queue
int LongestQueueSelectionStrategy::select()
{
    int result = -1;            // by default none of them is selectable
    int sizeMax = -1;
    for(int i = 0; i<gateSize; ++i)
    {
        PQueue *queue = check_and_cast<PQueue *>(selectableGate(i)->ownerModule());
        if (isSelectable(queue) && queue->length()>sizeMax)
        {
            sizeMax = queue->length();
            result = i;
        }
    }
    return result;            
}


