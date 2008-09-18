//
// Copyright (C) 2008 Andras Varga
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

#ifndef __JOIN_H__
#define __JOIN_H__

#include <list>
#include <omnetpp.h>
#include "Job.h"

namespace queueing {

/**
 * Merges sub-jobs generated by Split.
 */
class Join : public cSimpleModule
{
    protected:
        std::list<Job*> jobsHeld;
    public:
    	Join();
    	~Join();
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
};

}; // namespace

#endif
