/*
 * (c)2011 BlueBolt Ltd. All rights reserved.
 *
 * SceneHelper.h
 *
 *  Created on: 15 Dec 2011
 *      Author: ashley-r
 */

#ifndef SCENEHELPER_H_
#define SCENEHELPER_H_


#include <Alembic/Abc/IArrayProperty.h>
#include <Alembic/Abc/IScalarProperty.h>
#include <Alembic/Abc/IObject.h>

//#include <Alembic/AbcGeom/ICurves.h>
//#include <Alembic/AbcGeom/INuPatch.h>
//#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>
#include <Alembic/AbcGeom/IXform.h>

#include <vector>
#include <string>

class PolyMeshAndColors
{
public:
    Alembic::AbcGeom::IPolyMesh mMesh;
    std::vector< Alembic::AbcGeom::IC3fGeomParam > mC3s;
    std::vector< Alembic::AbcGeom::IC4fGeomParam > mC4s;
};

class SubDAndColors
{
public:
    Alembic::AbcGeom::ISubD mMesh;
    std::vector< Alembic::AbcGeom::IC3fGeomParam > mC3s;
    std::vector< Alembic::AbcGeom::IC4fGeomParam > mC4s;
};


#endif /* SCENEHELPER_H_ */
