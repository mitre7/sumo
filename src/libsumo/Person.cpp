/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2017-2017 German Aerospace Center (DLR) and others.
/****************************************************************************/
//
//   This program and the accompanying materials
//   are made available under the terms of the Eclipse Public License v2.0
//   which accompanies this distribution, and is available at
//   http://www.eclipse.org/legal/epl-v20.html
//
/****************************************************************************/
/// @file    Person.cpp
/// @author  Leonhard Luecken
/// @date    15.09.2017
/// @version $Id$
///
// C++ TraCI client API implementation
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <microsim/MSTransportableControl.h>
#include <microsim/MSVehicleControl.h>
#include <microsim/MSEdge.h>
#include <microsim/MSNet.h>
#include <microsim/pedestrians/MSPerson.h>
#include <utils/geom/GeomHelper.h>
#include <utils/common/StringTokenizer.h>
#include "VehicleType.h"
#include "Person.h"


// ===========================================================================
// member definitions
// ===========================================================================
namespace libsumo {
    std::vector<std::string>
        Person::getIDList() {
        MSTransportableControl& c = MSNet::getInstance()->getPersonControl();
        std::vector<std::string> ids;
        for (MSTransportableControl::constVehIt i = c.loadedBegin(); i != c.loadedEnd(); ++i) {
            if (i->second->getCurrentStageType() != MSTransportable::WAITING_FOR_DEPART) {
                ids.push_back(i->first);
            }
        }
        return std::move(ids);
    }


    int
        Person::getIDCount() {
        return MSNet::getInstance()->getPersonControl().size();
    }


    TraCIPosition
        Person::getPosition(const std::string& personID) {
        MSTransportable* p = getPerson(personID);
        TraCIPosition pos;
        pos.x = p->getPosition().x();
        pos.y = p->getPosition().y();
        pos.z = p->getPosition().z();
        return pos;
    }


    double
        Person::getAngle(const std::string& personID){
        return GeomHelper::naviDegree(getPerson(personID)->getAngle());
    }


    double
        Person::getSpeed(const std::string& personID){
        return getPerson(personID)->getSpeed();
    }


    std::string
        Person::getRoadID(const std::string& personID){
        return getPerson(personID)->getEdge()->getID();
    }


    double
        Person::getLanePosition(const std::string& personID) {
        return getPerson(personID)->getEdgePos();
    }


    TraCIColor
        Person::getColor(const std::string& personID){
        const RGBColor& col = getPerson(personID)->getParameter().color;
        TraCIColor tcol;
        tcol.r = col.red();
        tcol.g = col.green();
        tcol.b = col.blue();
        tcol.a = col.alpha();
        return tcol;
    }


    std::string
        Person::getTypeID(const std::string& personID) {
        return getPerson(personID)->getVehicleType().getID();
    }


    double
        Person::getWaitingTime(const std::string& personID) {
        return getPerson(personID)->getWaitingSeconds();
    }


    std::string
        Person::getNextEdge(const std::string& personID) {
        return dynamic_cast<MSPerson*>(getPerson(personID))->getNextEdge();
    }


    std::vector<std::string>
        Person::getEdges(const std::string& personID, int nextStageIndex) {
        MSTransportable* p = getPerson(personID);
        if (nextStageIndex >= p->getNumRemainingStages()) {
            throw TraCIException("The stage index must be lower than the number of remaining stages.");
        }
        if (nextStageIndex < (p->getNumRemainingStages() - p->getNumStages())) {
            throw TraCIException("The negative stage index must refer to a valid previous stage.");
        }
        std::vector<std::string> edgeIDs;
        for (auto& e : p->getEdges(nextStageIndex)) {
            edgeIDs.push_back(e->getID());
        }
        return edgeIDs;
    }


    int
        Person::getStage(const std::string& personID, int nextStageIndex) {
        MSTransportable* p = getPerson(personID);
        if (nextStageIndex >= p->getNumRemainingStages()) {
            throw TraCIException("The stage index must be lower than the number of remaining stages.");
        }
        if (nextStageIndex < (p->getNumRemainingStages() - p->getNumStages())) {
            throw TraCIException("The negative stage index must refer to a valid previous stage.");
        }
        return p->getStageType(nextStageIndex);
    }


    int
        Person::getRemainingStages(const std::string& personID) {
        return getPerson(personID)->getNumRemainingStages();
    }


    std::string
        Person::getVehicle(const std::string& personID) {
        const SUMOVehicle* veh = getPerson(personID)->getVehicle();
        if (veh == nullptr) {
            return "";
        } else {
            return veh->getID();
        }
    }


    std::string
        Person::getParameter(const std::string& personID, const std::string& param) {
        return getPerson(personID)->getParameter().getParameter(param, "");
    }




    void
        Person::setSpeed(const std::string& personID, double speed) {
        getPerson(personID)->setSpeed(speed);
    }


    void
        Person::setType(const std::string& personID, const std::string& typeID) {
        MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(typeID);
        if (vehicleType == 0) {
            throw TraCIException("The vehicle type '" + typeID + "' is not known.");
        }
        getPerson(personID)->replaceVehicleType(vehicleType);
    }


    void
        Person::add(const std::string& personID, const std::string& edgeID, double pos, double departInSecs, const std::string typeID) {
        MSTransportable* p;
        try {
            p = getPerson(personID);
        } catch (TraCIException&) {
            p = nullptr;
        }

        if (p != nullptr) {
            throw TraCIException("The person " + personID + " to add already exists.");
        }

        SUMOTime depart = TIME2STEPS(departInSecs);
        SUMOVehicleParameter vehicleParams;
        vehicleParams.id = personID;

        MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(typeID);
        if (!vehicleType) {
            throw TraCIException("Invalid type '" + typeID + "' for person '" + personID + "'");
        }

        const MSEdge* edge = MSEdge::dictionary(edgeID);
        if (!edge) {
            throw TraCIException("Invalid edge '" + edgeID + "' for person: '" + personID + "'");
        }

        if (depart < 0) {
            const int proc = (int)-depart;
            if (proc >= static_cast<int>(DEPART_DEF_MAX)) {
                throw TraCIException("Invalid departure time.");
            }
            vehicleParams.departProcedure = (DepartDefinition)proc;
            vehicleParams.depart = MSNet::getInstance()->getCurrentTimeStep();
        } else if (depart < MSNet::getInstance()->getCurrentTimeStep()) {
            vehicleParams.depart = MSNet::getInstance()->getCurrentTimeStep();
            WRITE_WARNING("Departure time " + toString(departInSecs) + " for person '" + personID
                + "' is in the past; using current time " + time2string(vehicleParams.depart) + " instead.");
        } else {
            vehicleParams.depart = depart;
        }

        vehicleParams.departPosProcedure = DEPART_POS_GIVEN;
        if (fabs(pos) > edge->getLength()) {
            throw TraCIException("Invalid departure position.");
        }
        if (pos < 0) {
            pos += edge->getLength();
        }
        vehicleParams.departPos = pos;

        SUMOVehicleParameter* params = new SUMOVehicleParameter(vehicleParams);
        MSTransportable::MSTransportablePlan* plan = new MSTransportable::MSTransportablePlan();
        plan->push_back(new MSTransportable::Stage_Waiting(*edge, 0, depart, pos, "awaiting departure", true));

        try {
            MSTransportable* person = MSNet::getInstance()->getPersonControl().buildPerson(params, vehicleType, plan, 0);
            MSNet::getInstance()->getPersonControl().add(person);
        } catch (ProcessError& e) {
            delete params;
            delete plan;
            throw TraCIException(e.what());
        }
    }


    void
        Person::appendDrivingStage(const std::string& personID, const std::string& toEdge, const std::string& lines, const std::string& stopID) {
        MSTransportable* p = getPerson(personID);
        const MSEdge* edge = MSEdge::dictionary(toEdge);
        if (!edge) {
            throw TraCIException("Invalid edge '" + toEdge + "' for person: '" + personID + "'");
        }
        if (lines.size() == 0) {
            return throw TraCIException("Empty lines parameter for person: '" + personID + "'");
        }
        MSStoppingPlace* bs = 0;
        if (stopID != "") {
            bs = MSNet::getInstance()->getStoppingPlace(stopID, SUMO_TAG_BUS_STOP);
            if (bs == 0) {
                throw TraCIException("Invalid stopping place id '" + stopID + "' for person: '" + personID + "'");
            }
        }
        p->appendStage(new MSPerson::MSPersonStage_Driving(*edge, bs, -NUMERICAL_EPS, StringTokenizer(lines).getVector()));
    }


    void
        Person::appendWaitingStage(const std::string& personID, double duration, const std::string& description, const std::string& stopID) {
        MSTransportable* p = getPerson(personID);
        if (duration < 0) {
            throw TraCIException("Duration for person: '" + personID + "' must not be negative");
        }
        MSStoppingPlace* bs = 0;
        if (stopID != "") {
            bs = MSNet::getInstance()->getStoppingPlace(stopID, SUMO_TAG_BUS_STOP);
            if (bs == 0) {
                throw TraCIException("Invalid stopping place id '" + stopID + "' for person: '" + personID + "'");
            }
        }
        p->appendStage(new MSTransportable::Stage_Waiting(*p->getArrivalEdge(), TIME2STEPS(duration), 0, p->getArrivalPos(), description, false));
    }


    void
        Person::appendWalkingStage(const std::string& personID, const std::vector<std::string>& edgeIDs, double arrivalPos, double duration, double speed, const std::string& stopID) {
        MSTransportable* p = getPerson(personID);
        ConstMSEdgeVector edges;
        try {
            MSEdge::parseEdgesList(edgeIDs, edges, "<unknown>");
        } catch (ProcessError& e) {
            throw TraCIException(e.what());
        }
        if (edges.empty()) {
            throw TraCIException("Empty edge list for walking stage of person '" + personID + "'.");
        }
        if (fabs(arrivalPos) > edges.back()->getLength()) {
            throw TraCIException("Invalid arrivalPos for walking stage of person '" + personID + "'.");
        }
        if (arrivalPos < 0) {
            arrivalPos += edges.back()->getLength();
        }
        if (speed < 0) {
            speed = p->getVehicleType().getMaxSpeed();
        }
        MSStoppingPlace* bs = 0;
        if (stopID != "") {
            bs = MSNet::getInstance()->getStoppingPlace(stopID, SUMO_TAG_BUS_STOP);
            if (bs == 0) {
                throw TraCIException("Invalid stopping place id '" + stopID + "' for person: '" + personID + "'");
            }
        }
        p->appendStage(new MSPerson::MSPersonStage_Walking(p->getID(), edges, bs, TIME2STEPS(duration), speed, p->getArrivalPos(), arrivalPos, 0));
    }


    void
        Person::removeStage(const std::string& personID, int nextStageIndex) {
        MSTransportable* p = getPerson(personID);
        if (nextStageIndex >= p->getNumRemainingStages()) {
            throw TraCIException("The stage index must be lower than the number of remaining stages.");
        }
        if (nextStageIndex < 0) {
            throw TraCIException("The stage index may not be negative.");
        }
        p->removeStage(nextStageIndex);
    }


    void
        Person::rerouteTraveltime(const std::string& personID) {
        MSTransportable* p = getPerson(personID);
        if (p->getNumRemainingStages() == 0 || p->getCurrentStageType() != MSTransportable::MOVING_WITHOUT_VEHICLE) {
            throw TraCIException("Person '" + personID + "' is not currenlty walking.");
        }
        const MSEdge* from = p->getEdge();
        double  departPos = p->getEdgePos();
        const MSEdge* to = p->getArrivalEdge();
        double  arrivalPos = p->getArrivalPos();
        double speed = p->getVehicleType().getMaxSpeed();
        ConstMSEdgeVector newEdges;
        MSNet::getInstance()->getPedestrianRouter().compute(from, to, departPos, arrivalPos, speed, 0, 0, newEdges);
        if (newEdges.empty()) {
            throw TraCIException("Could not find new route for person '" + personID + "'.");
        }
        ConstMSEdgeVector oldEdges = p->getEdges(0);
        assert(!oldEdges.empty());
        if (oldEdges.front()->getFunction() != EDGEFUNC_NORMAL) {
            oldEdges.erase(oldEdges.begin());
        }
        if (newEdges == oldEdges) {
            return;
        }
        if (newEdges.front() != from) {
            // @note: maybe this should be done automatically by the router
            newEdges.insert(newEdges.begin(), from);
        }
        //std::cout << " from=" << from->getID() << " to=" << to->getID() << " newEdges=" << toString(newEdges) << "\n";
        MSPerson::MSPersonStage_Walking* newStage = new MSPerson::MSPersonStage_Walking(p->getID(), newEdges, 0, -1, speed, departPos, arrivalPos, 0);
        if (p->getNumRemainingStages() == 1) {
            // Do not remove the last stage (a waiting stage would be added otherwise)
            p->appendStage(newStage);
            //std::cout << "case a: remaining=" << p->getNumRemainingStages() << "\n";
            p->removeStage(0);
        } else {
            p->removeStage(0);
            p->appendStage(newStage);
            //std::cout << "case b: remaining=" << p->getNumRemainingStages() << "\n";
        }
    }


    /** untested setter functions which alter the person's vtype ***/

    void
        Person::setParameter(const std::string& personID, const std::string& key, const std::string& value) {
        MSTransportable* p = getPerson(personID);
        ((SUMOVehicleParameter&)p->getParameter()).setParameter(key, value);
    }

    void
        Person::setLength(const std::string& personID, double length) {
        VehicleType::getVType(getSingularVType(personID))->setLength(length);
    }

    void
        Person::setWidth(const std::string& personID, double width) {
        VehicleType::getVType(getSingularVType(personID))->setWidth(width);
    }

    void
        Person::setHeight(const std::string& personID, double height) {
        VehicleType::getVType(getSingularVType(personID))->setHeight(height);
    }

    void
        Person::setMinGap(const std::string& personID, double minGap) {
        VehicleType::getVType(getSingularVType(personID))->setMinGap(minGap);
    }

    void
        Person::setColor(const std::string& personID, const TraCIColor& c) {
        VehicleType::getVType(getSingularVType(personID))->setColor(RGBColor(c.r, c.g, c.b, c.a));
    }



    /******** private functions *************/

    MSTransportable*
        Person::getPerson(const std::string& personID) {
        MSTransportableControl& c = MSNet::getInstance()->getPersonControl();
        MSTransportable* p = c.get(personID);
        if (p == 0) {
            throw TraCIException("Person '" + personID + "' is not known");
        }
        return p;
    }

    std::string
        Person::getSingularVType(const std::string& personID) {
        return getPerson(personID)->getSingularType().getID();
    }
}


/****************************************************************************/