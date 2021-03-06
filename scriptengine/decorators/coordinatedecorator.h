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

#ifndef COORDINATEDECORATOR_H
#define COORDINATEDECORATOR_H

#include "coordinate.h"

#include <QObject>

class CoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit CoordinateDecorator(QObject *parent = 0);


signals:

public slots:

    int x(Coordinate *self);
    int y(Coordinate *self);
    int z(Coordinate *self);
    QVector<int> vector(Coordinate *self);

    QString static_Coordinate_help();


};

#endif // COORDINATEDECORATOR_H
