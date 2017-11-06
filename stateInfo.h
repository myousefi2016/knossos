/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef STATE_INFO_H
#define STATE_INFO_H

#include "coordinate.h"
#include "hashtable.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QString>
#include <QWaitCondition>

#include <vector>

class stateInfo;
extern stateInfo * state;

#define NUM_MAG_DATASETS 65536

// Bytes for an object ID.
#define OBJID_BYTES sizeof(uint64_t)

//custom tail recursive constexpr log implementation
//required for the following array declarations/accesses: (because std::log2 isn’t required to be constexpr yet)
//  magPaths, magNames, magBoundaries, Dc2Pointer, Oc2Pointer
//TODO to be removed when the above mentioned variables vanish
//integral return value for castless use in subscripts
template<typename T>
constexpr std::size_t int_log(const T val, const T base = 2, const std::size_t res = 0) {
    return val < base ? res : int_log(val/base, base, res+1);
}

/**
  * @stateInfo
  * @brief stateInfo holds everything we need to know about the current instance of Knossos
  *
  * It gets instantiated in the main method of knossos.cpp and referenced in almost all important files and classes below the #includes with extern  stateInfo
  */
class stateInfo {
public:
    //  Info about the data
    bool gpuSlicer = false;

    bool quitSignal{false};

    // Bytes in one datacube: 2^3N
    std::size_t cubeBytes;

    // Area of a cube slice in pixels;
    int cubeSliceArea;

    // Supercube edge length in datacubes.
    int M;
    std::size_t cubeSetElements;

    // Bytes in one supercube (This is pretty much the memory
    // footprint of KNOSSOS): M^3 * 2^3M
    std::size_t cubeSetBytes;

    // With 2^N being the edge length of a datacube in pixels and
    // M being the edge length of a supercube (the set of all
    // simultaneously loaded datacubes) in datacubes:

// --- Inter-thread communication structures / signals / mutexes, etc. ---

    // ANY access to the Dc2Pointer or Oc2Pointer tables has
    // to be locked by this mutex.
    QMutex protectCube2Pointer;

 //---  Info about the state of KNOSSOS in general. --------

    // Dc2Pointer and Oc2Pointer provide a mappings from cube
    // coordinates to pointers to datacubes / overlay cubes loaded
    // into memory.
    // It is a set of key (cube coordinate) / value (pointer) pairs.
    // Whenever we access a datacube in memory, we do so through
    // this structure.
    std::vector<std::vector<coord2bytep_map_t>> cube2Pointer{decltype(cube2Pointer)(2, decltype(cube2Pointer)::value_type(int_log(NUM_MAG_DATASETS)+1))};// FIXME legacy init
    int segmentationLayerId{1};

    struct ViewerState * viewerState;
    class MainWindow * mainWindow{nullptr};
    class Viewer * viewer;
    class Scripting * scripting;
    class SignalRelay * signalRelay;
    struct SkeletonState * skeletonState;
};

#endif//STATE_INFO_H
