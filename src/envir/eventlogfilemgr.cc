//==========================================================================
//  EVENTLOGFILEMGR.CC - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//  Author: Andras Varga
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2015 Andras Varga
  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include <algorithm>
#include "common/opp_ctype.h"
#include "common/commonutil.h"  //vsnprintf
#include "common/fileutil.h"
#include "omnetpp/cconfigoption.h"
#include "omnetpp/cconfiguration.h"
#include "omnetpp/cmodule.h"
#include "omnetpp/cpacket.h"
#include "omnetpp/cgate.h"
#include "omnetpp/cchannel.h"
#include "omnetpp/csimplemodule.h"
#include "omnetpp/cdisplaystring.h"
#include "omnetpp/cclassdescriptor.h"
#include "eventlogfilemgr.h"
#include "eventlogwriter.h"
#include "envirbase.h"

NAMESPACE_BEGIN


Register_PerRunConfigOption(CFGID_EVENTLOG_FILE, "eventlog-file", CFG_FILENAME, "${resultdir}/${configname}-${runnumber}.elog", "Name of the eventlog file to generate.");
Register_PerRunConfigOption(CFGID_EVENTLOG_MESSAGE_DETAIL_PATTERN, "eventlog-message-detail-pattern", CFG_CUSTOM, nullptr,
        "A list of patterns separated by '|' character which will be used to write "
        "message detail information into the eventlog for each message sent during "
        "the simulation. The message detail will be presented in the sequence chart "
        "tool. Each pattern starts with an object pattern optionally followed by ':' "
        "character and a comma separated list of field patterns. In both "
        "patterns and/or/not/* and various field match expressions can be used. "
        "The object pattern matches to class name, the field pattern matches to field name by default.\n"
        "  EVENTLOG-MESSAGE-DETAIL-PATTERN := ( DETAIL-PATTERN '|' )* DETAIL_PATTERN\n"
        "  DETAIL-PATTERN := OBJECT-PATTERN [ ':' FIELD-PATTERNS ]\n"
        "  OBJECT-PATTERN := MATCH-EXPRESSION\n"
        "  FIELD-PATTERNS := ( FIELD-PATTERN ',' )* FIELD_PATTERN\n"
        "  FIELD-PATTERN := MATCH-EXPRESSION\n"
        "Examples (enter them without quotes):\n"
        "  \"*\": captures all fields of all messages\n"
        "  \"*Frame:*Address,*Id\": captures all fields named somethingAddress and somethingId from messages of any class named somethingFrame\n"
        "  \"MyMessage:declaredOn(MyMessage)\": captures instances of MyMessage recording the fields declared on the MyMessage class\n"
        "  \"*:(not declaredOn(cMessage) and not declaredOn(cNamedObject) and not declaredOn(cObject))\": records user-defined fields from all messages");
Register_PerRunConfigOption(CFGID_EVENTLOG_RECORDING_INTERVALS, "eventlog-recording-intervals", CFG_CUSTOM, nullptr, "Simulation time interval(s) when events should be recorded. Syntax: [<from>]..[<to>],... That is, both start and end of an interval are optional, and intervals are separated by comma. Example: ..10.2, 22.2..100, 233.3..");
Register_PerObjectConfigOption(CFGID_MODULE_EVENTLOG_RECORDING, "module-eventlog-recording", KIND_SIMPLE_MODULE, CFG_BOOL, "true", "Enables recording events on a per module basis. This is meaningful for simple modules only. \nExample:\n **.router[10..20].**.module-eventlog-recording = true\n **.module-eventlog-recording = false");

extern cConfigOption *CFGID_RECORD_EVENTLOG;

static bool compareMessageEventNumbers(cMessage *message1, cMessage *message2)
{
    return message1->getPreviousEventNumber() < message2->getPreviousEventNumber();
}

static ObjectPrinterRecursionControl recurseIntoMessageFields(void *object, cClassDescriptor *descriptor, int fieldIndex, void *fieldValue, void **parents, int level) {
    const char* propertyValue = descriptor->getFieldProperty(fieldIndex, "eventlog");

    if (propertyValue) {
        if (!strcmp(propertyValue, "skip"))
            return SKIP;
        else if (!strcmp(propertyValue, "fullName"))
            return FULL_NAME;
        else if (!strcmp(propertyValue, "fullPath"))
            return FULL_PATH;
    }

    bool isCObject = descriptor->getFieldIsCObject(fieldIndex);
    if (!isCObject)
        return RECURSE;
    else {
        if (!fieldValue)
            return RECURSE;
        else {
            cArray *array = dynamic_cast<cArray *>((cObject *)fieldValue);
            return !array || array->size() != 0 ? RECURSE : SKIP;
        }
    }
}

EventlogFileManager::EventlogFileManager()
{
    recordEventlog = false;
    feventlog = nullptr;
    objectPrinter = nullptr;
    recordingIntervals = nullptr;
    keyframeBlockSize = 1000;
    clearInternalState();
}

EventlogFileManager::~EventlogFileManager()
{
    delete objectPrinter;
    delete recordingIntervals;
}

void EventlogFileManager::clearInternalState()
{
    eventNumber = -1;
    entryIndex = -1;
    previousKeyframeFileOffset = -1;
    isEventLogRecordingEnabled = true;
    isIntervalEventLogRecordingEnabled = true;
    isModuleEventLogRecordingEnabled = true;
    consequenceLookaheadLimits.clear();
    eventNumberToSimulationStateEventLogEntryRanges.clear();
    moduleToModuleDisplayStringChangedEntryReferenceMap.clear();
    channelToConnectionDisplayStringChangedEntryReferenceMap.clear();
    messageToBeginSendEntryReferenceMap.clear();
}

void EventlogFileManager::configure()
{
    // main switch
    recordEventlog = getEnvir()->getConfig()->getAsBool(CFGID_RECORD_EVENTLOG);

    // setup eventlog object printer
    delete objectPrinter;
    objectPrinter = nullptr;
    const char *eventLogMessageDetailPattern = getEnvir()->getConfig()->getAsCustom(CFGID_EVENTLOG_MESSAGE_DETAIL_PATTERN);
    if (eventLogMessageDetailPattern)
        objectPrinter = new ObjectPrinter(recurseIntoMessageFields, eventLogMessageDetailPattern, 3);

    // setup eventlog recording intervals
    const char *text = getEnvir()->getConfig()->getAsCustom(CFGID_EVENTLOG_RECORDING_INTERVALS);
    if (text) {
        recordingIntervals = new Intervals();
        recordingIntervals->parse(text);
    }

    // setup filename
    filename = getEnvir()->getConfig()->getAsFilename(CFGID_EVENTLOG_FILE).c_str();
    dynamic_cast<EnvirBase *>(getEnvir())->processFileName(filename);
}

void EventlogFileManager::lifecycleEvent(SimulationLifecycleEventType eventType, cObject *details)
{
    switch (eventType) {
        case LF_PRE_NETWORK_SETUP:
            if (recordEventlog) {      //FIXME flag is duplicate!!!
                open();
                startRun();
            }
            else {
                remove();
            }
            break;
        case LF_ON_RUN_END:
            close();
            break;
        case LF_ON_SIMULATION_PAUSE:
            flush();
            break;
        default: break;
    }

}

void EventlogFileManager::open()
{
    if (!feventlog) {
        mkPath(directoryOf(filename.c_str()).c_str());
        FILE *out = fopen(filename.c_str(), "w");
        if (!out)
            throw cRuntimeError("Cannot open eventlog file `%s' for write", filename.c_str());
        ::printf("Recording eventlog to file `%s'...\n", filename.c_str());
        feventlog = out;
        clearInternalState();
    }
}

void EventlogFileManager::close()
{
    if (feventlog) {
        fclose(feventlog);
        feventlog = nullptr;
        isEventLogRecordingEnabled = false;
    }
}

void EventlogFileManager::remove()
{
    removeFile(filename.c_str(), "old eventlog file");
    entryIndex = -1;
}

void EventlogFileManager::recordSimulation()
{
    // TODO: this is a very simple and wrong implementation to be able to turn on eventlog recording after the simulation initialized or even after it is started
    // TODO: the real solution would be much more complicated than this, it would require additional eventlog entries to explicitly say what the simulation state is
    // TODO: writing a fake event line and pretending that the current state "happened" within that event is cheating and plain wrong
    // TODO: the simulation state is something that is "just being there" and we cannot describe that properly with the current eventlog entries that specify changes
    // TODO: such as CreateModule, BeginSend, etc.
    if (entryIndex == -1) {
        cModule *systemModule = getSimulation()->getSystemModule();
        recordInitialize();
        recordModules(systemModule);
        recordConnections(systemModule);
        recordMessages();
    }
}

void EventlogFileManager::recordInitialize()
{
    // TODO: we are always faking an initialize event to pretend that the whole simulation state has been created there
    // TODO: this is clearly a lie, but we do this only once per simulation
    eventNumber = 0;
    EventLogWriter::recordEventEntry_e_t_m_ce_msg(feventlog, eventNumber, 0, 1, -1, -1);
    entryIndex = 0;
    const char *runId = getEnvir()->getConfigEx()->getVariable(CFGVAR_RUNID);
    EventLogWriter::recordSimulationBeginEntry_v_rid_b(feventlog, OMNETPP_VERSION, runId, keyframeBlockSize);
    entryIndex++;
    recordKeyframe();
}

void EventlogFileManager::recordMessages()
{
    std::vector<cMessage *> messages;
    for (cMessageHeap::Iterator it = cMessageHeap::Iterator(getSimulation()->getMessageQueue()); !it.end(); it++) {
        cEvent *event = it();
        cMessage *msg = dynamic_cast<cMessage*>(event);
        if (msg)
            messages.push_back(msg);  //TODO record non-message cEvents too!
    }
    std::stable_sort(messages.begin(), messages.end(), compareMessageEventNumbers);
    eventnumber_t oldEventNumber = eventNumber;
    for (std::vector<cMessage *>::iterator it = messages.begin(); it != messages.end(); it++) {
        cMessage *msg = *it;
        if (eventNumber != msg->getPreviousEventNumber()) {
            fprintf(feventlog, "\n");
            eventNumber = msg->getPreviousEventNumber();
            EventLogWriter::recordEventEntry_e_t_m_ce_msg(feventlog, eventNumber, msg->getSendingTime(), msg->getSenderModuleId(), -1, -1);
            entryIndex = 0;
            removeBeginSendEntryReference(msg);
            recordKeyframe();
        }
        // TODO: this will write more than one fake ModuleMethodBegin entries for initialize, but it is a lie anyway
        if (eventNumber == 0)
            // NOTE: we lie that the network module called initialize in the arrival module which sent the message to itself
            EventLogWriter::recordModuleMethodBeginEntry_sm_tm_m(feventlog, 1, msg->getArrivalModuleId(), "initialize");
        eventnumber_t previousEventNumber = msg->getPreviousEventNumber();
        msg->setPreviousEventNumber(-1);
        messageCreated(msg);
        msg->setPreviousEventNumber(previousEventNumber);
        if (msg->isSelfMessage())
            messageScheduled(msg);
        else if (!msg->getSenderGate()) {
            beginSend(msg);
            if (msg->isPacket()) {
                cPacket *packet = (cPacket *)msg;
                simtime_t propagationDelay = packet->getArrivalTime() - packet->getSendingTime() - (packet->isReceptionStart() ? 0 : packet->getDuration());
                messageSendDirect(msg, msg->getArrivalGate(), propagationDelay, packet->getDuration());
            }
            else
                messageSendDirect(msg, msg->getArrivalGate(), 0, 0);
            endSend(msg);
        }
        else {
            beginSend(msg);
            messageSendHop(msg, msg->getSenderGate());
            endSend(msg);
        }
        if (eventNumber == 0)
            EventLogWriter::recordModuleMethodEndEntry(feventlog);
    }
    eventNumber = oldEventNumber;
}

void EventlogFileManager::recordModules(cModule *module)
{
    moduleCreated(module);
    for (cModule::GateIterator it(module); !it.end(); it++) {
        cGate *gate = it();
        gateCreated(gate);
    }
    displayStringChanged(module);
    for (cModule::SubmoduleIterator it(module); !it.end(); it++)
        recordModules(it());
}

void EventlogFileManager::recordConnections(cModule *module)
{
    for (cModule::GateIterator it(module); !it.end(); it++) {
        cGate *gate = it();
        if (gate->getNextGate())
            connectionCreated(gate);
        cChannel *channel = gate->getChannel();
        if (channel)
            displayStringChanged(channel);
    }
    for (cModule::SubmoduleIterator it(module); !it.end(); it++)
        recordConnections(it());
}

void EventlogFileManager::startRun()
{
    if (isEventLogRecordingEnabled) {
        eventNumber = 0;
        const char *runId = getEnvir()->getConfigEx()->getVariable(CFGVAR_RUNID);
        // we can't use simulation.getEventNumber() and simulation.getSimTime(), because when we start a new run
        // these numbers are still set from the previous run (i.e. not zero)
        EventLogWriter::recordEventEntry_e_t_m_ce_msg(feventlog, eventNumber, 0, 1, -1, -1);
        entryIndex = 0;
        EventLogWriter::recordSimulationBeginEntry_v_rid_b(feventlog, OMNETPP_VERSION, runId, keyframeBlockSize);
        entryIndex++;
        recordKeyframe();
        fflush(feventlog);
    }
}

void EventlogFileManager::endRun(bool isError, int resultCode, const char *message)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordSimulationEndEntry_e_c_m(feventlog, isError, resultCode, message);
        eventNumber = -1;
        entryIndex++;
        fflush(feventlog);
    }
}

bool EventlogFileManager::hasRecordingIntervals() const
{
    return recordingIntervals && !recordingIntervals->empty();
}

void EventlogFileManager::clearRecordingIntervals()
{
    if (recordingIntervals) {
        delete recordingIntervals;
        recordingIntervals = nullptr;
    }
}

void EventlogFileManager::flush()
{
    if (isEventLogRecordingEnabled)
        fflush(feventlog);
}

void EventlogFileManager::simulationEvent(cEvent *event)
{
    if (event->isMessage()) {
        cMessage *msg = static_cast<cMessage*>(event);
        cModule *mod = msg->getArrivalModule();
        eventNumber = getSimulation()->getEventNumber();
        bool isKeyframe = eventNumber % keyframeBlockSize == 0;
        isModuleEventLogRecordingEnabled = mod->isRecordEvents();
        isIntervalEventLogRecordingEnabled = !recordingIntervals || recordingIntervals->contains(getSimulation()->getSimTime());
        isEventLogRecordingEnabled = isKeyframe || (isModuleEventLogRecordingEnabled && isIntervalEventLogRecordingEnabled);
        if (isEventLogRecordingEnabled) {
            fprintf(feventlog, "\n");
            EventLogWriter::recordEventEntry_e_t_m_ce_msg(feventlog, eventNumber, getSimulation()->getSimTime(), mod->getId(), msg->getPreviousEventNumber(), msg->getId());
            entryIndex = 0;
            removeBeginSendEntryReference(msg);
            recordKeyframe();
        }
    }
    else {
        //TODO record non-message event
    }
}

void EventlogFileManager::bubble(cComponent *component, const char *text)
{
    if (isEventLogRecordingEnabled) {
        if (dynamic_cast<cModule *>(component)) {
            cModule *mod = (cModule *)component;
            EventLogWriter::recordBubbleEntry_id_txt(feventlog, mod->getId(), text);
            entryIndex++;
        }
        else if (dynamic_cast<cChannel *>(component)) {
            // TODO:
        }
    }
}

void EventlogFileManager::beginSend(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        // TODO: record message display string as well?
        if (msg->isPacket()) {
            cPacket *pkt = (cPacket *)msg;
            EventLogWriter::recordBeginSendEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                pkt->getId(), pkt->getTreeId(), pkt->getEncapsulationId(), pkt->getEncapsulationTreeId(),
                pkt->getClassName(), pkt->getFullName(),
                pkt->getKind(), pkt->getSchedulingPriority(), pkt->getBitLength(), pkt->hasBitError(),
                objectPrinter ? objectPrinter->printObjectToString(pkt).c_str() : nullptr,
                pkt->getPreviousEventNumber());
        }
        else {
            EventLogWriter::recordBeginSendEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                msg->getId(), msg->getTreeId(), msg->getId(), msg->getTreeId(),
                msg->getClassName(), msg->getFullName(),
                msg->getKind(), msg->getSchedulingPriority(), 0, false,
                objectPrinter ? objectPrinter->printObjectToString(msg).c_str() : nullptr,
                msg->getPreviousEventNumber());
        }
        entryIndex++;
        addPreviousEventNumber(msg->getPreviousEventNumber());
        addSimulationStateEventLogEntry(eventNumber, entryIndex);
        messageToBeginSendEntryReferenceMap[msg] = EventLogEntryReference(eventNumber, entryIndex);
    }
}

void EventlogFileManager::messageScheduled(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        EventlogFileManager::beginSend(msg);
        EventlogFileManager::endSend(msg);
    }
}

void EventlogFileManager::messageCancelled(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        if (msg->isPacket()) {
            cPacket *pkt = (cPacket *)msg;
            EventLogWriter::recordCancelEventEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                pkt->getId(), pkt->getTreeId(), pkt->getEncapsulationId(), pkt->getEncapsulationTreeId(),
                pkt->getClassName(), pkt->getFullName(),
                pkt->getKind(), pkt->getSchedulingPriority(), pkt->getBitLength(), pkt->hasBitError(),
                objectPrinter ? objectPrinter->printObjectToString(pkt).c_str() : nullptr,
                pkt->getPreviousEventNumber());
        }
        else {
            EventLogWriter::recordCancelEventEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                msg->getId(), msg->getTreeId(), msg->getId(), msg->getTreeId(),
                msg->getClassName(), msg->getFullName(),
                msg->getKind(), msg->getSchedulingPriority(), 0, false,
                objectPrinter ? objectPrinter->printObjectToString(msg).c_str() : nullptr,
                msg->getPreviousEventNumber());
        }
        entryIndex++;
        addPreviousEventNumber(msg->getPreviousEventNumber());
        removeBeginSendEntryReference(msg);
    }
}

void EventlogFileManager::messageSendDirect(cMessage *msg, cGate *toGate, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordSendDirectEntry_sm_dm_dg_pd_td(feventlog, msg->getSenderModuleId(), toGate->getOwnerModule()->getId(), toGate->getId(), propagationDelay, transmissionDelay);
        entryIndex++;
    }
}

void EventlogFileManager::messageSendHop(cMessage *msg, cGate *srcGate)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordSendHopEntry_sm_sg(feventlog, srcGate->getOwnerModule()->getId(), srcGate->getId());
        entryIndex++;
    }
}

void EventlogFileManager::messageSendHop(cMessage *msg, cGate *srcGate, simtime_t propagationDelay, simtime_t transmissionDelay)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordSendHopEntry_sm_sg_pd_td(feventlog, srcGate->getOwnerModule()->getId(), srcGate->getId(), propagationDelay, transmissionDelay);
        entryIndex++;
    }
}

void EventlogFileManager::endSend(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        bool isStart = msg->isPacket() ? ((cPacket *)msg)->isReceptionStart() : false;
        EventLogWriter::recordEndSendEntry_t_is(feventlog, msg->getArrivalTime(), isStart);
        entryIndex++;
    }
}

void EventlogFileManager::messageCreated(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        if (msg->isPacket()) {
            cPacket *pkt = (cPacket *)msg;
            EventLogWriter::recordCreateMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                pkt->getId(), pkt->getTreeId(), pkt->getEncapsulationId(), pkt->getEncapsulationTreeId(),
                pkt->getClassName(), pkt->getFullName(),
                pkt->getKind(), pkt->getSchedulingPriority(), pkt->getBitLength(), pkt->hasBitError(),
                objectPrinter ? objectPrinter->printObjectToString(pkt).c_str() : nullptr,
                pkt->getPreviousEventNumber());
        }
        else {
            EventLogWriter::recordCreateMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                msg->getId(), msg->getTreeId(), msg->getId(), msg->getTreeId(),
                msg->getClassName(), msg->getFullName(),
                msg->getKind(), msg->getSchedulingPriority(), 0, false,
                objectPrinter ? objectPrinter->printObjectToString(msg).c_str() : nullptr,
                msg->getPreviousEventNumber());
        }
        entryIndex++;
        addPreviousEventNumber(msg->getPreviousEventNumber());
    }
}

void EventlogFileManager::messageCloned(cMessage *msg, cMessage *clone)
{
    if (isEventLogRecordingEnabled) {
        if (msg->isPacket()) {
            cPacket *pkt = (cPacket *)msg;
            EventLogWriter::recordCloneMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe_cid(feventlog,
                pkt->getId(), pkt->getTreeId(), pkt->getEncapsulationId(), pkt->getEncapsulationTreeId(),
                pkt->getClassName(), pkt->getFullName(),
                pkt->getKind(), pkt->getSchedulingPriority(), pkt->getBitLength(), pkt->hasBitError(),
                objectPrinter ? objectPrinter->printObjectToString(pkt).c_str() : nullptr,
                pkt->getPreviousEventNumber(), clone->getId());
        }
        else {
            EventLogWriter::recordCloneMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe_cid(feventlog,
                msg->getId(), msg->getTreeId(), msg->getId(), msg->getTreeId(),
                msg->getClassName(), msg->getFullName(),
                msg->getKind(), msg->getSchedulingPriority(), 0, false,
                objectPrinter ? objectPrinter->printObjectToString(msg).c_str() : nullptr,
                msg->getPreviousEventNumber(), clone->getId());
        }
        entryIndex++;
        addPreviousEventNumber(msg->getPreviousEventNumber());
    }
}

void EventlogFileManager::messageDeleted(cMessage *msg)
{
    if (isEventLogRecordingEnabled) {
        if (msg->isPacket()) {
            cPacket *pkt = (cPacket *)msg;
            EventLogWriter::recordDeleteMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                pkt->getId(), pkt->getTreeId(), pkt->getEncapsulationId(), pkt->getEncapsulationTreeId(),
                pkt->getClassName(), pkt->getFullName(),
                pkt->getKind(), pkt->getSchedulingPriority(), pkt->getBitLength(), pkt->hasBitError(),
                objectPrinter ? objectPrinter->printObjectToString(pkt).c_str() : nullptr,
                pkt->getPreviousEventNumber());
        }
        else {
            EventLogWriter::recordDeleteMessageEntry_id_tid_eid_etid_c_n_k_p_l_er_d_pe(feventlog,
                msg->getId(), msg->getTreeId(), msg->getId(), msg->getTreeId(),
                msg->getClassName(), msg->getFullName(),
                msg->getKind(), msg->getSchedulingPriority(), 0, false,
                objectPrinter ? objectPrinter->printObjectToString(msg).c_str() : nullptr,
                msg->getPreviousEventNumber());
        }
        entryIndex++;
        addPreviousEventNumber(msg->getPreviousEventNumber());
    }
}

void EventlogFileManager::componentMethodBegin(cComponent *from, cComponent *to, const char *methodFmt, va_list va)
{
    if (isEventLogRecordingEnabled) {
        if (from && from->isModule() && to->isModule()) {
            const char *methodText = "";  // for the Enter_Method_Silent case
            if (methodFmt) {
                static char methodTextBuf[MAX_METHODCALL];
                vsnprintf(methodTextBuf, MAX_METHODCALL, methodFmt, va);
                methodTextBuf[MAX_METHODCALL-1] = '\0';
                methodText = methodTextBuf;
            }
            EventLogWriter::recordModuleMethodBeginEntry_sm_tm_m(feventlog, ((cModule *)from)->getId(), ((cModule *)to)->getId(), methodText);
            entryIndex++;
        }
    }
}

void EventlogFileManager::componentMethodEnd()
{
    if (isEventLogRecordingEnabled) {
        // TODO: problem when channel method is called: we'll emit an "End" entry but no "Begin"
        // TODO: same problem when the caller is not a module or is NULL
        EventLogWriter::recordModuleMethodEndEntry(feventlog);
        entryIndex++;
    }
}

void EventlogFileManager::moduleCreated(cModule *module)
{
    if (isEventLogRecordingEnabled) {
        bool recordModuleEvents = getEnvir()->getConfig()->getAsBool(module->getFullPath().c_str(), CFGID_MODULE_EVENTLOG_RECORDING);
        module->setRecordEvents(recordModuleEvents);
        bool isCompoundModule = !module->isSimple();
        // FIXME: size() is missing
        EventLogWriter::recordModuleCreatedEntry_id_c_t_pid_n_cm(feventlog, module->getId(), module->getClassName(), module->getNedTypeName(), module->getParentModule() ? module->getParentModule()->getId() : -1, module->getFullName(), isCompoundModule);
        entryIndex++;
        addSimulationStateEventLogEntry(eventNumber, entryIndex);
    }
}

void EventlogFileManager::moduleDeleted(cModule *module)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordModuleDeletedEntry_id(feventlog, module->getId());
        entryIndex++;
    }
}

void EventlogFileManager::moduleReparented(cModule *module, cModule *oldparent, int oldId)
{
    if (isEventLogRecordingEnabled) {
        throw cRuntimeError("Tools based on the eventlog do not support module reparenting -- please turn off eventlog recording if your model contains calls to cModule::changeParent()");
    }
}

void EventlogFileManager::gateCreated(cGate *newgate)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordGateCreatedEntry_m_g_n_i_o(feventlog, newgate->getOwnerModule()->getId(), newgate->getId(), newgate->getName(), newgate->isVector() ? newgate->getIndex() : -1, newgate->getType() == cGate::OUTPUT);
        entryIndex++;
        addSimulationStateEventLogEntry(eventNumber, entryIndex);
    }
}

void EventlogFileManager::gateDeleted(cGate *gate)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordGateDeletedEntry_m_g(feventlog, gate->getOwnerModule()->getId(), gate->getId());
        entryIndex++;
    }
}

void EventlogFileManager::connectionCreated(cGate *srcgate)
{
    if (isEventLogRecordingEnabled) {
        cGate *destgate = srcgate->getNextGate();
        // TODO: channel, channel attributes, etc
        EventLogWriter::recordConnectionCreatedEntry_sm_sg_dm_dg(feventlog, srcgate->getOwnerModule()->getId(), srcgate->getId(), destgate->getOwnerModule()->getId(), destgate->getId());
        entryIndex++;
        addSimulationStateEventLogEntry(eventNumber, entryIndex);
    }
}

void EventlogFileManager::connectionDeleted(cGate *srcgate)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordConnectionDeletedEntry_sm_sg(feventlog, srcgate->getOwnerModule()->getId(), srcgate->getId());
        entryIndex++;
    }
}

void EventlogFileManager::displayStringChanged(cComponent *component)
{
    if (isEventLogRecordingEnabled) {
        if (dynamic_cast<cModule *>(component)) {
            cModule *module = (cModule *)component;
            EventLogWriter::recordModuleDisplayStringChangedEntry_id_d(feventlog, module->getId(), module->getDisplayString().str());
            entryIndex++;
            addSimulationStateEventLogEntry(eventNumber, entryIndex);
            std::map<cModule *, EventLogEntryReference>::iterator it = moduleToModuleDisplayStringChangedEntryReferenceMap.find(module);
            if (it != moduleToModuleDisplayStringChangedEntryReferenceMap.end())
                removeSimulationStateEventLogEntry((*it).second);
            moduleToModuleDisplayStringChangedEntryReferenceMap[module] = EventLogEntryReference(eventNumber, entryIndex);
        }
        else if (dynamic_cast<cChannel *>(component)) {
            cChannel *channel = (cChannel *)component;
            cGate *gate = channel->getSourceGate();
            EventLogWriter::recordConnectionDisplayStringChangedEntry_sm_sg_d(feventlog, gate->getOwnerModule()->getId(), gate->getId(), channel->getDisplayString().str());
            entryIndex++;
            addSimulationStateEventLogEntry(eventNumber, entryIndex);
            std::map<cChannel *, EventLogEntryReference>::iterator it = channelToConnectionDisplayStringChangedEntryReferenceMap.find(channel);
            if (it != channelToConnectionDisplayStringChangedEntryReferenceMap.end())
                removeSimulationStateEventLogEntry((*it).second);
            channelToConnectionDisplayStringChangedEntryReferenceMap[channel] = EventLogEntryReference(eventNumber, entryIndex);
        }
    }
}

void EventlogFileManager::logLine(const char *prefix, const char *line, int lineLength)
{
    if (isEventLogRecordingEnabled) {
        EventLogWriter::recordLogLine(feventlog, prefix, line, lineLength);
        entryIndex++;
    }
}

//========================================================================== keyframe management

void EventlogFileManager::addSimulationStateEventLogEntry(EventLogEntryReference reference)
{
    addSimulationStateEventLogEntry(reference.eventNumber, reference.entryIndex);
}

void EventlogFileManager::addSimulationStateEventLogEntry(eventnumber_t eventNumber, int entryIndex)
{
    std::map<eventnumber_t, std::vector<EventLogEntryRange> >::iterator it = eventNumberToSimulationStateEventLogEntryRanges.find(eventNumber);
    if (it != eventNumberToSimulationStateEventLogEntryRanges.end()) {
        std::vector<EventLogEntryRange> &ranges = it->second;
        EventLogEntryRange &back = ranges.back();
        if (back.eventNumber == eventNumber && back.endEntryIndex == entryIndex - 1)
            back.endEntryIndex++;
        else
            ranges.push_back(EventLogEntryRange(eventNumber, entryIndex, entryIndex));
    }
    else {
        std::vector<EventLogEntryRange> ranges;
        ranges.push_back(EventLogEntryRange(eventNumber, entryIndex, entryIndex));
        eventNumberToSimulationStateEventLogEntryRanges[eventNumber] = ranges;
    }
}

void EventlogFileManager::removeSimulationStateEventLogEntry(EventLogEntryReference reference)
{
    removeSimulationStateEventLogEntry(reference.eventNumber, reference.entryIndex);
}

void EventlogFileManager::removeSimulationStateEventLogEntry(eventnumber_t eventNumber, int entryIndex)
{
    std::map<eventnumber_t, std::vector<EventLogEntryRange> >::iterator it = eventNumberToSimulationStateEventLogEntryRanges.find(eventNumber);
    if (it != eventNumberToSimulationStateEventLogEntryRanges.end()) {
        std::vector<EventLogEntryRange> &ranges = it->second;
        for (std::vector<EventLogEntryRange>::iterator jt = ranges.begin(); jt != ranges.end(); jt++) {
            EventLogEntryRange eventLogEntryRange = *jt;
            int beginEntryIndex = eventLogEntryRange.beginEntryIndex;
            int endEntryIndex = eventLogEntryRange.endEntryIndex;
            if (eventLogEntryRange.eventNumber == eventNumber && beginEntryIndex <= entryIndex && entryIndex <= endEntryIndex) {
                ranges.erase(jt);
                if (eventLogEntryRange.beginEntryIndex != eventLogEntryRange.endEntryIndex) {
                    if (beginEntryIndex != entryIndex)
                        ranges.push_back(EventLogEntryRange(eventNumber, beginEntryIndex, entryIndex - 1));
                    if (endEntryIndex != entryIndex)
                        ranges.push_back(EventLogEntryRange(eventNumber, entryIndex + 1, endEntryIndex));
                }
                if (ranges.size() == 0)
                    eventNumberToSimulationStateEventLogEntryRanges.erase(it);
                return;
            }
        }
    }
}

void EventlogFileManager::removeBeginSendEntryReference(cMessage *message)
{
    std::map<cMessage *, EventLogEntryReference>::iterator it = messageToBeginSendEntryReferenceMap.find(message);
    if (it != messageToBeginSendEntryReferenceMap.end())
        removeSimulationStateEventLogEntry((*it).second);
    messageToBeginSendEntryReferenceMap.erase(message);
}

void EventlogFileManager::recordKeyframe()
{
    if (eventNumber % keyframeBlockSize == 0) {
        consequenceLookaheadLimits.push_back(0);
        file_offset_t newPreviousKeyframeFileOffset = opp_ftell(feventlog);
        fprintf(feventlog, "KF");
        // previousKeyframeFileOffset
        fprintf(feventlog, " p %" INT64_PRINTF_FORMAT "d", previousKeyframeFileOffset);
        previousKeyframeFileOffset = newPreviousKeyframeFileOffset;
        // consequenceLookahead
        fprintf(feventlog, " c ");
        int i = 0;
        bool empty = true;
        for (std::vector<eventnumber_t>::iterator it = consequenceLookaheadLimits.begin(); it != consequenceLookaheadLimits.end(); it++) {
            eventnumber_t consequenceLookaheadLimit = *it;
            if (consequenceLookaheadLimit) {
                fprintf(feventlog, "%" INT64_PRINTF_FORMAT "d:%" INT64_PRINTF_FORMAT "d,", (eventnumber_t)keyframeBlockSize * i, consequenceLookaheadLimit);
                empty = false;
            }
            *it = 0;
            i++;
        }
        if (empty)
            fprintf(feventlog, "\"\"");
        // simulationStateEntries
        empty = true;
        fprintf(feventlog, " s ");
        for (std::map<eventnumber_t, std::vector<EventLogEntryRange> >::iterator it = eventNumberToSimulationStateEventLogEntryRanges.begin(); it != eventNumberToSimulationStateEventLogEntryRanges.end(); it++) {
            std::vector<EventLogEntryRange> &ranges = it->second;
            for (std::vector<EventLogEntryRange>::iterator jt = ranges.begin(); jt != ranges.end(); jt++) {
                (*jt).print(feventlog);
                fprintf(feventlog, ",");
                empty = false;
            }
        }
        if (empty)
            fprintf(feventlog, "\"\"");
        fprintf(feventlog, "\n");
        entryIndex++;
    }
}

void EventlogFileManager::addPreviousEventNumber(eventnumber_t previousEventNumber)
{
    if (previousEventNumber != -1) {
        eventnumber_t blockIndex = previousEventNumber / keyframeBlockSize;
        if (blockIndex + 1 > (eventnumber_t)consequenceLookaheadLimits.size())
            consequenceLookaheadLimits.resize(blockIndex + 1);
        consequenceLookaheadLimits[blockIndex] = std::max(consequenceLookaheadLimits[blockIndex], eventNumber - previousEventNumber);
    }
}

NAMESPACE_END
