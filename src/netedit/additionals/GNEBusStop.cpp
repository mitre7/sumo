/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEBusStop.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Nov 2015
/// @version $Id$
///
// A lane area vehicles can halt at (GNE version)
/****************************************************************************/

// ===========================================================================
// included modules
// ===========================================================================
#include <config.h>

#include <string>
#include <iostream>
#include <utility>
#include <foreign/fontstash/fontstash.h>
#include <utils/geom/PositionVector.h>
#include <utils/common/RandHelper.h>
#include <utils/common/SUMOVehicleClass.h>
#include <utils/common/ToString.h>
#include <utils/geom/GeomHelper.h>
#include <utils/gui/windows/GUISUMOAbstractView.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUIIconSubSys.h>
#include <utils/gui/globjects/GUIGLObjectPopupMenu.h>
#include <utils/gui/div/GLHelper.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUITexturesHelper.h>
#include <utils/xml/SUMOSAXHandler.h>
#include <utils/common/MsgHandler.h>
#include <netedit/netelements/GNELane.h>
#include <netedit/netelements/GNEEdge.h>
#include <netedit/netelements/GNEJunction.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNENet.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNEViewNet.h>
#include <netedit/GNEViewParent.h>

#include "GNEBusStop.h"
#include "GNEAccess.h"

// ===========================================================================
// method definitions
// ===========================================================================

GNEBusStop::GNEBusStop(const std::string& id, GNELane* lane, GNEViewNet* viewNet, const std::string &startPos, const std::string &endPos, const std::string& name, const std::vector<std::string>& lines, bool friendlyPosition, bool blockMovement) :
    GNEStoppingPlace(id, viewNet, GLO_BUS_STOP, SUMO_TAG_BUS_STOP, lane, startPos, endPos, name, friendlyPosition, blockMovement),
    myLines(lines) {
}


GNEBusStop::~GNEBusStop() {}


void
GNEBusStop::updateGeometry() {
    // Get value of option "lefthand"
    double offsetSign = OptionsCont::getOptions().getBool("lefthand") ? -1 : 1;

    // Update common geometry of stopping place
    setStoppingPlaceGeometry(myLane->getParentEdge().getNBEdge()->getLaneWidth(myLane->getIndex())/2);

    // Obtain a copy of the shape
    PositionVector tmpShape = myShape;

    // Move shape to side
    tmpShape.move2side(1.5 * offsetSign);

    // Get position of the sign
    mySignPos = tmpShape.getLineCenter();

    // Set block icon position
    myBlockIconPosition = myShape.getLineCenter();

    // Set block icon rotation, and using their rotation for sign
    setBlockIconRotation(myLane);

    // Refresh element (neccesary to avoid grabbing problems)
    myViewNet->getNet()->refreshElement(this);
}


void
GNEBusStop::writeAdditional(OutputDevice& device) const {
    // Write parameters
    device.openTag(getTag());
    writeAttribute(device, SUMO_ATTR_ID);
    writeAttribute(device, SUMO_ATTR_LANE);
    writeAttribute(device, SUMO_ATTR_STARTPOS);
    writeAttribute(device, SUMO_ATTR_ENDPOS);
    if (myName.empty() == false) {
        writeAttribute(device, SUMO_ATTR_NAME);
    }
    writeAttribute(device, SUMO_ATTR_FRIENDLY_POS);
    if (myLines.size() > 0) {
        writeAttribute(device, SUMO_ATTR_LINES);
    }
    // Write Access
    for (auto i : myAdditionalChilds) {
        i->writeAdditional(device);
    }
    // Close tag
    device.closeTag();
}


const std::vector<std::string>&
GNEBusStop::getLines() const {
    return myLines;
}


std::string 
GNEBusStop::generateAccessID() const {
    int counter = 0;
    while (myViewNet->getNet()->getAdditional(SUMO_TAG_ACCESS, getID() + toString(SUMO_TAG_ACCESS) + toString(counter)) != nullptr) {
        counter++;
    }
    return (getID() + toString(SUMO_TAG_ACCESS) + toString(counter));
}

 
bool 
GNEBusStop::accessCanBeCreated(GNEEdge &edge) const {
    for (auto i : myAdditionalChilds) {
        if (dynamic_cast<GNEAccess*>(i)->getEdge().getID() == edge.getID()) {
            return false;
        }
    }
    return true;
}


void
GNEBusStop::drawGL(const GUIVisualizationSettings& s) const {
    // obtain circle resolution
    int circleResolution = getCircleResolution(s);
    // Obtain exaggeration of the draw
    const double exaggeration = s.addSize.getExaggeration(s);
    // Start drawing adding an gl identificator
    glPushName(getGlID());
    // Add a draw matrix
    glPushMatrix();
    // Start with the drawing of the area traslating matrix to origin
    glTranslated(0, 0, getType());
    // Set color of the base
    if (isAttributeCarrierSelected()) {
        GLHelper::setColor(myViewNet->getNet()->selectedAdditionalColor);
    } else {
        GLHelper::setColor(s.SUMO_color_busStop);
    }
    // Draw the area using shape, shapeRotations, shapeLengths and value of exaggeration
    GLHelper::drawBoxLines(myShape, myShapeRotations, myShapeLengths, exaggeration);
    // Check if the distance is enought to draw details and if is being drawn for selecting
    if(s.drawForSelecting) {
        // only draw circle depending of distance between sign and mouse cursor
        if(myViewNet->getPositionInformation().distanceSquaredTo(mySignPos) <= (myCircleWidthSquared + 2)) {
            // Add a draw matrix for details
            glPushMatrix();
            // Start drawing sign traslating matrix to signal position
            glTranslated(mySignPos.x(), mySignPos.y(), 0);
            // scale matrix depending of the exaggeration
            glScaled(exaggeration, exaggeration, 1);
            // set color
            GLHelper::setColor(s.SUMO_color_busStop);
            // Draw circle
            GLHelper::drawFilledCircle(myCircleWidth, circleResolution);
            // pop draw matrix
            glPopMatrix();
        }
    } else if (s.scale * exaggeration >= 10) {
        // draw lines between BusStops and Acces
        for (auto i : myAdditionalChilds) {
            GLHelper::drawBoxLine(i->getShape()[0], RAD2DEG(mySignPos.angleTo2D(i->getShape()[0])) - 90, mySignPos.distanceTo2D(i->getShape()[0]), .05);
        }
        // Add a draw matrix for details
        glPushMatrix();
        // Iterate over every line
        for (int i = 0; i < (int)myLines.size(); ++i) {
            // push a new matrix for every line
            glPushMatrix();
            // Rotate and traslaste
            glTranslated(mySignPos.x(), mySignPos.y(), 0);
            glRotated(-1 * myBlockIconRotation, 0, 0, 1);
            // draw line with a color depending of the selection status
            if (isAttributeCarrierSelected()) {
                GLHelper::drawText(myLines[i].c_str(), Position(1.2, (double)i), .1, 1.f, myViewNet->getNet()->selectionColor, 0, FONS_ALIGN_LEFT);
            } else {
                GLHelper::drawText(myLines[i].c_str(), Position(1.2, (double)i), .1, 1.f, s.SUMO_color_busStop, 0, FONS_ALIGN_LEFT);
            }
            // pop matrix for every line
            glPopMatrix();
        }
        // Start drawing sign traslating matrix to signal position
        glTranslated(mySignPos.x(), mySignPos.y(), 0);
        // scale matrix depending of the exaggeration
        glScaled(exaggeration, exaggeration, 1);
        // Set color of the externe circle
        if (isAttributeCarrierSelected()) {
            GLHelper::setColor(myViewNet->getNet()->selectedAdditionalColor);
        } else {
            GLHelper::setColor(s.SUMO_color_busStop);
        }
        // Draw circle
        GLHelper::drawFilledCircle(myCircleWidth, circleResolution);
        // Traslate to front
        glTranslated(0, 0, .1);
        // Set color of the interne circle
        if (isAttributeCarrierSelected()) {
            GLHelper::setColor(myViewNet->getNet()->selectionColor);
        } else {
            GLHelper::setColor(s.SUMO_color_busStop_sign);
        }
        // draw another circle in the same position, but a little bit more small
        GLHelper::drawFilledCircle(myCircleInWidth, circleResolution);
        // If the scale * exageration is equal or more than 4.5, draw H
        if (s.scale * exaggeration >= 4.5) {
            if (isAttributeCarrierSelected()) {
                GLHelper::drawText("H", Position(), .1, myCircleInText, myViewNet->getNet()->selectedAdditionalColor, myBlockIconRotation);
            } else {
                GLHelper::drawText("H", Position(), .1, myCircleInText, s.SUMO_color_busStop, myBlockIconRotation);
            }
        }
        // pop draw matrix
        glPopMatrix();
        // Show Lock icon depending of the Edit mode
        drawLockIcon();
    }
    // pop draw matrix
    glPopMatrix();
    // Draw name if isn't being drawn for selecting
    drawName(getCenteringBoundary().getCenter(), s.scale, s.addName);
    if (s.addFullName.show && (myName != "") && !s.drawForSelecting) {
        GLHelper::drawText(myName, mySignPos, GLO_MAX - getType(), s.addFullName.scaledSize(s.scale), s.addFullName.color, myBlockIconRotation);
    }
    // Pop name
    glPopName();
}


std::string
GNEBusStop::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_ID:
            return getAdditionalID();
        case SUMO_ATTR_LANE:
            return myLane->getID();
        case SUMO_ATTR_STARTPOS:
            return toString(myStartPosition);
        case SUMO_ATTR_ENDPOS:
            return myEndPosition;
        case SUMO_ATTR_NAME:
            return myName;
        case SUMO_ATTR_FRIENDLY_POS:
            return toString(myFriendlyPosition);
        case SUMO_ATTR_LINES:
            return joinToString(myLines, " ");
        case GNE_ATTR_BLOCK_MOVEMENT:
            return toString(myBlockMovement);
        case GNE_ATTR_SELECTED:
            return toString(isAttributeCarrierSelected());
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEBusStop::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    if (value == getAttribute(key)) {
        return; //avoid needless changes, later logic relies on the fact that attributes have changed
    }
    switch (key) {
        case SUMO_ATTR_ID: {
            // change ID of BusStop
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            // Change Ids of all Acces childs
            for (auto i : myAdditionalChilds) {
                i->setAttribute(SUMO_ATTR_ID, generateAccessID(), undoList);
            }
            break;
        }
        case SUMO_ATTR_LANE:
        case SUMO_ATTR_STARTPOS:
        case SUMO_ATTR_ENDPOS:
        case SUMO_ATTR_NAME:
        case SUMO_ATTR_FRIENDLY_POS:
        case SUMO_ATTR_LINES:
        case GNE_ATTR_BLOCK_MOVEMENT:
        case GNE_ATTR_SELECTED:
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


bool
GNEBusStop::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            return isValidAdditionalID(value);
        case SUMO_ATTR_LANE:
            if (myViewNet->getNet()->retrieveLane(value, false) != nullptr) {
                return true;
            } else {
                return false;
            }
        case SUMO_ATTR_STARTPOS:
            if(value.empty()) {
                return true;
            } else {
                if (canParse<double>(value)) {
                    if(canParse<double>(myEndPosition)) {
                        // Check that new start Position is smaller that end position
                        return (parse<double>(value) < parse<double>(myEndPosition));
                    } else {
                        return true;
                    }
                } else {
                    return false;
                }
            }
        case SUMO_ATTR_ENDPOS:
            if(value.empty()) {
                return true;
            } else {
                if (canParse<double>(value)) {
                    if(canParse<double>(myStartPosition)) {
                        // Check that new start Position is smaller that end position
                        return (parse<double>(myStartPosition) < parse<double>(value));
                    } else {
                        return true;
                    }
                } else {
                    return false;
                }
            }
        case SUMO_ATTR_NAME:
            return true;
        case SUMO_ATTR_FRIENDLY_POS:
            return canParse<bool>(value);
        case SUMO_ATTR_LINES:
            return canParse<std::vector<std::string> >(value);
        case GNE_ATTR_BLOCK_MOVEMENT:
            return canParse<bool>(value);
        case GNE_ATTR_SELECTED:
            return canParse<bool>(value);
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}

// ===========================================================================
// private
// ===========================================================================

void
GNEBusStop::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            changeAdditionalID(value);
            break;
        case SUMO_ATTR_LANE:
            myLane = changeLane(myLane, value);
            break;
        case SUMO_ATTR_STARTPOS:
            myStartPosition = value;
            break;
        case SUMO_ATTR_ENDPOS:
            myEndPosition = value;
            break;
        case SUMO_ATTR_NAME:
            myName = value;
            break;
        case SUMO_ATTR_FRIENDLY_POS:
            myFriendlyPosition = parse<bool>(value);
            break;
        case SUMO_ATTR_LINES:
            myLines = GNEAttributeCarrier::parse<std::vector<std::string> >(value);
            break;
        case GNE_ATTR_BLOCK_MOVEMENT:
            myBlockMovement = parse<bool>(value);
            break;
        case GNE_ATTR_SELECTED:
            if(parse<bool>(value)) {
                selectAttributeCarrier();
            } else {
                unselectAttributeCarrier();
            }
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
    // After setting attribute always update Geometry
    updateGeometry();
}

void 
GNEBusStop::addAccess(GNELane* lane, double pos, bool friendlyPos, double length) {
    /*
    myAccess.push_back(Access(lane, pos, friendlyPos, length));
    */
}

/****************************************************************************/
