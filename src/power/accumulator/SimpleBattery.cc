//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "SimpleBattery.h"
#include "NodeOperations.h"
#include "ModuleAccess.h"

namespace inet {

namespace power {

Define_Module(SimpleBattery);

SimpleBattery::SimpleBattery() :
    nominalCapacity(J(sNaN)),
    residualCapacity(J(sNaN)),
    printCapacityStep(J(sNaN)),
    lastResidualCapacityUpdate(-1),
    timer(NULL),
    nodeShutdownCapacity(J(sNaN)),
    nodeStartCapacity(J(sNaN)),
    lifecycleController(NULL),
    node(NULL),
    nodeStatus(NULL)
{
}

SimpleBattery::~SimpleBattery()
{
    cancelAndDelete(timer);
}

void SimpleBattery::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        nominalCapacity = J(par("nominalCapacity"));
        residualCapacity = J(par("initialCapacity"));
        printCapacityStep = J(par("printCapacityStep"));
        nodeShutdownCapacity = J(par("nodeShutdownCapacity"));
        nodeStartCapacity = J(par("nodeStartCapacity"));
        lastResidualCapacityUpdate = simTime();
        emit(residualCapacityChangedSignal, residualCapacity.get());
        timer = new cMessage("timer");
        if (!isNaN(nodeStartCapacity.get()) || !isNaN(nodeShutdownCapacity.get())) {
            node = findContainingNode(this);
            nodeStatus = dynamic_cast<NodeStatus *>(node->getSubmodule("status"));
            if (!nodeStatus)
                throw cRuntimeError("Cannot find node status");
            lifecycleController = dynamic_cast<LifecycleController *>(simulation.getModuleByPath("lifecycleController"));
            if (!lifecycleController)
                throw cRuntimeError("Cannot find lifecycle controller");
        }
        scheduleTimer();
    }
}

void SimpleBattery::handleMessage(cMessage *message)
{
    if (message == timer) {
        setResidualCapacity(targetCapacity);
        scheduleTimer();
        EV_INFO << "Battery, residual capacity = " << residualCapacity << " (" << (int)round(unit(residualCapacity / nominalCapacity).get() * 100) << "%)" << endl;
    }
    else
        throw cRuntimeError("Unknown message");
}

void SimpleBattery::setPowerConsumption(int id, W consumedPower)
{
    Enter_Method_Silent();
    updateResidualCapacity();
    PowerSourceBase::setPowerConsumption(id, consumedPower);
    scheduleTimer();
}

void SimpleBattery::setPowerGeneration(int id, W generatedPower)
{
    Enter_Method_Silent();
    updateResidualCapacity();
    PowerSinkBase::setPowerGeneration(id, generatedPower);
    scheduleTimer();
}

void SimpleBattery::executeNodeOperation(J newResidualCapacity)
{
    if (!isNaN(nodeShutdownCapacity.get()) && newResidualCapacity <= nodeShutdownCapacity && nodeStatus->getState() == NodeStatus::UP)
    {
        EV_WARN << "Battery capacity reached node shutdown threshold" << endl;
        LifecycleOperation::StringMap params;
        NodeShutdownOperation *operation = new NodeShutdownOperation();
        operation->initialize(node, params);
        lifecycleController->initiateOperation(operation);
    }
    else if (!isNaN(nodeStartCapacity.get()) && newResidualCapacity >= nodeStartCapacity && nodeStatus->getState() == NodeStatus::DOWN)
    {
        EV_INFO << "Battery capacity reached node start threshold" << endl;
        LifecycleOperation::StringMap params;
        NodeStartOperation *operation = new NodeStartOperation();
        operation->initialize(node, params);
        lifecycleController->initiateOperation(operation);
    }
}

void SimpleBattery::setResidualCapacity(J newResidualCapacity)
{
    if (newResidualCapacity != residualCapacity)
    {
        residualCapacity = newResidualCapacity;
        lastResidualCapacityUpdate = simTime();
        executeNodeOperation(newResidualCapacity);
        if (residualCapacity == J(0))
            EV_WARN << "Battery depleted" << endl;
        else if (residualCapacity == nominalCapacity)
            EV_INFO << "Battery charged" << endl;
        emit(residualCapacityChangedSignal, residualCapacity.get());
    }
}

void SimpleBattery::updateResidualCapacity()
{
    simtime_t now = simTime();
    if (now != lastResidualCapacityUpdate) {
        J newResidualCapacity = residualCapacity + s((now - lastResidualCapacityUpdate).dbl()) * (totalGeneratedPower - totalConsumedPower);
        if (newResidualCapacity < J(0))
            newResidualCapacity = J(0);
        else if (newResidualCapacity > nominalCapacity)
            newResidualCapacity = nominalCapacity;
        setResidualCapacity(newResidualCapacity);
    }
}

void SimpleBattery::scheduleTimer()
{
    W totalPower = totalGeneratedPower - totalConsumedPower;
    targetCapacity = residualCapacity;
    if (totalPower > W(0)) {
        targetCapacity = isNaN(printCapacityStep.get()) ? nominalCapacity : ceil(unit(residualCapacity / printCapacityStep).get()) * printCapacityStep;
        // NOTE: make sure capacity will change over time despite double arithmetic
        simtime_t remainingTime = unit((targetCapacity - residualCapacity) / totalPower / s(1)).get();
        if (remainingTime == 0)
            targetCapacity += printCapacityStep;
        // override target capacity if start is needed
        if (!isNaN(nodeStartCapacity.get()) && nodeStatus->getState() == NodeStatus::DOWN && residualCapacity < nodeStartCapacity && nodeStartCapacity < targetCapacity)
            targetCapacity = nodeStartCapacity;
    }
    else if (totalPower < W(0)) {
        targetCapacity = isNaN(printCapacityStep.get()) ? J(0) : floor(unit(residualCapacity / printCapacityStep).get()) * printCapacityStep;
        // make sure capacity will change over time despite double arithmetic
        simtime_t remainingTime = unit((targetCapacity - residualCapacity) / totalPower / s(1)).get();
        if (remainingTime == 0)
            targetCapacity -= printCapacityStep;
        // override target capacity if shutdown is needed
        if (!isNaN(nodeShutdownCapacity.get()) && nodeStatus->getState() == NodeStatus::UP && residualCapacity > nodeStartCapacity && nodeShutdownCapacity > targetCapacity)
            targetCapacity = nodeShutdownCapacity;
    }
    // enforce target capacity to be in range
    if (targetCapacity < J(0))
        targetCapacity = J(0);
    else if (targetCapacity > nominalCapacity)
        targetCapacity = nominalCapacity;
    simtime_t remainingTime = unit((targetCapacity - residualCapacity) / totalPower / s(1)).get();
    if (timer->isScheduled())
        cancelEvent(timer);
    // don't schedule if there's no progress
    if (remainingTime > 0)
        scheduleAt(simTime() + remainingTime, timer);
}

} // namespace power

} // namespace inet
