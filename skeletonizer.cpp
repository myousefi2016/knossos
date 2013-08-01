/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

/*
#include <libxml/globals.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
*/

#include <time.h>
#include "skeletonizer.h"
#include "knossos-global.h"
#include "renderer.h"
#include "knossos.h"
#include "client.h"
#include "viewer.h"
#include "mainwindow.h"
#include "functions.h"

extern stateInfo *state;


Skeletonizer::Skeletonizer(QObject *parent) : QObject(parent) {

    idleTimeSession = 0;
    lockRadius = 100;
    lockPositions = false;
    strncpy(onCommentLock, "seed", 1024);
    branchpointUnresolved = false;
    autoFilenameIncrementBool = true;
    autoSaveBool = true;
    autoSaveInterval = 5;
    skeletonTime = 0;
    skeletonTimeCorrection = state->time.elapsed();
    definedSkeletonVpView = 0;

    //This number is currently arbitrary, but high values ensure a good performance
    state->skeletonState->skeletonDCnumber = 8000;
    state->skeletonState->workMode = ON_CLICK_DRAG;

    /* @CMP
    if(state->skeletonState->skeletonDCnumber != tempConfig->skeletonState->skeletonDCnumber)
        state->skeletonState->skeletonDCnumber = tempConfig->skeletonState->skeletonDCnumber;
    */

    updateSkeletonState();

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);

    /*@todo
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.");
        return false;
    }*/

    state->skeletonState->skeletonRevision = 0;

    state->skeletonState->nodeCounter = newDynArray(1048576);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(1048576);

    // Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->treeElements = 0;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    state->skeletonState->mergeOnLoadFlag = 0;
    state->skeletonState->segRadiusToNodeRadius = 0.5;
    state->skeletonState->autoFilenameIncrementBool = true;
    state->skeletonState->greatestNodeID = 0;

    state->skeletonState->showXYplane = false;
    state->skeletonState->showXZplane = false;
    state->skeletonState->showYZplane = false;
    state->skeletonState->showNodeIDs = false;
    state->skeletonState->highlightActiveTree = false;
    state->skeletonState->rotateAroundActiveNode = false;
    state->skeletonState->showIntersections = false;

    state->skeletonState->displayListSkeletonSkeletonizerVP = 0;
    state->skeletonState->displayListView = 0;
    state->skeletonState->displayListDataset = 0;

    state->skeletonState->defaultNodeRadius = 1.5;
    state->skeletonState->overrideNodeRadiusBool = false;
    state->skeletonState->overrideNodeRadiusVal = 1.;

    state->skeletonState->currentComment = NULL;

    state->skeletonState->lastSaveTicks = 0;
    state->skeletonState->autoSaveInterval = 5;


    state->skeletonState->skeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->skeletonFile, '\0', 8192 * sizeof(char));
    setDefaultSkelFileName();

    state->skeletonState->prevSkeletonFile = (char*) malloc(8192 * sizeof(char));
    memset(state->skeletonState->prevSkeletonFile, '\0', 8192 * sizeof(char));

    state->skeletonState->commentBuffer = (char*) malloc(10240 * sizeof(char));
    memset(state->skeletonState->commentBuffer, '\0', 10240 * sizeof(char));

    state->skeletonState->searchStrBuffer = (char*) malloc(2048 * sizeof(char));
    memset(state->skeletonState->searchStrBuffer, '\0', 2048 * sizeof(char));

    state->skeletonState->undoList = (cmdList *)malloc(sizeof(struct cmdList));
    memset(state->skeletonState->undoList, '\0', sizeof(struct cmdList));
    state->skeletonState->undoList->cmdCount = 0;
    state->skeletonState->undoList->firstCmd = NULL;
    state->skeletonState->undoList->lastCmd = NULL;

    state->skeletonState->redoList = (cmdList *)malloc(sizeof(struct cmdList));
    memset(state->skeletonState->redoList, '\0', sizeof(struct cmdList));
    state->skeletonState->redoList->cmdCount = 0;
    state->skeletonState->redoList->firstCmd = NULL;
    state->skeletonState->redoList->lastCmd = NULL;

    state->skeletonState->saveCnt = 0;

    if((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z))
        state->skeletonState->volBoundary = state->boundary.x * 2;
    if((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.y * 2;
    if((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y))
        state->skeletonState->volBoundary = state->boundary.x * 2;

    state->skeletonState->viewChanged = true;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->datasetChanged = true;
    state->skeletonState->skeletonSliceVPchanged = true;
    state->skeletonState->unsavedChanges = false;
    state->skeletonState->askingPopBranchConfirmation = false;

}

nodeListElement *Skeletonizer::addNodeListElement(
              int nodeID,
              float radius,
              nodeListElement **currentNode,
              Coordinate *position,
              int inMag) {

    nodeListElement *newElement = NULL;

    /*
     * This skeleton modifying function does not lock the skeleton and therefore
     * has to be called from functions that do lock and NEVER directly.
     *
     */

    newElement = (nodeListElement*) malloc(sizeof(struct nodeListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new nodeListElement.");
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct nodeListElement));

    if((*currentNode) == NULL) {
        // Requested to add a node to a list that hasn't yet been started.
        *currentNode = newElement;
        newElement->previous = NULL;
        newElement->next = NULL;
    }
    else {
        newElement->previous = *currentNode;
        newElement->next = (*currentNode)->next;
        (*currentNode)->next = newElement;
        if(newElement->next) {
            newElement->next->previous = newElement;
        }
    }

    SET_COORDINATE(newElement->position, position->x, position->y, position->z);
    //Assign radius
    newElement->radius = radius;
    //Set node ID. This ID is unique in every tree list (there should only exist 1 tree list, see initSkeletonizer()).
    //Take the provided nodeID.

    newElement->numSegs = 0;
    newElement->nodeID = nodeID;
    newElement->isBranchNode = false;
    newElement->createdInMag = inMag;

    return newElement;
}

segmentListElement *Skeletonizer::addSegmentListElement (segmentListElement **currentSegment,
    nodeListElement *sourceNode,
    nodeListElement *targetNode) {
    segmentListElement *newElement = NULL;

    /*
    * This skeleton modifying function does not lock the skeleton and therefore
    * has to be called from functions that do lock and NEVER directly.
    */

    newElement = (segmentListElement*) malloc(sizeof(struct segmentListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new segmentListElement.");
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct segmentListElement));
    if(*currentSegment == NULL) {
        // Requested to start a new list
        *currentSegment = newElement;
        newElement->previous = NULL;
        newElement->next = NULL;
    }
    else {
        newElement->previous = *currentSegment;
        newElement->next = (*currentSegment)->next;
        (*currentSegment)->next = newElement;
        if(newElement->next)
            newElement->next->previous = newElement;
    }

    //Valid segment
    newElement->flag = SEGMENT_FORWARD;
    newElement->source = sourceNode;
    newElement->target = targetNode;
    return newElement;
}


treeListElement* Skeletonizer::findTreeByTreeID(int treeID) {
    treeListElement *currentTree;

    currentTree = state->skeletonState->firstTree;

    while(currentTree) {
        if(currentTree->treeID == treeID) {
            return currentTree;
        }
        currentTree = currentTree->next;
    }
    return NULL;
}

bool Skeletonizer::addNodeToSkeletonStruct(nodeListElement *node) {
    skeletonDC *currentSkeletonDC;
    skeletonDCnode *currentNewSkeletonDCnode;
    Coordinate currentMagPos;

    currentMagPos.x = node->position.x / state->magnification;
    currentMagPos.y = node->position.y / state->magnification;
    currentMagPos.z = node->position.z / state->magnification;

    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                    Coordinate::Px2DcCoord(currentMagPos));

    if(currentSkeletonDC == HT_FAILURE) {
        currentSkeletonDC = (skeletonDC*) malloc(sizeof(skeletonDC));
        memset(currentSkeletonDC, '\0', sizeof(skeletonDC));
        Hashtable::ht_put(state->skeletonState->skeletonDCs,
               Coordinate::Px2DcCoord(currentMagPos),
               (Byte *)currentSkeletonDC);
    }

    currentNewSkeletonDCnode = (skeletonDCnode*) malloc(sizeof (skeletonDCnode));
    memset(currentNewSkeletonDCnode, '\0', sizeof(skeletonDCnode));

    if(!currentSkeletonDC->firstSkeletonDCnode)
        currentSkeletonDC->firstSkeletonDCnode = currentNewSkeletonDCnode;
    else {
        currentNewSkeletonDCnode->next = currentSkeletonDC->firstSkeletonDCnode;
        currentSkeletonDC->firstSkeletonDCnode = currentNewSkeletonDCnode;
    }
    currentNewSkeletonDCnode->node = node;

    return true;
}

bool Skeletonizer::addSegmentToSkeletonStruct(segmentListElement *segment) {
    int i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *currentNewSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;

    if(segment) {
        if(segment->flag == 2)
            return false;
    }
    else
        return false;

    curMagTargetPos.x = segment->target->position.x;
    curMagTargetPos.y = segment->target->position.y;
    curMagTargetPos.z = segment->target->position.z;
    curMagSourcePos.x = segment->source->position.x;
    curMagSourcePos.y = segment->source->position.y;
    curMagSourcePos.z = segment->source->position.z;

    segVector.x = (float)(curMagTargetPos.x - curMagSourcePos.x);
    segVector.y = (float)(curMagTargetPos.y - curMagSourcePos.y);
    segVector.z = (float)(curMagTargetPos.z - curMagSourcePos.z);

    segVectorLength = euclidicNorm(&segVector);
    normalizeVector(&segVector);

    SET_COORDINATE(lastSegVectorPoint,
                   curMagSourcePos.x,
                   curMagSourcePos.y,
                   curMagSourcePos.z);

    //Add the segment pointer to the first skeleton DC
    //Is there a skeleton DC for the first skeleton DC to which a pointer has to be added?
    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                    Coordinate::Px2DcCoord(curMagSourcePos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return false;
    }
    currentNewSkeletonDCsegment = (skeletonDCsegment*)malloc(sizeof (skeletonDCsegment));
    memset(currentNewSkeletonDCsegment, '\0', sizeof(skeletonDCsegment));

    if(!currentSkeletonDC->firstSkeletonDCsegment)
        currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
    else {
        currentNewSkeletonDCsegment->next = currentSkeletonDC->firstSkeletonDCsegment;
        currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
    }
    currentNewSkeletonDCsegment->segment = segment;

    //We walk along the entire segment and add pointers to the segment in all skeleton DCs we are passing.
    for(i = 0; i <= ((int)segVectorLength); i++) {
        currentSegVectorPoint.x = curMagSourcePos.x + roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + roundFloat(((float)i) * segVector.z);

        if(!COMPARE_COORDINATE(Coordinate::Px2DcCoord(lastSegVectorPoint), Coordinate::Px2DcCoord(currentSegVectorPoint))) {
            //We crossed a skeleton DC boundary, now we have to add a pointer to the segment inside the skeleton DC
            currentSkeletonDC = (struct skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs, Coordinate::Px2DcCoord(currentSegVectorPoint));
            if(currentSkeletonDC == HT_FAILURE) {
                currentSkeletonDC = (skeletonDC*)malloc(sizeof(skeletonDC));
                memset(currentSkeletonDC, '\0', sizeof(skeletonDC));
                Hashtable::ht_put(state->skeletonState->skeletonDCs, Coordinate::Px2DcCoord(currentSegVectorPoint), (Byte *)currentSkeletonDC);
            }
            currentNewSkeletonDCsegment = (skeletonDCsegment*)malloc(sizeof (skeletonDCsegment));
            memset(currentNewSkeletonDCsegment, '\0', sizeof(skeletonDCsegment));

            if(!currentSkeletonDC->firstSkeletonDCsegment)
                currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
            else {
                currentNewSkeletonDCsegment->next = currentSkeletonDC->firstSkeletonDCsegment;
                currentSkeletonDC->firstSkeletonDCsegment = currentNewSkeletonDCsegment;
            }
            currentNewSkeletonDCsegment->segment = segment;

            lastSegVectorPoint.x = currentSegVectorPoint.x;
            lastSegVectorPoint.y = currentSegVectorPoint.y;
            lastSegVectorPoint.z = currentSegVectorPoint.z;
        }
    }

    return true;
}

bool Skeletonizer::delNodeFromSkeletonStruct(nodeListElement *node) {
    skeletonDC *currentSkeletonDC;
    skeletonDCnode *currentSkeletonDCnode, *lastSkeletonDCnode = NULL;
    Coordinate curMagPos;

    curMagPos.x = node->position.x / state->magnification;
    curMagPos.y = node->position.y / state->magnification;
    curMagPos.z = node->position.z / state->magnification;

    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                             Coordinate::Px2DcCoord(curMagPos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return false;
    }

    currentSkeletonDCnode = currentSkeletonDC->firstSkeletonDCnode;

    while(currentSkeletonDCnode) {
        //We found the node we want to delete..
        if(currentSkeletonDCnode->node == node) {
            if(lastSkeletonDCnode != NULL)
                lastSkeletonDCnode->next = currentSkeletonDCnode->next;
            else
                currentSkeletonDC->firstSkeletonDCnode = currentSkeletonDCnode->next;

            free(currentSkeletonDCnode);

            break;
        }

        lastSkeletonDCnode = currentSkeletonDCnode;
        currentSkeletonDCnode = currentSkeletonDCnode->next;
    }

    return true;
}

bool Skeletonizer::delSegmentFromSkeletonStruct(segmentListElement *segment) {
    int i;
    skeletonDC *currentSkeletonDC;
    skeletonDCsegment *lastSkeletonDCsegment, *currentSkeletonDCsegment;
    float segVectorLength = 0.;
    floatCoordinate segVector;
    Coordinate currentSegVectorPoint, lastSegVectorPoint;
    Coordinate curMagTargetPos, curMagSourcePos;

    if(segment) {
        if(segment->flag == 2) {
            return false;
        }
    }
    else {
        return false;
    }
    curMagTargetPos.x = segment->target->position.x / state->magnification;
    curMagTargetPos.y = segment->target->position.y / state->magnification;
    curMagTargetPos.z = segment->target->position.z / state->magnification;
    curMagSourcePos.x = segment->source->position.x / state->magnification;
    curMagSourcePos.y = segment->source->position.y / state->magnification;
    curMagSourcePos.z = segment->source->position.z / state->magnification;

    segVector.x = (float)(curMagTargetPos.x - curMagSourcePos.x);
    segVector.y = (float)(curMagTargetPos.y - curMagSourcePos.y);
    segVector.z = (float)(curMagTargetPos.z - curMagSourcePos.z);

    segVectorLength = euclidicNorm(&segVector);
    normalizeVector(&segVector);

    SET_COORDINATE(lastSegVectorPoint,
                   curMagSourcePos.x,
                   curMagSourcePos.y,
                   curMagSourcePos.z);

    //Is there a skeleton DC for the first skeleton DC from which the segment has to be removed?
    currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                        Coordinate::Px2DcCoord(curMagSourcePos));
    if(currentSkeletonDC == HT_FAILURE) {
        //A skeleton DC is missing
        LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
        return false;
    }

    lastSkeletonDCsegment = NULL;
    currentSkeletonDCsegment = currentSkeletonDC->firstSkeletonDCsegment;
    while(currentSkeletonDCsegment) {
        //We found the segment we want to delete..
        if(currentSkeletonDCsegment->segment == segment) {
            //Our segment is the first segment
            if(lastSkeletonDCsegment == NULL) {
                currentSkeletonDC->firstSkeletonDCsegment = currentSkeletonDCsegment->next;
            }
            else {
                lastSkeletonDCsegment->next = currentSkeletonDCsegment->next;
            }
            free(currentSkeletonDCsegment);
            break;
        }

        lastSkeletonDCsegment = currentSkeletonDCsegment;
        currentSkeletonDCsegment = currentSkeletonDCsegment->next;
    }

    //We walk along the entire segment and delete pointers to the segment in all skeleton DCs we are passing.
    for(i = 0; i <= ((int)segVectorLength); i++) {
        currentSegVectorPoint.x = curMagSourcePos.x + roundFloat(((float)i) * segVector.x);
        currentSegVectorPoint.y = curMagSourcePos.y + roundFloat(((float)i) * segVector.y);
        currentSegVectorPoint.z = curMagSourcePos.z + roundFloat(((float)i) * segVector.z);

        if(!COMPARE_COORDINATE(Coordinate::Px2DcCoord(lastSegVectorPoint), Coordinate::Px2DcCoord(currentSegVectorPoint))) {
            //We crossed a skeleton DC boundary, now we have to delete the pointer to the segment inside the skeleton DC
            currentSkeletonDC = (skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs,
                                                     Coordinate::Px2DcCoord(currentSegVectorPoint));
            if(currentSkeletonDC == HT_FAILURE) {
                //A skeleton DC is missing
                LOG("Error: a skeleton DC is missing that should be available. You may encounter other errors.");
                return false;
            }

            lastSkeletonDCsegment = NULL;
            currentSkeletonDCsegment = currentSkeletonDC->firstSkeletonDCsegment;
            while(currentSkeletonDCsegment) {
                //We found the segment we want to delete..
                if(currentSkeletonDCsegment->segment == segment) {
                    //Our segment is the first segment
                    if(lastSkeletonDCsegment == NULL) {
                        currentSkeletonDC->firstSkeletonDCsegment = currentSkeletonDCsegment->next;
                    }
                    else {
                        lastSkeletonDCsegment->next = currentSkeletonDCsegment->next;
                    }
                    free(currentSkeletonDCsegment);
                    break;
                }

                lastSkeletonDCsegment = currentSkeletonDCsegment;
                currentSkeletonDCsegment = currentSkeletonDCsegment->next;
            }

            lastSegVectorPoint.x = currentSegVectorPoint.x;
            lastSegVectorPoint.y = currentSegVectorPoint.y;
            lastSegVectorPoint.z = currentSegVectorPoint.z;
        }
    }

    return true;
}

/** @todo cleanup */
void Skeletonizer::WRAP_popBranchNode() {
    /*
    Skeletonizer::popBranchNode(CHANGE_MANUAL);
    state->skeletonState->askingPopBranchConfirmation = false;
    */
}

void Skeletonizer::popBranchNodeCanceled() {
    state->skeletonState->askingPopBranchConfirmation = false;
}


bool Skeletonizer::UI_addSkeletonNode(Coordinate *clickedCoordinate, Byte VPtype) {
    int addedNodeID;

    color4F treeCol;
    /* -1 causes new color assignment */
    treeCol.r = -1.;
    treeCol.g = -1.;
    treeCol.b = -1.;
    treeCol.a = 1.;

    if(!state->skeletonState->activeTree)
        addTreeListElement(true, CHANGE_MANUAL, 0, treeCol);

    addedNodeID = addNode(CHANGE_MANUAL,
                          0,
                          state->skeletonState->defaultNodeRadius,
                          state->skeletonState->activeTree->treeID,
                          clickedCoordinate,
                          VPtype,
                          state->magnification,
                          -1,
                          true);
    if(!addedNodeID) {
        LOG("Error: Could not add new node!");
        return false;
    }

    setActiveNode(CHANGE_MANUAL, NULL, addedNodeID);

    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, true, true, NULL, addedNodeID);
        addComment(CHANGE_MANUAL, "First Node", NULL, addedNodeID);
    }

    emit idleTimeSignal();

    //Remote::checkIdleTime();

    return true;
}

uint Skeletonizer::addSkeletonNodeAndLinkWithActive(Coordinate *clickedCoordinate, Byte VPtype, int makeNodeActive) {
    int targetNodeID;

    if(!state->skeletonState->activeNode) {
        LOG("Please create a node before trying to link nodes.");
        state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
        return false;
    }

    //Add a new node at the target position first.
    targetNodeID = addNode(CHANGE_MANUAL,
                           0,
                           state->skeletonState->defaultNodeRadius,
                           state->skeletonState->activeTree->treeID,
                           clickedCoordinate,
                           VPtype,
                           state->magnification,
                           -1,
                           true);
    if(!targetNodeID) {
        LOG("Could not add new node while trying to add node and link with active node!");
        return false;
    }

    addSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, targetNodeID);

    if(makeNodeActive) {
        setActiveNode(CHANGE_MANUAL, NULL, targetNodeID);
    }
    if((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                               state->skeletonState->activeTree->treeID) == 1) {
        /* First node in this tree */

        pushBranchNode(CHANGE_MANUAL, true, true, NULL, targetNodeID);
        addComment(CHANGE_MANUAL, "First Node", NULL, targetNodeID);

    }

    return targetNodeID;
}

bool Skeletonizer::updateSkeletonState() {
    //Time to auto-save
    /*qDebug() << state->time.elapsed();
    qDebug() << state->skeletonState->lastSaveTicks;
    qDebug() << ((state->time.elapsed() - state->skeletonState->lastSaveTicks) / 60000.0);
    qDebug() << state->skeletonState->autoSaveInterval << " min"; */

    if(state->skeletonState->autoSaveBool || state->clientState->saveMaster) {
        if(state->skeletonState->autoSaveInterval) {
            if((state->time.elapsed() - state->skeletonState->lastSaveTicks) / 60000.0 >= state->skeletonState->autoSaveInterval) {
                state->skeletonState->lastSaveTicks = state->time.elapsed();                

                emit saveSkeletonSignal(true);
            }
        }
    }

    /* @CMP
    if(state->skeletonState->skeletonDCnumber != tempConfig->skeletonState->skeletonDCnumber) {
        state->skeletonState->skeletonDCnumber = tempConfig->skeletonState->skeletonDCnumber;
    }
    if(state->skeletonState->workMode != tempConfig->skeletonState->workMode) {
        setSkeletonWorkMode(CHANGE_MANUAL, tempConfig->skeletonState->workMode);
    }
    */
    setSkeletonWorkMode(CHANGE_MANUAL, state->skeletonState->workMode);
    return true;
}

bool Skeletonizer::nextCommentlessNode() { return true; }

bool Skeletonizer::previousCommentlessNode() {
    struct nodeListElement *currNode;
    currNode = state->skeletonState->activeNode->previous;
    if(currNode && (!currNode->comment)) {
        LOG("yes");
        setActiveNode(CHANGE_MANUAL,
                          currNode,
                          0);
        jumpToActiveNode();
    }
    return true;
}

bool Skeletonizer::updateSkeletonFileName(int targetRevision, int increment, char *filename) {
    int extensionDelim = -1, countDelim = -1;
    char betweenDots[8192];
    char count[8192];
    char origFilename[8192], skeletonFileBase[8192];
    int i, j;

    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    memset(skeletonFileBase, '\0', 8192);
    memset(origFilename, '\0', 8192);
    strncpy(origFilename, filename, 8192 - 1);

    if(increment) {
        for(i = 8192 - 1; i >= 0; i--) {
            if(filename[i] == '.') {
                extensionDelim = i;
                break;
            }
        }

        for(i--; i >= 0; i--) {
            if(filename[i] == '.') {
                //check if string between dots represents a number
                strncpy(betweenDots, &filename[i+1], extensionDelim - i-1);
                for(j = 0; j < extensionDelim - i - 1; j++) {
                    if(atoi(&betweenDots[j]) == 0 && betweenDots[j] != '0') {
                        goto noCounter;
                    }
                }
                //string between dots is a number
                countDelim = i;
                break;
            }
        }

        noCounter:
        if(countDelim > -1) {
            strncpy(skeletonFileBase,
                    filename,
                    countDelim);
        }
        else if(extensionDelim > -1){
            strncpy(skeletonFileBase,
                    filename,
                    extensionDelim);
        }
        else {
            strncpy(skeletonFileBase,
                    filename,
                    8192 - 1);
        }

        if(countDelim && extensionDelim) {
            strncpy(count, &filename[countDelim + 1], extensionDelim - countDelim);
            state->skeletonState->saveCnt = atoi(count);
            state->skeletonState->saveCnt++;
        }
        else {
            state->skeletonState->saveCnt = 0;
        }

        sprintf(state->skeletonState->skeletonFile, "%s.%.3d.nml\0",
                skeletonFileBase,
                state->skeletonState->saveCnt);
    }

    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_SKELETONFILENAME, increment, origFilename))
            Client::skeletonSyncBroken();
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

int Skeletonizer::saveSkeleton() {

    treeListElement *currentTree = NULL;
    nodeListElement *currentNode = NULL;
    PTRSIZEINT currentBranchPointID;
    segmentListElement *currentSegment = NULL;
    commentListElement *currentComment = NULL;
    stack *reverseBranchStack = NULL, *tempReverseStack = NULL;
    int r;
    /*
    int time;
    xmlChar attrString[128];

    xmlDocPtr xmlDocument;
    xmlNodePtr thingsXMLNode, nodesXMLNode, edgesXMLNode, currentXMLNode,
               paramsXMLNode, branchesXMLNode, commentsXMLNode;

    memset(attrString, '\0', 128 * sizeof(xmlChar));

     //  This function should always be called through UI_saveSkeleton
     // for proper error and file name display to the user.


     // We need to do this to be able to save the branch point stack to a file
     //and still have the branch points available to the user afterwards.

    reverseBranchStack = newStack(2048);
    tempReverseStack = newStack(2048);
    while((currentBranchPointID =
          (PTRSIZEINT)popStack(state->skeletonState->branchStack))) {
        pushStack(reverseBranchStack, (void *)currentBranchPointID);
        pushStack(tempReverseStack, (void *)currentBranchPointID);
    }

    while((currentBranchPointID =
          (PTRSIZEINT)popStack(tempReverseStack))) {
        currentNode = (struct nodeListElement *)findNodeByNodeID(currentBranchPointID);
        pushBranchNode(CHANGE_MANUAL, false, false, currentNode, 0);
    }

    xmlDocument = xmlNewDoc(BAD_CAST"1.0");
    thingsXMLNode = xmlNewDocNode(xmlDocument, NULL, BAD_CAST"things", NULL);
    xmlDocSetRootElement(xmlDocument, thingsXMLNode);

    paramsXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"parameters", NULL);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"experiment", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", state->name);
    xmlNewProp(currentXMLNode, BAD_CAST"name", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"lastsavedin", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", KVERSION);
    xmlNewProp(currentXMLNode, BAD_CAST"version", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"createdin", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%s", state->skeletonState->skeletonCreatedInVersion);
    xmlNewProp(currentXMLNode, BAD_CAST"version", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"scale", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.x / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.y / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->scale.z / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"offset", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.x / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.y / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->offset.z / state->magnification);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"time", NULL);
    xmlStrPrintf(attrString,
                 128,
                 BAD_CAST"%d",
                 state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed(),
                 xorInt(time));
    xmlNewProp(currentXMLNode, BAD_CAST"ms", attrString);
    memset(attrString, '\0', 128);

    if(state->skeletonState->activeNode) {
        currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"activeNode", NULL);
        xmlStrPrintf(attrString,
                     128,
                     BAD_CAST"%d",
                     state->skeletonState->activeNode->nodeID);
        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
    }

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"editPosition", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.x + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.y + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", state->viewerState->currentPosition.z + 1);
    xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
    memset(attrString, '\0', 128);
    {
        int j = 0;
        char element [8];
        currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"skeletonVPState", NULL);
        for (j = 0; j < 16; j++){
            sprintf (element, "E%d", j);
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->skeletonVpModelView[j]);
            xmlNewProp(currentXMLNode, BAD_CAST(element), attrString);
            memset(attrString, '\0', 128);
        }
    }
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->translateX);
    xmlNewProp(currentXMLNode, BAD_CAST"translateX", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->translateY);
    xmlNewProp(currentXMLNode, BAD_CAST"translateY", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"vpSettingsZoom", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"XYPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"XZPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"YZPlane", attrString);
    memset(attrString, '\0', 128);
    xmlStrPrintf(attrString, 128, BAD_CAST"%f", state->skeletonState->zoomLevel);
    xmlNewProp(currentXMLNode, BAD_CAST"SkelVP", attrString);
    memset(attrString, '\0', 128);

    currentXMLNode = xmlNewTextChild(paramsXMLNode, NULL, BAD_CAST"idleTime", NULL);
    xmlStrPrintf(attrString, 128, BAD_CAST"%d", xorInt(state->skeletonState->idleTime));
    xmlNewProp(currentXMLNode, BAD_CAST"ms", attrString);
    memset(attrString, '\0', 128);

    currentTree = state->skeletonState->firstTree;
    if((currentTree == NULL) && (state->skeletonState->currentComment == NULL)) {
        return false; //No Skeleton to save
    }

    while(currentTree) {
        // Every "thing" has associated nodes and edges.

        currentXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"thing", NULL);
        xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentTree->treeID);
        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
        if(currentTree->colorSetManually) {
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.r);
            xmlNewProp(currentXMLNode, BAD_CAST"color.r", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.g);
            xmlNewProp(currentXMLNode, BAD_CAST"color.g", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.b);
            xmlNewProp(currentXMLNode, BAD_CAST"color.b", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentTree->color.a);
            xmlNewProp(currentXMLNode, BAD_CAST"color.a", attrString);
            memset(attrString, '\0', 128);
        }
        else {
            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.r", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.g", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", -1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.b", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", 1.);
            xmlNewProp(currentXMLNode, BAD_CAST"color.a", attrString);
            memset(attrString, '\0', 128);
        }
        memset(attrString, '\0', 128);
        xmlNewProp(currentXMLNode, BAD_CAST"comment", (xmlChar*)currentTree->comment);

        nodesXMLNode = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"nodes", NULL);
        edgesXMLNode = xmlNewTextChild(currentXMLNode, NULL, BAD_CAST"edges", NULL);

        currentNode = currentTree->firstNode;
        while(currentNode) {
            currentXMLNode = xmlNewTextChild(nodesXMLNode, NULL, BAD_CAST"node", NULL);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->nodeID);
            xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%f", currentNode->radius);
            xmlNewProp(currentXMLNode, BAD_CAST"radius", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.x + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"x", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.y + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"y", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->position.z + 1);
            xmlNewProp(currentXMLNode, BAD_CAST"z", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->createdInVp);
            xmlNewProp(currentXMLNode, BAD_CAST"inVp", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->createdInMag);
            xmlNewProp(currentXMLNode, BAD_CAST"inMag", attrString);
            memset(attrString, '\0', 128);

            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentNode->timestamp);
            xmlNewProp(currentXMLNode, BAD_CAST"time", attrString);
            memset(attrString, '\0', 128);

            currentSegment = currentNode->firstSegment;
            while(currentSegment) {
                if(currentSegment->flag == SEGMENT_FORWARD) {
                    currentXMLNode = xmlNewTextChild(edgesXMLNode, NULL, BAD_CAST"edge", NULL);

                    xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentSegment->source->nodeID);
                    xmlNewProp(currentXMLNode, BAD_CAST"source", attrString);
                    memset(attrString, '\0', 128);

                    xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentSegment->target->nodeID);
                    xmlNewProp(currentXMLNode, BAD_CAST"target", attrString);
                    memset(attrString, '\0', 128);
                }

                currentSegment = currentSegment->next;
            }

            currentNode = currentNode->next;
        }

        currentTree = currentTree->next;
    }


    commentsXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"comments", NULL);
    currentComment = state->skeletonState->currentComment;
    if(state->skeletonState->currentComment != NULL) {
        do {
            currentXMLNode = xmlNewTextChild(commentsXMLNode, NULL, BAD_CAST"comment", NULL);
            xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentComment->node->nodeID);
            xmlNewProp(currentXMLNode, BAD_CAST"node", attrString);
            memset(attrString, '\0', 128);
            xmlNewProp(currentXMLNode, BAD_CAST"content", BAD_CAST currentComment->content);
            currentComment = currentComment->next;
        } while (currentComment != state->skeletonState->currentComment);
    }


    branchesXMLNode = xmlNewTextChild(thingsXMLNode, NULL, BAD_CAST"branchpoints", NULL);
    while((currentBranchPointID = (PTRSIZEINT)popStack(reverseBranchStack))) {
        currentXMLNode = xmlNewTextChild(branchesXMLNode,
                                         NULL,
                                         BAD_CAST"branchpoint",
                                         NULL);

#if QT_POINTER_SIZE == 8
        xmlStrPrintf(attrString, 128, BAD_CAST"%"PRId64, currentBranchPointID);
#else
        xmlStrPrintf(attrString, 128, BAD_CAST"%d", currentBranchPointID);
#endif

        xmlNewProp(currentXMLNode, BAD_CAST"id", attrString);
        memset(attrString, '\0', 128);
    }

    r = xmlSaveFormatFile(state->skeletonState->skeletonFile, xmlDocument, 1);
    xmlFreeDoc(xmlDocument);
    */
    return r;
}
//uint loadNMLSkeleton() { }

bool Skeletonizer::loadSkeleton() {
    /*
    xmlDocPtr xmlDocument;
    xmlNodePtr currentXMLNode, thingsXMLNode, thingOrParamXMLNode, nodesEdgesXMLNode;
    int neuronID = 0, nodeID = 0, merge = false;
    int nodeID1, nodeID2, greatestNodeIDbeforeLoading = 0, greatestTreeIDbeforeLoading = 0;
    float radius;
    Byte VPtype;
    int inMag, magnification = 0;
    int globalMagnificationSpecified = false;
    xmlChar *attribute;
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode = NULL;
    Coordinate *currentCoordinate, loadedPosition;
    Coordinate offset;
    floatCoordinate scale;
    int time, activeNodeID = 0;
    int skeletonTime = 0;
    color4F neuronColor;

    QTime bench;
    bench.start();
    LOG("Starting to load skeleton...");

     //  This function should always be called through UI_loadSkeleton for
     //  proper error and file name display to the user.


     //  There's no sanity check for values read from files, yet.



     // These defaults should usually always be overridden by values
     // given in the file.


    SET_COORDINATE(offset, state->offset.x, state->offset.y, state->offset.z);
    SET_COORDINATE(scale, state->scale.x, state->scale.y, state->scale.z);
    SET_COORDINATE(loadedPosition, 0, 0, 0);

    currentCoordinate = (Coordinate*) malloc(sizeof(Coordinate));
    if(currentCoordinate == NULL) {
        LOG("Out of memory.");
        return false;
    }
    memset(currentCoordinate, '\0', sizeof(currentCoordinate));

    xmlDocument = xmlParseFile(state->skeletonState->skeletonFile);

    if(xmlDocument == NULL) {
        LOG("Document not parsed successfully.");
        return false;
    }

    thingsXMLNode = xmlDocGetRootElement(xmlDocument);
    if(thingsXMLNode == NULL) {
        LOG("Empty document.");
        xmlFreeDoc(xmlDocument);
        return false;
    }

    if(xmlStrEqual(thingsXMLNode->name, (const xmlChar *)"things") == false) {
        LOG("Root element %s in file %s unrecognized. Not loading.",
            thingsXMLNode->name,
            state->skeletonState->skeletonFile);
        return false;
    }

    if(!state->skeletonState->mergeOnLoadFlag) {
        merge = false;
        clearSkeleton(CHANGE_MANUAL, true);
    }
    else {
        merge = true;
        greatestNodeIDbeforeLoading = state->skeletonState->greatestNodeID;
        greatestTreeIDbeforeLoading = state->skeletonState->greatestTreeID;
    }

    // If "createdin"-node does not exist, skeleton was created in a version
     // before 3.2
    strcpy(state->skeletonState->skeletonCreatedInVersion, "pre-3.2");

    thingOrParamXMLNode = thingsXMLNode->xmlChildrenNode;
    while(thingOrParamXMLNode) {
        LOG("Document parsed successfully.");
        bench.restart();
        if(xmlStrEqual(thingOrParamXMLNode->name, (const xmlChar *)"parameters")) {
            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"createdin")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"version");
                    if(attribute){
                        strcpy(state->skeletonState->skeletonCreatedInVersion, (char *)attribute);
                    }
                }
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"magnification")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"factor");

                     // This is for legacy skeleton files.
                     // In the past, magnification was specified on a per-file basis
                     // or not specified at all.
                     // A magnification factor of 0 shall represent an unknown magnification.

                    if(attribute) {
                        magnification = atoi((char *)attribute);
                        globalMagnificationSpecified = true;
                    }
                    else
                        magnification = 0;
                }
                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"offset")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        offset.x = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        offset.y = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        offset.z = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"time")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"ms");
                    if(attribute) {
                        if(hasObfuscatedTime()) {
                            skeletonTime = xorInt(atoi((char *) attribute));
                        }
                        else {
                            skeletonTime = atoi((char *)attribute);
                        }
                    }
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"activeNode")) {
                    if(!merge) {
                        attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"id");
                        if(attribute) {
                            activeNodeID = atoi((char *)attribute);
                        }
                    }
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"scale")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        scale.x = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        scale.y = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        scale.z = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"editPosition")) {
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                    if(attribute)
                        loadedPosition.x = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                    if(attribute)
                        loadedPosition.y = atoi((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                    if(attribute)
                        loadedPosition.z = atoi((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"skeletonVPState")) {
                    int j = 0;
                    char element [8];
                    for (j = 0; j < 16; j++){
                        sprintf (element, "E%d", j);
                        attribute = xmlGetProp(currentXMLNode, (const xmlChar *)element);
                        state->skeletonState->skeletonVpModelView[j] = atof((char *)attribute);
                    }
                    glMatrixMode(GL_MODELVIEW);
                    glLoadMatrixf(state->skeletonState->skeletonVpModelView);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"translateX");
                    state->skeletonState->translateX = atof((char *)attribute);

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"translateY");
                    state->skeletonState->translateY = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"vpSettingsZoom")) {

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"XYPlane");
                    if(attribute)
                        state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"XZPlane");
                    if(attribute)
                        state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"YZPlane");
                    if(attribute)
                        state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = atof((char *)attribute);
                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"SkelVP");
                    if(attribute)
                        state->skeletonState->zoomLevel = atof((char *)attribute);
                }

                if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"idleTime")) {

                    attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"ms");
                    if(attribute) {
                        if(hasObfuscatedTime()) {
                            state->skeletonState->idleTime = xorInt(atoi((char*)attribute));
                        } else {
                            state->skeletonState->idleTime = atoi((char*)attribute);
                        }
                    }
                    state->skeletonState->idleTimeTicksOffset = state->time.elapsed();
                }

                currentXMLNode = currentXMLNode->next;
            }
        }

        if(xmlStrEqual(thingOrParamXMLNode->name,
                       (const xmlChar *)"comments")) {

            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name,
                               (const xmlChar *)"comment")) {

                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"node");
                    if(attribute) {
                        if(!merge)
                            nodeID = atoi((char *)attribute);
                        else
                            nodeID = atoi((char *)attribute) + greatestNodeIDbeforeLoading;

                        currentNode = findNodeByNodeID(nodeID);
                    }
                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"content");
                    if(attribute && currentNode) {
                        addComment(CHANGE_MANUAL, (char *)attribute, currentNode, 0);
                    }
                }

                currentXMLNode = currentXMLNode->next;
            }
        }


        if(xmlStrEqual(thingOrParamXMLNode->name,
                       (const xmlChar *)"branchpoints")) {

            currentXMLNode = thingOrParamXMLNode->children;
            while(currentXMLNode) {
                if(xmlStrEqual(currentXMLNode->name,
                               (const xmlChar *)"branchpoint")) {

                    attribute = xmlGetProp(currentXMLNode,
                                           (const xmlChar *)"id");
                    if(attribute) {
                        if(!merge)
                            nodeID = atoi((char *)attribute);
                        else
                            nodeID = atoi((char *)attribute) + greatestNodeIDbeforeLoading;

                        currentNode = findNodeByNodeID(nodeID);
                        if(currentNode)
                            pushBranchNode(CHANGE_MANUAL, true, false, currentNode, 0);
                    }
                }

                currentXMLNode = currentXMLNode->next;
            }
        }

        if(xmlStrEqual(thingOrParamXMLNode->name, (const xmlChar *)"thing")) {
            // Add tree
            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "id");
            if(attribute) {
                neuronID = atoi((char *)attribute);
                free(attribute);
            }
            else
                neuronID = 0;   // Whatever.

            // -1. causes default color assignment based on ID
            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.r");
            if(attribute) {
                neuronColor.r = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else {
                neuronColor.r = -1.;
            }

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.g");
            if(attribute) {
                neuronColor.g = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.g = -1.;

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.b");
            if(attribute) {
                neuronColor.b = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.b = -1.;

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *) "color.a");
            if(attribute) {
                neuronColor.a = (float)strtod((char *)attribute, (char **)NULL);
                free(attribute);
            }
            else
                neuronColor.a = 1.;

            if(!merge) {
                currentTree = addTreeListElement(true, CHANGE_MANUAL, neuronID, neuronColor);
                setActiveTreeByID(neuronID);
            }
            else {
                neuronID += greatestTreeIDbeforeLoading;
                currentTree = addTreeListElement(true, CHANGE_MANUAL, neuronID, neuronColor);
                setActiveTreeByID(currentTree->treeID);
                neuronID = currentTree->treeID;
            }

            attribute = xmlGetProp(thingOrParamXMLNode, (const xmlChar *)"comment");
            if(attribute) {
                addTreeComment(CHANGE_MANUAL, currentTree->treeID, (char *)attribute);
                free(attribute);
            }

            nodesEdgesXMLNode = thingOrParamXMLNode->children;
            while(nodesEdgesXMLNode) {
                if(xmlStrEqual(nodesEdgesXMLNode->name, (const xmlChar *)"nodes")) {
                    currentXMLNode = nodesEdgesXMLNode->children;
                    while(currentXMLNode) {
                        if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"node")) {
                            // Add node.

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"id");
                            if(attribute) {
                                nodeID = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID = 0;

                            LOG("Adding node: %d", nodeID);
                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"radius");
                            if(attribute) {
                                radius = (float)strtod((char *)attribute, (char **)NULL);
                                free(attribute);
                            }
                            else
                                radius = state->skeletonState->defaultNodeRadius;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"x");
                            if(attribute) {
                                currentCoordinate->x = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->x = currentCoordinate->x * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->x = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"y");
                            if(attribute) {
                                currentCoordinate->y = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->y = currentCoordinate->y * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->y = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"z");
                            if(attribute) {
                                currentCoordinate->z = atoi((char *)attribute) - 1;
                                if(globalMagnificationSpecified)
                                    currentCoordinate->z = currentCoordinate->z * magnification;
                                free(attribute);
                            }
                            else
                                currentCoordinate->z = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"inVp");
                            if(attribute) {
                                VPtype = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                VPtype = VIEWPORT_UNDEFINED;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"inMag");
                            if(attribute) {
                                inMag = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                inMag = magnification; // For legacy skeleton files

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"time");
                            if(attribute) {
                                time = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                time = skeletonTime; // For legacy skeleton files

                            if(!merge)
                                addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, false);
                            else {
                                nodeID += greatestNodeIDbeforeLoading;
                                addNode(CHANGE_MANUAL, nodeID, radius, neuronID, currentCoordinate, VPtype, inMag, time, false);
                            }
                        }

                        currentXMLNode = currentXMLNode->next;
                    }
                }
                else if(xmlStrEqual(nodesEdgesXMLNode->name, (const xmlChar *)"edges")) {
                    currentXMLNode = nodesEdgesXMLNode->children;
                    while(currentXMLNode) {
                        if(xmlStrEqual(currentXMLNode->name, (const xmlChar *)"edge")) {
                            // Add edge
                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"source");
                            if(attribute) {
                                nodeID1 = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID1 = 0;

                            attribute = xmlGetProp(currentXMLNode, (const xmlChar *)"target");
                            if(attribute) {
                                nodeID2 = atoi((char *)attribute);
                                free(attribute);
                            }
                            else
                                nodeID2 = 0;

                            // printf("Trying to add a segment between %d and %d\n", nodeID1, nodeID2);
                            if(!merge)
                                addSegment(CHANGE_MANUAL, nodeID1, nodeID2);
                            else
                                addSegment(CHANGE_MANUAL, nodeID1 + greatestNodeIDbeforeLoading, nodeID2 + greatestNodeIDbeforeLoading);

                        }
                        currentXMLNode = currentXMLNode->next;
                    }
                }

                nodesEdgesXMLNode = nodesEdgesXMLNode->next;
            }
        }

        thingOrParamXMLNode = thingOrParamXMLNode->next;
        qDebug() << "end " << bench.elapsed();
    }

    free(currentCoordinate);
    xmlFreeDoc(xmlDocument);

    if(activeNodeID) {
        setActiveNode(CHANGE_MANUAL, NULL, activeNodeID);
    }


    /* @todo
    if((loadedPosition.x != 0) &&
       (loadedPosition.y != 0) &&
       (loadedPosition.z != 0)) {
        state->viewerState->currentPosition.x =
            loadedPosition.x - 1;
        state->viewerState->currentPosition.y =
            loadedPosition.y - 1;
        state->viewerState->currentPosition.z =
            loadedPosition.z - 1;
         @todo change to Signal loadSkeleton has to be non-static
        emit updatePositionSignal(TELL_COORDINATE_CHANGE);

} */

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    state->skeletonState->skeletonTime = skeletonTime;
    state->skeletonState->skeletonTimeCorrection = state->time.elapsed();
    return true;
}

void Skeletonizer::setDefaultSkelFileName() {
    // Generate a default file name based on date and time.
    time_t curtime;
    struct tm *localtimestruct;

    curtime = time(NULL);
    localtimestruct = localtime(&curtime);
    if(localtimestruct->tm_year >= 100)
        localtimestruct->tm_year -= 100;

#ifdef Q_OS_UNIX
    snprintf(state->skeletonState->skeletonFile,
            8192,
            "/Users/amos/skeleton-%.2d%.2d%.2d-%.2d%.2d.000.nml",
            localtimestruct->tm_mday,
            localtimestruct->tm_mon + 1,
            localtimestruct->tm_year,
            localtimestruct->tm_hour,
            localtimestruct->tm_min);
#else
    snprintf(state->skeletonState->skeletonFile,
            8192,
            "skeletonFiles\\skeleton-%.2d%.2d%.2d-%.2d%.2d.000.nml",
            localtimestruct->tm_mday,
            localtimestruct->tm_mon + 1,
            localtimestruct->tm_year,
            localtimestruct->tm_hour,
            localtimestruct->tm_min);
#endif
    MainWindow::cpBaseDirectory(state->viewerState->gui->skeletonDirectory, state->skeletonState->skeletonFile, 2048);
}

bool Skeletonizer::delActiveNode() {
    if(state->skeletonState->activeNode) {
        delNode(CHANGE_MANUAL, 0, state->skeletonState->activeNode);
    }
    else {
        return false;
    }

    return true;
}

bool Skeletonizer::delActiveTree() {
    struct treeListElement *nextTree = NULL;

    if(state->skeletonState->activeTree) {
        if(state->skeletonState->activeTree->next) {
            nextTree = state->skeletonState->activeTree->next;
        }
        else if(state->skeletonState->activeTree->previous) {
            nextTree = state->skeletonState->activeTree->previous;
        }

        delTree(CHANGE_MANUAL, state->skeletonState->activeTree->treeID);

        if(nextTree) {
            setActiveTreeByID(nextTree->treeID);
        }
        else {
            state->skeletonState->activeTree = NULL;
        }
    }
    else {
       LOG("No active tree available.");
       return false;
    }

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_LINK_WITH_ACTIVE_NODE;

    return true;
}

bool Skeletonizer::delSegment(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    // Delete the segment out of the segment list and out of the visualization structure!


    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!segToDel)
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(!segToDel) {
        LOG("Cannot delete segment, no segment with corresponding node IDs available!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!segToDel) {
        segToDel = findSegmentByNodeIDs(sourceNodeID, targetNodeID);
    } else {
        sourceNodeID = segToDel->source->nodeID;
        targetNodeID = segToDel->target->nodeID;
    }

    if(!segToDel) {
        LOG("Cannot delete segment, no segment with corresponding node IDs available!");
        Knossos::unlockSkeleton(false);
        return false;

    }

    /* numSegs counts forward AND backward segments!!! */
    segToDel->source->numSegs--;
    segToDel->target->numSegs--;

    //Out of skeleton structure
    delSegmentFromSkeletonStruct(segToDel);

    if(segToDel == segToDel->source->firstSegment)
        segToDel->source->firstSegment = segToDel->next;
    else {
        //if(segToDel->previous) //Why??? Previous EXISTS if its not the first seg...
            segToDel->previous->next = segToDel->next;
        if(segToDel->next)
            segToDel->next->previous = segToDel->previous;
    }

    //Delete reverse segment in target node
    if(segToDel->reverseSegment == segToDel->target->firstSegment)
        segToDel->target->firstSegment = segToDel->reverseSegment->next;
    else {
        //if(segToDel->reverseSegment->previous)
            segToDel->reverseSegment->previous->next = segToDel->reverseSegment->next;
        if(segToDel->reverseSegment->next)
            segToDel->reverseSegment->next->previous = segToDel->reverseSegment->previous;
    }

    /* A bit cumbersome, but we cannot delete the segment and then find its source node.. */
    segToDel->length = 0.f;
    updateCircRadius(segToDel->source);

    free(segToDel);
    state->skeletonState->totalSegmentElements--;

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_DELSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

/*
 * We have to delete the node from 2 structures: the skeleton's nested linked list structure
 * and the skeleton visualization structure (hashtable with skeletonDCs).
 */
bool Skeletonizer::delNode(int targetRevision, int nodeID, nodeListElement *nodeToDel) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext = NULL;
    struct nodeListElement *newActiveNode = NULL;
    int treeID;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!nodeToDel)
        nodeToDel = findNodeByNodeID(nodeID);

    nodeID = nodeToDel->nodeID;

    if(!nodeToDel) {
        LOG("The given node %d doesn't exist. Unable to delete it.", nodeID);
        Knossos::unlockSkeleton(false);
        return false;
    }

    treeID = nodeToDel->correspondingTree->treeID;

    if(nodeToDel->comment) {
        delComment(CHANGE_MANUAL, nodeToDel->comment, 0);
    }

     // First, delete all segments pointing towards and away of the nodeToDelhas
     // been

    currentSegment = nodeToDel->firstSegment;

    while(currentSegment) {

        tempNext = currentSegment->next;

        if(currentSegment->flag == SEGMENT_FORWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment);
        else if(currentSegment->flag == SEGMENT_BACKWARD)
            delSegment(CHANGE_MANUAL, 0,0, currentSegment->reverseSegment);

        currentSegment = tempNext;
    }

    nodeToDel->firstSegment = NULL;


     // Delete the node out of the visualization structure


    delNodeFromSkeletonStruct(nodeToDel);

    if(nodeToDel == nodeToDel->correspondingTree->firstNode) {
        nodeToDel->correspondingTree->firstNode = nodeToDel->next;
    }
    else {
        if(nodeToDel->previous) {
            nodeToDel->previous->next = nodeToDel->next;
        }
        if(nodeToDel->next) {
            nodeToDel->next->previous = nodeToDel->previous;
        }
    }

    setDynArray(state->skeletonState->nodesByNodeID, nodeToDel->nodeID, NULL);

    if(state->skeletonState->activeNode == nodeToDel) {
        newActiveNode = findNearbyNode(nodeToDel->correspondingTree,
                                       nodeToDel->position);

        setActiveNode(CHANGE_NOSYNC, newActiveNode, 0);
    }

    free(nodeToDel);

    state->skeletonState->totalNodeElements--;
    //state->viewerState->gui->totalNodes--;

    setDynArray(state->skeletonState->nodeCounter,
            treeID,
            (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) - 1));

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELNODE, nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }

    Knossos::unlockSkeleton(true);
    return true;
}

bool Skeletonizer::delTree(int targetRevision, int treeID) {

    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    struct treeListElement *currentTree;
    struct nodeListElement *currentNode, *nodeToDel;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        LOG("There exists no tree with ID %d. Unable to delete it.", treeID);
        Knossos::unlockSkeleton(false);
        return false;
    }

    currentNode = currentTree->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNode(CHANGE_MANUAL, 0, nodeToDel);
    }
    currentTree->firstNode = NULL;

    if(currentTree == state->skeletonState->firstTree)
        state->skeletonState->firstTree = currentTree->next;
    else {
        if(currentTree->previous)
            currentTree->previous->next = currentTree->next;
        if(currentTree->next)
            currentTree->next->previous = currentTree->previous;
    }

    free(currentTree);

    state->skeletonState->treeElements--;

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELTREE, treeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

nodeListElement* Skeletonizer::findNearbyNode(treeListElement *nearbyTree, Coordinate searchPosition) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeWithCurrentlySmallestDistance = NULL;
    treeListElement *currentTree = NULL;
    floatCoordinate distanceVector;
    float smallestDistance = 0;


    //  If available, search for a node within nearbyTree first.


    if(nearbyTree) {
        currentNode = nearbyTree->firstNode;

        while(currentNode) {
            // We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;
            //set nearest distance to distance to first node found, then to distance of any nearer node found.
            if(smallestDistance == 0 || euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }
            currentNode = currentNode->next;
        }

        if(nodeWithCurrentlySmallestDistance) {
            return nodeWithCurrentlySmallestDistance;
        }
    }

     // Ok, we didn't find any node in nearbyTree.
     // Now we take the nearest node, independent of the tree it belongs to.


    currentTree = state->skeletonState->firstTree;
    smallestDistance = 0;
    while(currentTree) {
        currentNode = currentTree->firstNode;

        while(currentNode) {
            //We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;

            if(smallestDistance == 0 || euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }

            currentNode = currentNode->next;
        }

       currentTree = currentTree->next;
    }

    if(nodeWithCurrentlySmallestDistance) {
        return nodeWithCurrentlySmallestDistance;
    }

    return NULL;
}

nodeListElement* Skeletonizer::findNodeInRadius(Coordinate searchPosition) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeWithCurrentlySmallestDistance = NULL;
    treeListElement *currentTree = NULL;
    floatCoordinate distanceVector;
    float smallestDistance = CATCH_RADIUS;

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        currentNode = currentTree->firstNode;

        while(currentNode) {
            //We make the nearest node the next active node
            distanceVector.x = (float)searchPosition.x - (float)currentNode->position.x;
            distanceVector.y = (float)searchPosition.y - (float)currentNode->position.y;
            distanceVector.z = (float)searchPosition.z - (float)currentNode->position.z;

            if(euclidicNorm(&distanceVector) < smallestDistance) {
                smallestDistance = euclidicNorm(&distanceVector);
                nodeWithCurrentlySmallestDistance = currentNode;
            }
            currentNode = currentNode->next;
        }
       currentTree = currentTree->next;
    }

    if(nodeWithCurrentlySmallestDistance) {
        return nodeWithCurrentlySmallestDistance;
    }

    return NULL;
}

bool Skeletonizer::setActiveTreeByID(int treeID) {
    treeListElement *currentTree;

    currentTree = findTreeByTreeID(treeID);
    if(!currentTree) {
        LOG("There exists no tree with ID %d!", treeID);
        return false;
    }


    state->skeletonState->activeTree = currentTree;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    state->viewerState->gui->activeTreeID = currentTree->treeID;

    qDebug() << " new active tree id is " << state->skeletonState->activeTree->treeID;

    return true;
}

bool Skeletonizer::setActiveNode(int targetRevision, nodeListElement *node, int nodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     // If both *node and nodeID are specified, nodeID wins.
     // If neither *node nor nodeID are specified
     // (node == NULL and nodeID == 0), the active node is
     // set to NULL.

    if(targetRevision != CHANGE_NOSYNC) {
        if(Knossos::lockSkeleton(targetRevision) == false) {
            Knossos::unlockSkeleton(false);
            return false;
        }
    }

    if(nodeID != 0) {
        node = findNodeByNodeID(nodeID);
        if(!node) {
            LOG("No node with id %d available.", nodeID);
            Knossos::unlockSkeleton(false);
            return false;
        }
    }

    if(node) {
        nodeID = node->nodeID;
    }


    state->skeletonState->activeNode = node;
    state->skeletonState->viewChanged = true;
    state->skeletonState->skeletonChanged = true;

    if(node) {
        setActiveTreeByID(node->correspondingTree->treeID);
    }

    else
        state->skeletonState->activeTree = NULL;

    if(node) {
        if(node->comment) {
            state->skeletonState->currentComment = node->comment;
            memset(state->skeletonState->commentBuffer, '\0', 10240);

            strncpy(state->skeletonState->commentBuffer,
                    state->skeletonState->currentComment->content,
                    strlen(state->skeletonState->currentComment->content));
        }
        else
            memset(state->skeletonState->commentBuffer, '\0', 10240);
    }

    state->skeletonState->unsavedChanges = true;

    if(targetRevision != CHANGE_NOSYNC) {
        state->skeletonState->skeletonRevision++;

        if(targetRevision == CHANGE_MANUAL) {
            if(!Client::syncMessage("brd", KIKI_SETACTIVENODE, nodeID)) {
                Client::skeletonSyncBroken();
            }
        }
        else {
            Viewer::refreshViewports();
        }
        Knossos::unlockSkeleton(true);
    }

    if(node) {
        state->skeletonState->activeNode= node;
        qDebug() << node->nodeID;
    }


     // Calling drawGUI() here leads to a crash during synchronizationn.
     // Why? TDItem

    // drawGUI();

    return true;
}

int Skeletonizer::addNode(int targetRevision,
                int nodeID,
                float radius,
                int treeID,
                Coordinate *position,
                Byte VPtype,
                int inMag,
                int time,
                int respectLocks) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    nodeListElement *tempNode = NULL;
    treeListElement *tempTree = NULL;
    floatCoordinate lockVector;
    int lockDistance = 0;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    state->skeletonState->branchpointUnresolved = false;

     // respectLocks refers to locking the position to a specific coordinate such as to
     // disallow tracing areas further than a certain distance away from a specific point in the
     // dataset.


    if(respectLocks) {
        if(state->skeletonState->positionLocked) {
            lockVector.x = (float)position->x - (float)state->skeletonState->lockedPosition.x;
            lockVector.y = (float)position->y - (float)state->skeletonState->lockedPosition.y;
            lockVector.z = (float)position->z - (float)state->skeletonState->lockedPosition.z;

            lockDistance = euclidicNorm(&lockVector);
            if(lockDistance > state->skeletonState->lockRadius) {
                LOG("Node is too far away from lock point (%d), not adding.", lockDistance);
                Knossos::unlockSkeleton(false);
                return false;
            }
        }
    }

    if(nodeID) {
        tempNode = findNodeByNodeID(nodeID);
    }

    if(tempNode) {
        LOG("Node with ID %d already exists, no node added.", nodeID);
        Knossos::unlockSkeleton(false);
        return false;
    }
    tempTree = findTreeByTreeID(treeID);

    if(!tempTree) {
        LOG("There exists no tree with the provided ID %d!", treeID);
        Knossos::unlockSkeleton(false);
        return false;
    }

    // One node more in all trees
    state->skeletonState->totalNodeElements++;

    // One more node in this tree.
    setDynArray(state->skeletonState->nodeCounter,
                treeID,
                (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter, treeID) + 1));

    if(nodeID == 0) {
        nodeID = state->skeletonState->totalNodeElements;
        //Test if node ID over node counter is available. If not, find a valid one.
        while(findNodeByNodeID(nodeID)) {
            nodeID++;
        }
    }

    tempNode = addNodeListElement(nodeID, radius, &(tempTree->firstNode), position, inMag);
    tempNode->correspondingTree = tempTree;
    tempNode->comment = NULL;
    tempNode->createdInVp = VPtype;

    if(tempTree->firstNode == NULL) {
        tempTree->firstNode = tempNode;
    }

    updateCircRadius(tempNode);

    if(time == -1) {
        time = state->skeletonState->skeletonTime - state->skeletonState->skeletonTimeCorrection + state->time.elapsed();
    }

    tempNode->timestamp = time;

    setDynArray(state->skeletonState->nodesByNodeID, nodeID, (void *)tempNode);

    //printf("Added node %p, id %d, tree %p\n", tempNode, tempNode->nodeID,
    //        tempNode->correspondingTree);

    //Add a pointer to the node in the skeleton DC structure
   // addNodeToSkeletonStruct(tempNode);
    state->skeletonState->skeletonChanged = true;

    if(nodeID > state->skeletonState->greatestNodeID) {
        state->skeletonState->greatestNodeID = nodeID;
    }
    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = true;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brddfddddddd", KIKI_ADDNODE,
                                              state->clientState->myId,
                                              nodeID,
                                              radius,
                                              treeID,
                                              VPtype,
                                              inMag,
                                              -1,
                                              position->x,
                                              position->y,
                                              position->z))
            Client::skeletonSyncBroken();
    }

    else {
        Viewer::refreshViewports();
    }

    Knossos::unlockSkeleton(true);

    return nodeID;
}

bool Skeletonizer::addSegment(int targetRevision, int sourceNodeID, int targetNodeID) {
    nodeListElement *targetNode, *sourceNode;
    segmentListElement *sourceSeg;
    floatCoordinate node1, node2;
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(findSegmentByNodeIDs(sourceNodeID, targetNodeID)) {
        LOG("Segment between nodes %d and %d exists already.", sourceNodeID, targetNodeID);
        Knossos::unlockSkeleton(false);
        return false;
    }

    //Check if source and target nodes are existant
    sourceNode = findNodeByNodeID(sourceNodeID);
    targetNode = findNodeByNodeID(targetNodeID);

    if(!(sourceNode) || !(targetNode)) {
        LOG("Could not link the nodes, because at least one is missing!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(sourceNode == targetNode) {
        LOG("Cannot link node with itself!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    //One segment more in all trees
    state->skeletonState->totalSegmentElements++;

     // Add the segment to the tree structure

    sourceSeg = addSegmentListElement(&(sourceNode->firstSegment), sourceNode, targetNode);
    sourceSeg->reverseSegment = addSegmentListElement(&(targetNode->firstSegment), sourceNode, targetNode);

    sourceSeg->reverseSegment->flag = SEGMENT_BACKWARD;

    sourceSeg->reverseSegment->reverseSegment = sourceSeg;

    /* numSegs counts forward AND backward segments!!! */
   sourceNode->numSegs++;
   targetNode->numSegs++;

   /* Do we really skip this node? Test cum dist. to last rendered node! */
   node1.x = (float)sourceNode->position.x;
   node1.y = (float)sourceNode->position.y;
   node1.z = (float)sourceNode->position.z;

   node2.x = (float)targetNode->position.x - node1.x;
   node2.y = (float)targetNode->position.y - node1.y;
   node2.z = (float)targetNode->position.z - node1.z;

   sourceSeg->length = sourceSeg->reverseSegment->length
       = sqrtf(scalarProduct(&node2, &node2));

    // Add the segment to the skeleton DC structure


    addSegmentToSkeletonStruct(sourceSeg);

    // printf("added segment for nodeID %d: %d, %d, %d -> nodeID %d: %d, %d, %d\n", sourceNode->nodeID, sourceNode->position.x + 1, sourceNode->position.y + 1, sourceNode->position.z + 1, targetNode->nodeID, targetNode->position.x + 1, targetNode->position.y + 1, targetNode->position.z + 1);

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_ADDSEGMENT, sourceNodeID, targetNodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::clearSkeleton(int targetRevision, int loadingSkeleton) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *currentTree, *treeToDel;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    currentTree = state->skeletonState->firstTree;
    while(currentTree) {
        treeToDel = currentTree;
        currentTree = treeToDel->next;
        delTree(CHANGE_MANUAL, treeToDel->treeID);
    }

    state->skeletonState->activeNode = NULL;
    state->skeletonState->activeTree = NULL;

    state->skeletonState->skeletonTime = 0;
    //state->skeletonState->skeletonTimeCorrection = SDL_GetTicks(); SDL TODO

    Hashtable::ht_rmtable(state->skeletonState->skeletonDCs);
    delDynArray(state->skeletonState->nodeCounter);
    delDynArray(state->skeletonState->nodesByNodeID);
    delStack(state->skeletonState->branchStack);

    //Create a new hash-table that holds the skeleton datacubes
    state->skeletonState->skeletonDCs = Hashtable::ht_new(state->skeletonState->skeletonDCnumber);
    if(state->skeletonState->skeletonDCs == HT_FAILURE) {
        LOG("Unable to create skeleton hash-table.");
        Knossos::unlockSkeleton(false);
        return false;
    }

    //Generate empty tree structures
    state->skeletonState->firstTree = NULL;
    state->skeletonState->totalNodeElements = 0;
    state->skeletonState->totalSegmentElements = 0;
    state->skeletonState->treeElements = 0;
    state->skeletonState->activeTree = NULL;
    state->skeletonState->activeNode = NULL;

    if(loadingSkeleton == false) {
        setDefaultSkelFileName();
    }

    state->skeletonState->nodeCounter = newDynArray(1048576);
    state->skeletonState->nodesByNodeID = newDynArray(1048576);
    state->skeletonState->branchStack = newStack(1048576);

    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;

    state->skeletonState->skeletonRevision++;
    state->skeletonState->unsavedChanges = true;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("br", KIKI_CLEARSKELETON)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    state->skeletonState->skeletonRevision = 0;

    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::mergeTrees(int targetRevision, int treeID1, int treeID2) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    treeListElement *tree1, *tree2;
    nodeListElement *currentNode;
    nodeListElement *firstNode, *lastNode;

    if(treeID1 == treeID2) {
        LOG("Could not merge trees. Provided IDs are the same!");
        return false;
    }
    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    tree1 = findTreeByTreeID(treeID1);
    tree2 = findTreeByTreeID(treeID2);

    if(!(tree1) || !(tree2)) {
        LOG("Could not merge trees, provided IDs are not valid!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    currentNode = tree2->firstNode;

    while(currentNode) {
        //Change the corresponding tree
        currentNode->correspondingTree = tree1;

        currentNode = currentNode->next;
    }

    //Now we insert the node list of tree2 before the node list of tree1
    if(tree1->firstNode && tree2->firstNode) {
        //First, we have to find the last node of tree2 (this node has to be connected
        //to the first node inside of tree1)
        firstNode = tree2->firstNode;
        lastNode = firstNode;
        while(lastNode->next) {
            lastNode = lastNode->next;
        }

        tree1->firstNode->previous = lastNode;
        lastNode->next = tree1->firstNode;
        tree1->firstNode = firstNode;

    }
    else if(tree2->firstNode) {
        tree1->firstNode = tree2->firstNode;
    }

    // The new node count for tree1 is the old node count of tree1 plus the node
    // count of tree2
    setDynArray(state->skeletonState->nodeCounter,
                tree1->treeID,
                (void *)
                ((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                        tree1->treeID) +
                (PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                        tree2->treeID)));

    // The old tree is gone and has 0 nodes.
    setDynArray(state->skeletonState->nodeCounter,
                tree2->treeID,
                (void *)0);

    //Delete the "empty" tree 2
    if(tree2 == state->skeletonState->firstTree) {
        state->skeletonState->firstTree = tree2->next;
        tree2->next->previous = NULL;
    }
    else {
        tree2->previous->next = tree2->next;
        if(tree2->next)
            tree2->next->previous = tree2->previous;
    }
    free(tree2);

    if(state->skeletonState->activeTree->treeID == tree2->treeID) {
       setActiveTreeByID(tree1->treeID);
       state->viewerState->gui->activeTreeID = tree1->treeID;
    }

    state->skeletonState->treeElements--;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brdd", KIKI_MERGETREE, treeID1, treeID2)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);
    return true;
}

nodeListElement* Skeletonizer::getNodeWithPrevID(nodeListElement *currentNode) {
    nodeListElement *node = currentNode->correspondingTree->firstNode;
    nodeListElement *prevNode = NULL;
    nodeListElement *highestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int maxID = 0;

    while(node) {
        if(node->nodeID > maxID) {
            highestNode = node;
            maxID = node->nodeID;
        }

        if(node->nodeID < currentNode->nodeID) {
            tempMinDistance = currentNode->nodeID - node->nodeID;

            if(tempMinDistance == 1) { //smallest distance possible
                return node;
            }

            if(tempMinDistance < minDistance) {
                minDistance = tempMinDistance;
                prevNode = node;
            }


        }

        node = node->next;
    }

    if(!prevNode) {
        prevNode = highestNode;
    }

    return prevNode;
}

nodeListElement* Skeletonizer::getNodeWithNextID(nodeListElement *currentNode) {
    nodeListElement *node = currentNode->correspondingTree->firstNode;
    nodeListElement *nextNode = NULL;
    nodeListElement *lowestNode = NULL;
    unsigned int minDistance = UINT_MAX;
    unsigned int tempMinDistance = minDistance;
    unsigned int minID = UINT_MAX;

    while(node) {
        if(node->nodeID < minID) {
            lowestNode = node;
            minID = node->nodeID;
        }

        if(node->nodeID > currentNode->nodeID) {
            tempMinDistance = node->nodeID - currentNode->nodeID;

            if(tempMinDistance == 1) { //smallest distance possible
                return node;
            }

            if(tempMinDistance < minDistance) {
                minDistance = tempMinDistance;
                nextNode = node;
            }
        }

        node = node->next;
    }

    if(!nextNode) {
        nextNode = lowestNode;
    }

    return nextNode;
}

nodeListElement* Skeletonizer::findNodeByNodeID(int nodeID) {
    nodeListElement *node;
    node = (struct nodeListElement *)getDynArray(state->skeletonState->nodesByNodeID, nodeID);
    return node;
}

nodeListElement* Skeletonizer::findNodeByCoordinate(Coordinate *position) {
    nodeListElement *currentNode;
    treeListElement *currentTree;

    currentTree = state->skeletonState->firstTree;

    while(currentTree) {
        currentNode = currentTree->firstNode;
        while(currentNode) {
            if(COMPARE_COORDINATE(currentNode->position, *position))
                return currentNode;

            currentNode = currentNode->next;
        }
        currentTree = currentTree->next;
    }
    return NULL;
}

treeListElement* Skeletonizer::addTreeListElement(int sync, int targetRevision, int treeID, color4F color) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

     //  The variable sync is a workaround for the problem that this function
     // will sometimes be called by other syncable functions that themselves hold
     // the lock and handle synchronization. If set to false, all synchro
     // routines will be skipped.

    treeListElement *newElement = NULL;

    if(sync != false) {
        if(Knossos::lockSkeleton(targetRevision) == false) {
            LOG("addtreelistelement unable to lock.");
            Knossos::unlockSkeleton(false);
            return false;
        }
    }


    newElement = findTreeByTreeID(treeID);
    if(newElement) {
        LOG("Tree with ID %d already exists!", treeID);
        Knossos::unlockSkeleton(true);
        return newElement;
    }

    newElement = (treeListElement*)malloc(sizeof(struct treeListElement));
    if(newElement == NULL) {
        LOG("Out of memory while trying to allocate memory for a new treeListElement.");
        Knossos::unlockSkeleton(false);
        return NULL;
    }
    memset(newElement, '\0', sizeof(struct treeListElement));

    state->skeletonState->treeElements++;

    //Tree ID is unique in tree list
    //Take the provided tree ID if there is one.
    if(treeID != 0) {
        newElement->treeID = treeID;
    }
    else {
        newElement->treeID = state->skeletonState->treeElements;
        //Test if tree ID over tree counter is available. If not, find a valid one.
        while(findTreeByTreeID(newElement->treeID)) {
            newElement->treeID++;
        }
    }



    // calling function sets values < 0 when no color was specified
    if(color.r < 0) {//Set a tree color
        int index = (newElement->treeID - 1) % 256; //first index is 0
        newElement->color.r = state->viewerState->treeAdjustmentTable[index];
        newElement->color.g = state->viewerState->treeAdjustmentTable[index + 256];
        newElement->color.b = state->viewerState->treeAdjustmentTable[index + 512];
        newElement->color.a = 1.;
    }
    else {
        newElement->color = color;
    }
    newElement->colorSetManually = false;

    memset(newElement->comment, '\0', 8192);

    //Insert the new tree at the beginning of the tree list
    newElement->next = state->skeletonState->firstTree;
    newElement->previous = NULL;
    //The old first element should have the new first element as previous element
    if(state->skeletonState->firstTree) {
        state->skeletonState->firstTree->previous = newElement;
    }
    //We change the old and new first elements
    state->skeletonState->firstTree = newElement;

    state->skeletonState->activeTree = newElement;
    LOG("Added new tree with ID: %d.", newElement->treeID);

    if(newElement->treeID > state->skeletonState->greatestTreeID) {
        state->skeletonState->greatestTreeID = newElement->treeID;
    }

    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;

    if(sync != false) {
        state->skeletonState->skeletonRevision++;

        if(targetRevision == CHANGE_MANUAL) {
            if(!Client::syncMessage("brdfff", KIKI_ADDTREE, newElement->treeID,
                            newElement->color.r, newElement->color.g, newElement->color.b)) {
                Client::skeletonSyncBroken();
            }
        }

        Knossos::unlockSkeleton(true);
    }
    else {
        Viewer::refreshViewports();
    }

    if(treeID == 1) {
        Skeletonizer::setActiveTreeByID(1);
    }

    return newElement;
}

treeListElement* Skeletonizer::getTreeWithPrevID(treeListElement *currentTree) {
    treeListElement *tree = state->skeletonState->firstTree;
    treeListElement *prevTree = NULL;
    uint idDistance = state->skeletonState->treeElements;
    uint tempDistance = idDistance;

    while(tree) {
        if(tree->treeID < currentTree->treeID) {
            tempDistance = currentTree->treeID - tree->treeID;
            if(tempDistance == 1) {//smallest distance possible
                return tree;
            }
            if(tempDistance < idDistance) {
                idDistance = tempDistance;
                prevTree = tree;
            }
        }
        tree = tree->next;
    }
    return prevTree;
}

treeListElement* Skeletonizer::getTreeWithNextID(treeListElement *currentTree) {
    treeListElement *tree = state->skeletonState->firstTree;
    treeListElement *nextTree = NULL;
    uint idDistance = state->skeletonState->treeElements;
    uint tempDistance = idDistance;

    while(tree) {
        if(tree->treeID > currentTree->treeID) {
            tempDistance = tree->treeID - currentTree->treeID;

            if(tempDistance == 1) { //smallest distance possible
                return tree;
            }
            if(tempDistance < idDistance) {
                idDistance = tempDistance;
                nextTree = tree;
            }
        }
        tree = tree->next;
    }
    return nextTree;
}

bool Skeletonizer::addTreeComment(int targetRevision, int treeID, char *comment) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    treeListElement *tree = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        LOG("addTreeComment unable to lock.");
        Knossos::unlockSkeleton(false);
        return false;
    }

    tree = findTreeByTreeID(treeID);

    if(comment && tree) {
        strncpy(tree->comment, comment, 8192);
    }

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_ADDTREECOMMENT, treeID, comment)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    qDebug() << state->skeletonState->activeTree->comment;

    return true;
}

segmentListElement* Skeletonizer::findSegmentByNodeIDs(int sourceNodeID, int targetNodeID) {
    segmentListElement *currentSegment;
    nodeListElement *currentNode;

    currentNode = findNodeByNodeID(sourceNodeID);

    if(!currentNode) { return NULL;}

    currentSegment = currentNode->firstSegment;
    while(currentSegment) {
        if(currentSegment->flag == SEGMENT_BACKWARD) {
            currentSegment = currentSegment->next;
            continue;
        }
        if(currentSegment->target->nodeID == targetNodeID) {
            return currentSegment;
        }
        currentSegment = currentSegment->next;
    }

    return NULL;
}

bool Skeletonizer::genTestNodes(uint number) {
    uint i;
    Coordinate pos;
    srand( time(NULL) );

    for(i = 1; i < number; i++) {
        pos.x = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.x);
        pos.y = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.y);
        pos.z = (int)(((double)rand() / (double)RAND_MAX) * (double)state->boundary.z);
        addNode(CHANGE_MANUAL, 0, state->skeletonState->defaultNodeRadius, 1, &pos, VIEWPORT_UNDEFINED, state->magnification, 0, false);
    }

    return true;
}

bool Skeletonizer::editNode(int targetRevision,
                 int nodeID,
                 nodeListElement *node,
                 float newRadius,
                 int newXPos,
                 int newYPos,
                 int newZPos,
                 int inMag) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    segmentListElement *currentSegment = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(!node) {
        node = findNodeByNodeID(nodeID);
    }
    if(!node) {
        LOG("Cannot edit: node id %d invalid.", nodeID);
        Knossos::unlockSkeleton(false);
        return false;
    }

    nodeID = node->nodeID;

    //Since the position can change, we have to rebuild the corresponding spatial skeleton structure
    /*
    delNodeFromSkeletonStruct(node);
    currentSegment = node->firstSegment;
    while(currentSegment) {
        delSegmentFromSkeletonStruct(currentSegment);
        currentSegment = currentSegment->next;
    }*/

    if(!((newXPos < 0) || (newXPos > state->boundary.x)
       || (newYPos < 0) || (newYPos > state->boundary.y)
       || (newZPos < 0) || (newZPos > state->boundary.z))) {
        SET_COORDINATE(node->position, newXPos, newYPos, newZPos);
    }

    if(newRadius != 0.) {
        node->radius = newRadius;
    }
    node->createdInMag = inMag;

    //Since the position can change, we have to rebuild the corresponding spatial skeleton structure
    /*
    addNodeToSkeletonStruct(node);
    currentSegment = node->firstSegment;
    while(currentSegment) {
        addSegmentToSkeletonStruct(currentSegment);
        currentSegment = currentSegment->next;
    }*/

    updateCircRadius(node);
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brddfdddd", KIKI_EDITNODE,
                                            state->clientState->myId,
                                            nodeID,
                                            newRadius,
                                            inMag,
                                            newXPos,
                                            newYPos,
                                            newZPos)) {

            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

void* Skeletonizer::popStack(stack *stack) {
    //The stack should hold values != NULL only, NULL indicates an error.
    void *element = NULL;

    if(stack->stackpointer >= 0) {
        element = stack->elements[stack->stackpointer];
    }
    else {
        return NULL;
    }
    stack->stackpointer--;
    stack->elementsOnStack--;

    return element;
}

bool Skeletonizer::pushStack(stack *stack, void *element) {
    if(element == NULL) {
        LOG("Stack can't hold NULL.");
        return false;
    }

    if(stack->stackpointer + 1 == stack->size) {
        stack->elements = (void**)realloc(stack->elements, stack->size * 2 * sizeof(void *));
        if(stack->elements == NULL) {
            LOG("Out of memory.");
            _Exit(false);
        }
        stack->size = stack->size * 2;
    }

    stack->stackpointer++;
    stack->elementsOnStack++;

    stack->elements[stack->stackpointer] = element;

    return true;
}

stack* Skeletonizer::newStack(int size) {
    struct stack *newStack = NULL;

    if(size <= 0) {
        LOG("That doesn't really make any sense, right? Cannot create stack with size <= 0.");
        return NULL;
    }

    newStack = (stack *)malloc(sizeof(struct stack));
    if(newStack == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(newStack, '\0', sizeof(struct stack));

    newStack->elements = (void **)malloc(sizeof(void *) * size);
    if(newStack->elements == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(newStack->elements, '\0', sizeof(void *) * size);

    newStack->size = size;
    newStack->stackpointer = -1;
    newStack->elementsOnStack = 0;

    return newStack;

}

bool Skeletonizer::delStack(stack *stack) {
    free(stack->elements);
    free(stack);

    return true;
}

bool Skeletonizer::delDynArray(dynArray *array) {
    free(array->elements);
    free(array);

    return true;
}

void* Skeletonizer::getDynArray(dynArray *array, int pos) {
    if(pos > array->end) {
        return false;
    }
    return array->elements[pos];
}

bool Skeletonizer::setDynArray(dynArray *array, int pos, void *value) {
    while(pos > array->end) {
        array->elements = (void**)realloc(array->elements, (array->end + 1 +
                                  array->firstSize) * sizeof(void *));
        if(array->elements == NULL) {
            LOG("Out of memory.");
            _Exit(false);
        }
        memset(&(array->elements[array->end + 1]), '\0', array->firstSize);
        array->end = array->end + array->firstSize;
    }

    array->elements[pos] = value;

    return true;
}

dynArray* Skeletonizer::newDynArray(int size) {
    dynArray *newArray = NULL;

    newArray = (dynArray*)malloc(sizeof(struct dynArray));
    if(newArray == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(newArray, '\0', sizeof(struct dynArray));

    newArray->elements = (void**)malloc(sizeof(void *) * size);
    if(newArray->elements == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }
    memset(newArray->elements, '\0', sizeof(void *) * size);

    newArray->end = size - 1;
    newArray->firstSize = size;

    return newArray;
}

int Skeletonizer::splitConnectedComponent(int targetRevision, int nodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    stack *remainingNodes = NULL;
    stack *componentNodes = NULL;
    nodeListElement *n = NULL, *last = NULL, *node = NULL;
    treeListElement *newTree = NULL, *currentTree = NULL;
    segmentListElement *currentEdge = NULL;
    dynArray *treesSeen = NULL;
    int treesCount = 0;
    int i = 0, id;
    PTRSIZEINT nodeCountAllTrees = 0, nodeCount = 0;
    uint visitedBase, index;
    Byte *visitedRight = NULL, *visitedLeft = NULL, **visited = NULL;
    uint visitedRightLen = 16384, visitedLeftLen = 16384, *visitedLen = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

     //  This function takes a node and splits the connected component
     //  containing that node into a new tree, unless the connected component
     //  is equivalent to exactly one entire tree.

     //  It uses depth-first search. Breadth-first-search would be the same with
     //  a queue instead of a stack for storing pending nodes. There is no
     //  practical difference between the two algorithms for this task.
     //
     //  TODO trees might become empty when the connected component spans more
     //       than one tree.


    node = findNodeByNodeID(nodeID);
    if(!node) {
        Knossos::unlockSkeleton(false);
        return false;
    }


     //  This stack can be rather small without ever having to be resized as
     //  our graphs are usually very sparse.


    remainingNodes = newStack(512);
    componentNodes = newStack(4096);


     //  This is used to keep track of which trees we've seen. The connected
     //  component for a specific node can be part of more than one tree. That
     //  makes it slightly tedious to check whether the connected component is a
     //  strict subgraph.


    treesSeen = newDynArray(16);


     //  These are used to store which nodes have been visited.
     //  Nodes with IDs smaller than the entry node will be stored in
     //  visitedLeft, nodes with equal or larger ID will be stored in
     //  visitedRight. This is done to save some memory. ;)


    visitedRight = (Byte*)malloc(16384 * sizeof(Byte));
    visitedLeft = (Byte*)malloc(16384 * sizeof(Byte));

    if(visitedRight == NULL || visitedLeft == NULL) {
        LOG("Out of memory.");
        _Exit(false);
    }

    memset(visitedLeft, NODE_PRISTINE, 16384);
    memset(visitedRight, NODE_PRISTINE, 16384);

    visitedBase = node->nodeID;
    pushStack(remainingNodes, (void *)node);

    // popStack() returns NULL when the stack is empty.
    while((n = (struct nodeListElement *)popStack(remainingNodes))) {
        if(n->nodeID >= visitedBase) {
            index = n->nodeID - visitedBase;
            visited = &visitedRight;
            visitedLen = &visitedRightLen;
        }
        else {
            index = visitedBase - n->nodeID - 1;
            visited = &visitedLeft;
            visitedLen = &visitedLeftLen;
        }

        // If the index is out bounds of the visited array, we resize the array
        while(index > *visitedLen) {
            *visited = (Byte*)realloc(*visited, (*visitedLen + 16384) * sizeof(Byte));
            if(*visited == NULL) {
                LOG("Out of memory.");
                _Exit(false);
            }

            memset(*visited + *visitedLen, NODE_PRISTINE, 16384 * sizeof(Byte));
            *visitedLen = *visitedLen + 16384;
        }

        if((*visited)[index] == NODE_VISITED) {
            continue;
        }
        (*visited)[index] = NODE_VISITED;

        pushStack(componentNodes, (void *)n);

        // Check if the node is in a tree we haven't yet seen.
        // If it is, add that tree as seen.
        for(i = 0; i < treesCount; i++) {
            if(getDynArray(treesSeen, i) == (void *)n->correspondingTree) {
                break;
            }
        }
        if(i == treesCount) {
            setDynArray(treesSeen, treesCount, (void *)n->correspondingTree);
            treesCount++;
        }

        nodeCount = nodeCount + 1;

        // And now we push all adjacent nodes on the stack.
        currentEdge = n->firstSegment;
        while(currentEdge) {
            if(currentEdge->flag == SEGMENT_FORWARD) {
                pushStack(remainingNodes, (void *)currentEdge->target);
            }
            else if(currentEdge->flag == SEGMENT_BACKWARD) {
                pushStack(remainingNodes, (void *)currentEdge->source);
            }
            currentEdge = currentEdge->next;
        }
    }

     //  If the total number of nodes visited is smaller than the sum of the
     //  number of nodes in all trees we've seen, the connected component is a
     //  strict subgraph of the graph containing all trees we've seen and we
     //  should split it.

     //  Since we're checking for treesCount > 1 below, this implementation is
     //  now slightly redundant. We want this function to not do anything when
     //  there are no disconnected components in the same tree, but create a new
     //  tree when the connected component spans several trees. This is a useful
     //  feature when performing skeleton consolidation and allows one to merge
     //  many trees at once.
     //  Just remove the treesCount > 1 below to get back to the original
     //  behaviour of only splitting strict subgraphs.

    for(i = 0; i < treesCount; i++) {
        currentTree = (struct treeListElement *)getDynArray(treesSeen, i);
        id = currentTree->treeID;
        nodeCountAllTrees += (PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                     id);
    }

    if(treesCount > 1 || nodeCount < nodeCountAllTrees) {
        color4F treeCol;
        treeCol.r = -1.;
        newTree = addTreeListElement(false, CHANGE_MANUAL, 0, treeCol);
        // Splitting the connected component.

        while((n = (struct nodeListElement *)popStack(componentNodes))) {
            // Removing node list element from its old position
            setDynArray(state->skeletonState->nodeCounter,
                        n->correspondingTree->treeID,
                        (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                         n->correspondingTree->treeID) - 1));

            if(n->previous != NULL) {
                n->previous->next = n->next;
            }
            else {
                n->correspondingTree->firstNode = n->next;
            }
            if(n->next != NULL) {
                n->next->previous = n->previous;
            }

            // Inserting node list element into new list.
            setDynArray(state->skeletonState->nodeCounter, newTree->treeID,
                        (void *)((PTRSIZEINT)getDynArray(state->skeletonState->nodeCounter,
                                                         newTree->treeID) + 1));

            n->next = NULL;

            if(last != NULL) {
                n->previous = last;
                last->next = n;
            }
            else {
                n->previous = NULL;
                newTree->firstNode = n;
            }
            last = n;
            n->correspondingTree = newTree;
        }
        state->viewerState->gui->activeTreeID = state->skeletonState->activeTree->treeID;
        state->skeletonState->skeletonChanged = true;
    }
    else {
        LOG("The connected component is equal to the entire tree, not splitting.");
    }

    delStack(remainingNodes);
    delStack(componentNodes);

    free(visitedLeft);
    free(visitedRight);

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_SPLIT_CC, nodeID)) {
            LOG("broken in splitcc.");
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return nodeCount;
}

bool Skeletonizer::addComment(int targetRevision, const char *content, nodeListElement *node, int nodeID) {
    //This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    commentListElement *newComment;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    newComment = (commentListElement*)malloc(sizeof(struct commentListElement));
    memset(newComment, '\0', sizeof(struct commentListElement));

    newComment->content = (char*)malloc(strlen(content) * sizeof(char) + 1);
    memset(newComment->content, '\0', strlen(content) * sizeof(char) + 1);

    if(nodeID) {
        node = findNodeByNodeID(nodeID);
    }
    if(node) {
        newComment->node = node;
        node->comment = newComment;
    }

    if(content) {
        strncpy(newComment->content, content, strlen(content));
        state->skeletonState->skeletonChanged = true;
    }

    if(!state->skeletonState->currentComment) {
        state->skeletonState->currentComment = newComment;
        //We build a circular linked list
        newComment->next = newComment;
        newComment->previous = newComment;
    }
    else {
        //We insert into a circular linked list
        state->skeletonState->currentComment->previous->next = newComment;
        newComment->next = state->skeletonState->currentComment;
        newComment->previous = state->skeletonState->currentComment->previous;
        state->skeletonState->currentComment->previous = newComment;

        state->skeletonState->currentComment = newComment;
    }


    //write into commentBuffer, so that comment appears in comment text field when added via Shortcut
    memset(state->skeletonState->commentBuffer, '\0', 10240);
    strncpy(state->skeletonState->commentBuffer,
            state->skeletonState->currentComment->content,
            strlen(state->skeletonState->currentComment->content));



    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrds", KIKI_ADDCOMMENT, node->nodeID, content)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::delComment(int targetRevision, commentListElement *currentComment, int commentNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    int nodeID = 0;
    nodeListElement *commentNode = NULL;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(commentNodeID) {
        commentNode = findNodeByNodeID(commentNodeID);
        if(commentNode) {
            currentComment = commentNode->comment;
        }
    }

    if(!currentComment) {
        LOG("Please provide a valid comment element to delete!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(currentComment->content) {
        free(currentComment->content);
    }
    if(currentComment->node) {
        nodeID = currentComment->node->nodeID;
        currentComment->node->comment = NULL;
        state->skeletonState->skeletonChanged = true;
    }

    if(state->skeletonState->currentComment == currentComment) {
        memset(state->skeletonState->commentBuffer, '\0', 10240);
    }

    if(currentComment->next == currentComment) {
        state->skeletonState->currentComment = NULL;
    }
    else {
        currentComment->next->previous = currentComment->previous;
        currentComment->previous->next = currentComment->next;

        if(state->skeletonState->currentComment == currentComment) {
            state->skeletonState->currentComment = currentComment->next;
        }
    }

    free(currentComment);

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_DELCOMMENT, nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::editComment(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    // this function also seems to be kind of useless as you could do just the same
    // thing with addComment() with minimal changes ....?

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(nodeID) {
        currentComment = findNodeByNodeID(nodeID)->comment;
    }
    if(!currentComment) {
        LOG("Please provide a valid comment element to edit!");
        Knossos::unlockSkeleton(false);
        return false;
    }

    nodeID = currentComment->node->nodeID;

    if(newContent) {
        if(currentComment->content) {
            //free(currentComment->content);
        }
        currentComment->content = (char*)malloc(strlen(newContent) * sizeof(char) + 1);
        memset(currentComment->content, '\0', strlen(newContent) * sizeof(char) + 1);
        strncpy(currentComment->content, newContent, strlen(newContent));

        //write into commentBuffer, so that comment appears in comment text field when added via Shortcut
        /*
        memset(state->skeletonState->commentBuffer, '\0', 10240);
        strncpy(state->skeletonState->commentBuffer,
                state->skeletonState->currentComment->content,
                strlen(state->skeletonState->currentComment->content));
        */
    }

    if(newNodeID) {
        newNode = findNodeByNodeID(newNodeID);
    }
    if(newNode) {
        if(currentComment->node) {
            currentComment->node->comment = NULL;
        }
        currentComment->node = newNode;
        newNode->comment = currentComment;
    }

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("blrdds",
                    KIKI_EDITCOMMENT,
                    nodeID,
                    currentComment->node->nodeID,
                    newContent)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}
commentListElement* Skeletonizer::nextComment(char *searchString) {
   commentListElement *firstComment, *currentComment;

    if(!strlen(searchString)) {
        //->previous here because it would be unintuitive for the user otherwise.
        //(we insert new comments always as first elements)
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->previous;
            setActiveNode(CHANGE_MANUAL,
                          state->skeletonState->currentComment->node,
                          0);
            jumpToActiveNode();
        }
    }
    else {
        if(state->skeletonState->currentComment) {
            firstComment = state->skeletonState->currentComment->previous;
            currentComment = firstComment;
            do {
                if(strstr(currentComment->content, searchString) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(CHANGE_MANUAL,
                                  state->skeletonState->currentComment->node,
                                  0);
                    jumpToActiveNode();
                    break;
                }
                currentComment = currentComment->previous;

            } while (firstComment != currentComment);
        }
    }

    memset(state->skeletonState->commentBuffer, '\0', 10240);

    if(state->skeletonState->currentComment) {
        strncpy(state->skeletonState->commentBuffer,
                state->skeletonState->currentComment->content,
                strlen(state->skeletonState->currentComment->content));

        if(state->skeletonState->lockPositions) {
            if(strstr(state->skeletonState->commentBuffer, state->skeletonState->onCommentLock)) {
                lockPosition(state->skeletonState->currentComment->node->position);
            }
            else {
                unlockPosition();
            }
        }

    }

    emit drawGUISignal();

    return state->skeletonState->currentComment;
}

commentListElement* Skeletonizer::previousComment(char *searchString) {
    commentListElement *firstComment, *currentComment;
    // ->next here because it would be unintuitive for the user otherwise.
    // (we insert new comments always as first elements)

    if(!strlen(searchString)) {
        if(state->skeletonState->currentComment) {
            state->skeletonState->currentComment = state->skeletonState->currentComment->next;
            setActiveNode(CHANGE_MANUAL,
                          state->skeletonState->currentComment->node,
                          0);
            jumpToActiveNode();
        }
    }
    else {
        if(state->skeletonState->currentComment) {
            firstComment = state->skeletonState->currentComment->next;
            currentComment = firstComment;
            do {
                if(strstr(currentComment->content, searchString) != NULL) {
                    state->skeletonState->currentComment = currentComment;
                    setActiveNode(CHANGE_MANUAL,
                                  state->skeletonState->currentComment->node,
                                  0);
                    jumpToActiveNode();
                    break;
                }
                currentComment = currentComment->next;

            } while (firstComment != currentComment);

        }
    }
    memset(state->skeletonState->commentBuffer, '\0', 10240);

    if(state->skeletonState->currentComment) {
        strncpy(state->skeletonState->commentBuffer,
            state->skeletonState->currentComment->content,
            strlen(state->skeletonState->currentComment->content));

        if(state->skeletonState->lockPositions) {
            if(strstr(state->skeletonState->commentBuffer, state->skeletonState->onCommentLock)) {
                lockPosition(state->skeletonState->currentComment->node->position);
            }
            else {
                unlockPosition();
            }
        }
    }
    emit drawGUISignal();
    return state->skeletonState->currentComment;
}

bool Skeletonizer::searchInComment(char *searchString, commentListElement *comment) {
    return true;
}

bool Skeletonizer::unlockPosition() {
    if(state->skeletonState->positionLocked) {
        LOG("Spatial locking disabled.");
    }
    state->skeletonState->positionLocked = false;

    return true;
}

bool Skeletonizer::lockPosition(Coordinate lockCoordinate) {
    LOG("locking to (%d, %d, %d).", lockCoordinate.x, lockCoordinate.y, lockCoordinate.z);
    state->skeletonState->positionLocked = true;
    SET_COORDINATE(state->skeletonState->lockedPosition,
                   lockCoordinate.x,
                   lockCoordinate.y,
                   lockCoordinate.z);

    return true;
}

/* @todo loader gets out of sync (endless loop) */
bool Skeletonizer::popBranchNode(int targetRevision) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.
    // SYNCHRO BUG:
    // both instances will have to confirm branch point deletion if
    // confirmation is asked.

    nodeListElement *branchNode = NULL;
    PTRSIZEINT branchNodeID = 0;

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    // Nodes on the branch stack may not actually exist anymore
    while(true){
        if (branchNode != NULL)
            if (branchNode->isBranchNode == true) {
                break;
            }
        branchNodeID = (PTRSIZEINT)popStack(state->skeletonState->branchStack);
        if(branchNodeID == 0) {
            // AGAR AG_TextMsg(AG_MSG_INFO, "No branch points remain.");
            LOG("No branch points remain.");

            goto exit_popbranchnode;
        }

        branchNode = findNodeByNodeID(branchNodeID);
    }

    if(branchNode && branchNode->isBranchNode) {
#if QT_POINTER_SIZE == 8
        LOG("Branch point (node ID %"PRId64") deleted.", branchNodeID);
#else
        LOG("Branch point (node ID %d) deleted.", branchNodeID);
#endif


        setActiveNode(CHANGE_NOSYNC, branchNode, 0);

        branchNode->isBranchNode--;
        state->skeletonState->skeletonChanged = true;

        emit userMoveSignal(branchNode->position.x - state->viewerState->currentPosition.x,
                            branchNode->position.y - state->viewerState->currentPosition.y,
                            branchNode->position.z - state->viewerState->currentPosition.z,
                            TELL_COORDINATE_CHANGE);

        state->skeletonState->branchpointUnresolved = true;
    }

exit_popbranchnode:

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("br", KIKI_POPBRANCH)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }

    Knossos::unlockSkeleton(true);
    return true;
}

bool Skeletonizer::pushBranchNode(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    if(branchNodeID) {
        branchNode = findNodeByNodeID(branchNodeID);
    }
    if(branchNode) {
        if(branchNode->isBranchNode == 0 || !checkDoubleBranchpoint) {
            pushStack(state->skeletonState->branchStack, (void *)(PTRSIZEINT)branchNode->nodeID);
            if(setBranchNodeFlag) {
                branchNode->isBranchNode = true;
            }
            state->skeletonState->skeletonChanged = true;
            LOG("Branch point (node ID %d) added.", branchNode->nodeID);

        }
        else {
            LOG("Active node is already a branch point");
            Knossos::unlockSkeleton(true);
            return true;
        }
    }
    else {
        LOG("Make a node active before adding branch points.");
        Knossos::unlockSkeleton(true);
        return true;
    }

    state->skeletonState->unsavedChanges = true;
    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_PUSHBRANCH, branchNode->nodeID)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::setSkeletonWorkMode(int targetRevision, uint workMode) {
    // This is a SYNCHRONIZABLE skeleton function. Be a bit careful.

    if(Knossos::lockSkeleton(targetRevision) == false) {
        Knossos::unlockSkeleton(false);
        return false;
    }

    state->skeletonState->workMode = workMode;

    state->skeletonState->skeletonRevision++;

    if(targetRevision == CHANGE_MANUAL) {
        if(!Client::syncMessage("brd", KIKI_SETSKELETONMODE, workMode)) {
            Client::skeletonSyncBroken();
        }
    }
    else {
        Viewer::refreshViewports();
    }
    Knossos::unlockSkeleton(true);

    return true;
}

bool Skeletonizer::jumpToActiveNode() {
    if(state->skeletonState->activeNode) {
        emit userMoveSignal(state->skeletonState->activeNode->position.x - state->viewerState->currentPosition.x,
                            state->skeletonState->activeNode->position.y - state->viewerState->currentPosition.y,
                            state->skeletonState->activeNode->position.z - state->viewerState->currentPosition.z,
                            TELL_COORDINATE_CHANGE);
    }

    return true;
}

void Skeletonizer::UI_popBranchNode() {
    // Inconsistency:
    // Confirmation will not be asked when no branch points remain, except if the remaining
    //  branch point nodes don't exist anymore (nodes have been deleted).

    // This is workaround around agar bug #171
    if(state->skeletonState->askingPopBranchConfirmation == false) {
        state->skeletonState->askingPopBranchConfirmation = true;

        if(state->skeletonState->branchpointUnresolved && state->skeletonState->branchStack->stackpointer != -1) {
            //yesNoPrompt(NULL,
            //            "No node was added after jumping to the last branch point. Do you really want to jump?",
            //            WRAP_popBranchNode,
            //            popBranchNodeCanceled);

        }
        else {
            WRAP_popBranchNode();
        }
    }
}

void Skeletonizer::restoreDefaultTreeColor() {
    int index = (state->skeletonState->activeTree->treeID - 1) % 256;
    state->skeletonState->activeTree->color.r = state->viewerState->defaultTreeTable[index];
    state->skeletonState->activeTree->color.g = state->viewerState->defaultTreeTable[index + 256];
    state->skeletonState->activeTree->color.b = state->viewerState->defaultTreeTable[index + 512];
    state->skeletonState->activeTree->color.a = 1.;

    state->skeletonState->activeTree->colorSetManually = false;
    state->skeletonState->skeletonChanged = true;
    state->skeletonState->unsavedChanges = true;
}

bool Skeletonizer::updateTreeColors() {
    treeListElement *tree = state->skeletonState->firstTree;
    while(tree) {
        uint index = (tree->treeID - 1) % 256;
        tree->color.r = state->viewerState->treeAdjustmentTable[index];
        tree->color.g = state->viewerState->treeAdjustmentTable[index +  256];
        tree->color.b = state->viewerState->treeAdjustmentTable[index + 512];
        tree->color.a = 1.;
        tree = tree->next;
    }
    state->skeletonState->skeletonChanged = true;
    return true;
}


bool Skeletonizer::updateCircRadius(nodeListElement *node) {

    struct segmentListElement *currentSegment = NULL;
      node->circRadius = node->radius;

      /* Any segment longer than the current circ radius?*/
      currentSegment = node->firstSegment;
      while(currentSegment) {
          if(currentSegment->length > node->circRadius)
              node->circRadius = currentSegment->length;
          currentSegment = currentSegment->next;
      }

      return true;

}

int Skeletonizer::xorInt(int xorMe) {
    return xorMe ^ (int) 5642179165;
}

void Skeletonizer::setColorFromNode(nodeListElement *node, color4F *color) {
    int nr;

        if(node->isBranchNode) { //branch nodes are always blue
            SET_COLOR((*color), 0.f, 0.f, 1.f, 1.f);
            return;
        }

        if(node->comment != NULL) {
            // default color for comment nodes
            SET_COLOR((*color), 1.f, 1.f, 0.f, 1.f);

            if(state->skeletonState->userCommentColoringOn == false) {
                // conditional node coloring is disabled
                // comment nodes have default color, return
                return;
            }

            if((nr = commentContainsSubstr(node->comment, -1)) == -1) {
                //no substring match => keep default color and return
                return;
            }
            if(state->skeletonState->commentColors[nr].a > 0.f) {
                //substring match, change color
                *color = state->skeletonState->commentColors[nr];
            }
        }
        return;
}

void Skeletonizer::setRadiusFromNode(nodeListElement *node, float *radius) {
    int nr;

        if(state->skeletonState->overrideNodeRadiusBool)
            *radius = state->skeletonState->overrideNodeRadiusVal;
        else
            *radius = node->radius;

        if(node->comment != NULL) {
            if(state->skeletonState->commentNodeRadiusOn == false) {
                // conditional node radius is disabled
                // return
                return;
            }

            if((nr = commentContainsSubstr(node->comment, -1)) == -1) {
                //no substring match => keep default radius and return
                return;
            }

            if(state->skeletonState->commentNodeRadii[nr] > 0.f) {
                //substring match, change radius
                *radius = state->skeletonState->commentNodeRadii[nr];
            }
        }

}

bool Skeletonizer::delSkelState(struct skeletonState *skelState) {
    if(skelState == NULL) {
        return false;
    }
    delTreesFromState(skelState);
    Hashtable::ht_rmtable(skelState->skeletonDCs);
    free(skelState->searchStrBuffer);
    free(skelState->prevSkeletonFile);
    free(skelState->skeletonFile);
    free(skelState);
    skelState = NULL;

    return true;
}

bool Skeletonizer::delTreesFromState(struct skeletonState *skelState) {
    struct treeListElement *current;
    struct treeListElement *treeToDel;

    if(skelState->firstTree == NULL) {
        return false;
    }
    current = skelState->firstTree;
    while(current) {
        treeToDel = current;
        current = current->next;
        delTreeFromState(treeToDel, skelState);
    }
    skelState->treeElements = 0;
    skelState->firstTree = NULL;
    skelState->activeTree = NULL;
    skelState->activeNode = NULL;
    skelState->greatestTreeID = 0;
    skelState->greatestNodeID = 0;
    delStack(skelState->branchStack);
    skelState->branchStack = NULL;
    delDynArray(skelState->nodeCounter);
    skelState->nodeCounter = NULL;
    delDynArray(skelState->nodesByNodeID);
    skelState->nodesByNodeID = NULL;
    free(skelState->commentBuffer);
    skelState->commentBuffer = NULL;
    free(skelState->currentComment);
    skelState->currentComment = NULL;

    return true;
}

bool Skeletonizer::delTreeFromState(treeListElement *treeToDel, skeletonState *skelState) {
    nodeListElement *currentNode = NULL;
    nodeListElement *nodeToDel = NULL;

    if(treeToDel == NULL) {
        return false;
    }

    currentNode = treeToDel->firstNode;
    while(currentNode) {
        nodeToDel = currentNode;
        currentNode = nodeToDel->next;
        delNodeFromState(nodeToDel, skelState);
    }
    treeToDel->firstNode = NULL;

    if(treeToDel->previous) {
        treeToDel->previous->next = treeToDel->next;
    }
    if(treeToDel->next) {
        treeToDel->next->previous = treeToDel->previous;
    }
    free(treeToDel);
    return true;
}

bool Skeletonizer::hasObfuscatedTime() {
    int major = 0, minor = 0;
    char point;

    sscanf(state->skeletonState->skeletonCreatedInVersion, "%d%c%d", &major, &point, &minor);

    if(major > 3) {
        return true;
    }
    if(major == 3 && minor >= 4) {
        return true;
    }
    return false;
}

bool Skeletonizer::delNodeFromState(struct nodeListElement *nodeToDel, struct skeletonState *skelState) {
    struct segmentListElement *currentSegment;
    struct segmentListElement *tempNext;

    if(nodeToDel == NULL) {
        return false;
    }

    if(nodeToDel->comment) {
        delCommentFromState(nodeToDel->comment, skelState);
    }

    /*
     * First, delete all segments pointing towards and away of the nodeToDelhas
     * been */

    currentSegment = nodeToDel->firstSegment;
    while(currentSegment) {
        tempNext = currentSegment->next;
        if(currentSegment->flag == SEGMENT_FORWARD) {
            delSegmentFromCmd(currentSegment);
        }
        else if(currentSegment->flag == SEGMENT_BACKWARD) {
            delSegmentFromCmd(currentSegment->reverseSegment);
        }
        currentSegment = tempNext;
    }
    nodeToDel->firstSegment = NULL;

    //TDitem: is this necessary for undo?
    if(nodeToDel == nodeToDel->correspondingTree->firstNode) {
        nodeToDel->correspondingTree->firstNode = nodeToDel->next;
    }
    else {
        if(nodeToDel->previous) {
            nodeToDel->previous->next = nodeToDel->next;
        }
        if(nodeToDel->next) {
            nodeToDel->next->previous = nodeToDel->previous;
        }
    }
    free(nodeToDel);
    return true;
}

bool Skeletonizer::delCommentFromState(struct commentListElement *commentToDel, struct skeletonState *skelState) {
    int nodeID = 0;

    if(commentToDel == NULL) {
        return false;
    }
    if(commentToDel->content) {
        free(commentToDel->content);
    }
    if(commentToDel->node) {
        nodeID = commentToDel->node->nodeID;
        commentToDel->node->comment = NULL;
    }

    if(commentToDel->next == commentToDel) { //only comment in list
        skelState->currentComment = NULL;
    }
    else {
        commentToDel->next->previous = commentToDel->previous;
        commentToDel->previous->next = commentToDel->next;
        if(skelState->currentComment == commentToDel) {
            skelState->currentComment = commentToDel->next;
        }
    }
    free(commentToDel);
    return true;
}

bool Skeletonizer::delSegmentFromCmd(struct segmentListElement *segToDel) {
    if(!segToDel) {
        return false;
    }
    /* numSegs counts forward AND backward segments!!! */
    segToDel->source->numSegs--;
    segToDel->target->numSegs--;

    if(segToDel == segToDel->source->firstSegment)
        segToDel->source->firstSegment = segToDel->next;
    else {
        //if(segToDel->previous) //Why??? Previous EXISTS if its not the first seg...
            segToDel->previous->next = segToDel->next;
        if(segToDel->next) {
            segToDel->next->previous = segToDel->previous;
        }
    }

    //Delete reverse segment in target node
    if(segToDel->reverseSegment == segToDel->target->firstSegment) {
        segToDel->target->firstSegment = segToDel->reverseSegment->next;
    }
    else {
        //if(segToDel->reverseSegment->previous)
            segToDel->reverseSegment->previous->next = segToDel->reverseSegment->next;
        if(segToDel->reverseSegment->next)
            segToDel->reverseSegment->next->previous = segToDel->reverseSegment->previous;
    }
    /* A bit cumbersome, but we cannot delete the segment and then find its source node.. */
    segToDel->length = 0.f;
    updateCircRadius(segToDel->source);

    free(segToDel);
    return true;
}

bool Skeletonizer::moveToPrevTree() {
    /*
    struct treeListElement *prevTree = getTreeWithPrevID(state->skeletonState->activeTree);
    struct nodeListElement *node;
    if(state->skeletonState->activeTree == NULL) {
        return false;
    }
    if(prevTree) {
        setActiveTreeByID(prevTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;
        if(node == NULL) {
            return true;
        }
        else {
            setActiveNode(CHANGE_MANUAL, node, node->nodeID);

            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                           node->position.x,
                           node->position.y,
                           node->position.z);
            Knossos::sendRemoteSignal();
        }
        return true;
    }
    LOG("Reached first tree.");
    */
    return false;
}

bool Skeletonizer::moveToNextTree() {
    /*
    struct treeListElement *nextTree = getTreeWithNextID(state->skeletonState->activeTree);
    struct nodeListElement *node;

    if(state->skeletonState->activeTree == NULL) {
        return false;
    }
    if(nextTree) {
        setActiveTreeByID(nextTree->treeID);
        //set tree's first node to active node if existent
        node = state->skeletonState->activeTree->firstNode;
        if(node == NULL) {
            return true;
        }
        else {
            setActiveNode(CHANGE_MANUAL, node, node->nodeID);
            SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                           node->position.x,
                           node->position.y,
                           node->position.z);
            sendRemoteSignal();
        }
        return true;
    }
    LOG("Reached last tree.");
    */
    return false;

}

bool Skeletonizer::moveToPrevNode() {
    /*
    struct nodeListElement *prevNode = getNodeWithPrevID(state->skeletonState->activeNode);

    if(state->skeletonState->activeNode == NULL) {
        return FALSE;
    }
    if(prevNode) {
        setActiveNode(CHANGE_MANUAL, prevNode, prevNode->nodeID);
        tempConfig->remoteState->type = REMOTE_RECENTERING;
        SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                       prevNode->position.x,
                       prevNode->position.y,
                       prevNode->position.z);
        sendRemoteSignal();
        return TRUE;
    }
    */
    return false;
}

bool Skeletonizer::moveToNextNode() {
    /*
    struct nodeListElement *nextNode = getNodeWithNextID(state->skeletonState->activeNode);

    if(state->skeletonState->activeNode == NULL) {
        return FALSE;
    }
    if(nextNode) {
        setActiveNode(CHANGE_MANUAL, nextNode, nextNode->nodeID);
        tempConfig->remoteState->type = REMOTE_RECENTERING;
        SET_COORDINATE(tempConfig->remoteState->recenteringPosition,
                       nextNode->position.x,
                       nextNode->position.y,
                       nextNode->position.z);
        sendRemoteSignal();
        return TRUE;
    }
    */
    return false;

}

// index optionally specifies substr, range is [-1, NUM_COMMSUBSTR - 1].
// If -1, all substrings are compared against the comment.
unsigned int Skeletonizer::commentContainsSubstr(struct commentListElement *comment, int index) {
    int i;

    if(!comment) {
        return -1;
    }
    if(index == -1) { //no index specified
        for(i = 0; i < NUM_COMMSUBSTR; i++) {
            if(strlen(state->viewerState->gui->commentSubstr[i]) > 0
                && strstr(comment->content, state->viewerState->gui->commentSubstr[i]) != NULL) {
                return i;
            }
        }
    }
    else if(index > -1 && index < NUM_COMMSUBSTR) {
        if(strlen(state->viewerState->gui->commentSubstr[index]) > 0
           && strstr(comment->content, state->viewerState->gui->commentSubstr[index]) != NULL) {
            return index;
        }
    }
    return -1;
}

void Skeletonizer::refreshUndoRedoBuffers() {
    struct treeListElement *testingTree;
    struct treeListElement *copy = state->skeletonState->activeTree;
    state->skeletonState->undoList->lastCmd->cmd = testingTree;
    state->skeletonState->undoList->lastCmd->next;
}

void Skeletonizer::undo() {
    struct cmdListElement *redoCmdEl = NULL;
    struct cmdListElement *cmdEl = NULL;

    cmdDelTree *delTree; cmdAddTree *addTree; cmdSplitTree *splitTree, *redoSplitTree; cmdMergeTree *mergeTree;
    cmdChangeTreeColor *chgTreeColor; cmdChangeActiveTree *chgActiveTree;
    cmdDelNode *delNode; cmdAddNode *addNode; cmdLinkNode *linkNode; cmdUnlinkNode *unlinkNode;
    cmdPushBranchNode *pushBranch; cmdPopBranch *popBranch; cmdChangeActiveNode *chgActiveNode;
    cmdDelComment *delComment; cmdAddComment *addComment; cmdChangeComment *chgComment;

    cmdEl = popCmd(state->skeletonState->undoList);
    if(cmdEl == NULL) {
        return; //nothing to undo
    }

    switch(cmdEl->cmdType) {
    case CMD_DELTREE:
        break;
    case CMD_ADDTREE:
        break;
    case CMD_SPLITTREE:
        //first add to redo-list cmd holding current skeletonState
        redoSplitTree = (cmdSplitTree *)malloc(sizeof(cmdSplitTree));
        //redoSplitTree->skelState = copySkelState(state->skeletonState);
        redoCmdEl = (cmdListElement *)malloc(sizeof(struct cmdListElement));
        redoCmdEl->cmdType = CMD_SPLITTREE;
        redoCmdEl->cmd = redoSplitTree;
        pushCmd(state->skeletonState->redoList, redoCmdEl);

        //then undo
        delSkelState(state->skeletonState);
        splitTree = (cmdSplitTree*) cmdEl->cmd;
        //state->skeletonState = copySkelState(splitTree->skelState);
        delCmdListElement(cmdEl);
        break;
    case CMD_MERGETREE:
        break;
    case CMD_CHGTREECOL:
        break;
    case CMD_CHGACTIVETREE:
        break;
    case CMD_DELNODE:
        break;
    case CMD_ADDNODE:
        break;
    case CMD_LINKNODE:
        break;
    case CMD_UNLINKNODE:
        break;
    case CMD_PUSHBRANCH:
        break;
    case CMD_POPBRANCH:
        break;
    case CMD_CHGACTIVENODE:
        break;
    case CMD_ADDCOMMENT:
        break;
    case CMD_CHGCOMMENT:
        break;
    case CMD_DELCOMMENT:
        break;
    default:
        LOG("no matching command");
    }
}

void Skeletonizer::redo() {
    struct cmdListElement *cmdEl = state->skeletonState->redoList->lastCmd;

    if(cmdEl == NULL) {
        return; //nothing to redo
    }

    cmdDelTree *delTree; cmdAddTree *addTree; cmdSplitTree *splitTree; cmdMergeTree *mergeTree;
    cmdChangeTreeColor *chgTreeColor; cmdChangeActiveTree *chgActiveTree;
    cmdDelNode *delNode; cmdAddNode *addNode; cmdLinkNode *linkNode; cmdUnlinkNode *unlinkNode;
    cmdPushBranchNode *pushBranch; cmdPopBranch *popBranch; cmdChangeActiveNode *chgActiveNode;
    cmdDelComment *delComment; cmdAddComment *addComment; cmdChangeComment *chgComment;

    switch(cmdEl->cmdType) {
    case CMD_DELTREE:
        break;
    case CMD_ADDTREE:
        break;
    case CMD_SPLITTREE:
        splitTree = (cmdSplitTree*) cmdEl->cmd;
        break;
    case CMD_MERGETREE:
        break;
    case CMD_CHGTREECOL:
        break;
    case CMD_CHGACTIVETREE:
        break;
    case CMD_DELNODE:
        break;
    case CMD_ADDNODE:
        break;
    case CMD_LINKNODE:
        break;
    case CMD_UNLINKNODE:
        break;
    case CMD_PUSHBRANCH:
        break;
    case CMD_POPBRANCH:
        break;
    case CMD_CHGACTIVENODE:
        break;
    case CMD_ADDCOMMENT:
        break;
    case CMD_CHGCOMMENT:
        break;
    case CMD_DELCOMMENT:
        break;
    default:
        LOG("no matching command");
    }
    //remove cmd from redo-list and add to undo-list
    state->skeletonState->redoList->lastCmd = cmdEl->prev;
    if(cmdEl->prev) {
        cmdEl->prev->next = NULL;
    }
    cmdEl->prev = state->skeletonState->undoList->lastCmd;
    state->skeletonState->undoList->lastCmd->next = cmdEl;
    state->skeletonState->undoList->lastCmd = cmdEl;
}

//get last command
struct cmdListElement *Skeletonizer::popCmd(struct cmdList *cmdList) {
    struct cmdListElement *returnCmd = cmdList->lastCmd;

    if(returnCmd == NULL) {
        return NULL;
    }

    cmdList->lastCmd = returnCmd->prev;
    if(returnCmd->prev) {
        returnCmd->prev->next = NULL;
    }
    else { //was first in list
        cmdList->firstCmd = NULL;
    }
    cmdList->cmdCount--;

    return returnCmd;
}

void Skeletonizer::pushCmd(struct cmdList *cmdList, struct cmdListElement *cmdListEl) {
    struct cmdListElement *cmdToDel = NULL;

    if(cmdList->firstCmd == NULL) { //list empty
        cmdList->firstCmd = cmdListEl;
        cmdList->lastCmd = cmdListEl;
        cmdListEl->next = NULL;
        cmdListEl->prev = NULL;
        cmdList->cmdCount = 1;
    }
    else {
        cmdList->lastCmd->next = cmdListEl;
        cmdListEl->prev = cmdList->lastCmd;
        cmdListEl->next = NULL;
        cmdList->lastCmd = cmdListEl;
        if(cmdList->cmdCount < CMD_MAXSTEPS) {  //list not full
            cmdList->cmdCount++;
        }
        else { //list is full, remove oldest cmd (only happens for undo list)
            cmdToDel = cmdList->firstCmd;
            cmdList->firstCmd = cmdToDel->next;
            cmdList->firstCmd->prev = NULL;
            delCmdListElement(cmdToDel);
        }
    }
}

void Skeletonizer::flushCmdList(struct cmdList *cmdList) {
    struct cmdListElement *elToDel = NULL;
    struct cmdListElement *nextEl = NULL;

    if(cmdList->firstCmd == NULL) {
        return;
    }

    elToDel = cmdList->lastCmd;
    while(elToDel) {
        nextEl = elToDel->prev;
        delCmdListElement(elToDel);
        elToDel = nextEl;
    }

    cmdList->cmdCount = 0;
    cmdList->firstCmd = NULL;
    cmdList->lastCmd = NULL;
}

// also sets pointer to NULL for you
void Skeletonizer::delCmdListElement(struct cmdListElement *cmdEl) {
    cmdSplitTree *splitTreeCMD;

    switch(cmdEl->cmdType) {
    case CMD_DELTREE:
        break;
    case CMD_ADDTREE:
        break;
    case CMD_SPLITTREE:
        splitTreeCMD = (cmdSplitTree*) cmdEl->cmd;
        delSkelState(splitTreeCMD->skelState);
        free(splitTreeCMD);
        free(cmdEl);
        cmdEl = NULL;
        break;
    case CMD_MERGETREE:
        break;
    case CMD_CHGTREECOL:
        break;
    case CMD_CHGACTIVETREE:
        break;
    case CMD_DELNODE:
        break;
    case CMD_ADDNODE:
        break;
    case CMD_LINKNODE:
        break;
    case CMD_UNLINKNODE:
        break;
    case CMD_PUSHBRANCH:
        break;
    case CMD_POPBRANCH:
        break;
    case CMD_CHGACTIVENODE:
        break;
    case CMD_ADDCOMMENT:
        break;
    case CMD_CHGCOMMENT:
        break;
    case CMD_DELCOMMENT:
        break;
    default:
        LOG("no matching command");
    }
}

void Skeletonizer::setZoomLevel(float value) {
    this->zoomLevel = value;
}

void Skeletonizer::setShowIntersections(bool on) {
    this->showIntersections = on;
}

void Skeletonizer::setShowXyPlane(bool on) {
    this->showXYplane = on;
}

void Skeletonizer::setRotateAroundActiveNode(bool on) {
    this->rotateAroundActiveNode = on;
}

void Skeletonizer::setOverrideNodeRadius(bool on) {
    this->overrideNodeRadiusBool = on;
}

void Skeletonizer::setSegRadiusToNodeRadius(float value) {
    this->segRadiusToNodeRadius = value;
}

void Skeletonizer::setHighlightActiveTree(bool on) {
    this->highlightActiveTree = on;
}

void Skeletonizer::setSkeletonChanged(bool on) {
    this->skeletonChanged = on;
}

void Skeletonizer::setShowNodeIDs(bool on) {
    this->showNodeIDs = on;
}

