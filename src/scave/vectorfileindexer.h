//=========================================================================
//  VECTORFILEINDEXER.H - part of
//                  OMNeT++/OMNEST
//           Discrete System Simulation in C++
//
//  Author: Tamas Borbely
//
//=========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __OMNETPP_IVECTORFILEINDEXER_H
#define __OMNETPP_IVECTORFILEINDEXER_H

#include <string>
#include "common/progressmonitor.h"
#include "resultfilemanager.h"

NAMESPACE_BEGIN

/**
 * Generate an index file (.vci) for an output vector file (.vec)
 */
class SCAVE_API VectorFileIndexer
{
    typedef OPP::common::IProgressMonitor IProgressMonitor;
    public:
        void generateIndex(const char *filename, IProgressMonitor *monitor = nullptr);

        void rebuildVectorFile(const char *filename, IProgressMonitor *monitor = nullptr);
};

NAMESPACE_END


#endif

