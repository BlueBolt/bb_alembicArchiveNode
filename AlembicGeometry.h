/* (c)2012 BlueBolt Ltd. All rights reserved.
 *
 * AlembicGeometry.h
 *
 *  Created on: 21 Aug 2012
 *      Author: ashley-r
 */

#ifndef ALEMBICGEOMETRY_H_
#define ALEMBICGEOMETRY_H_

#include <maya/MGLFunctionTable.h>
#include <maya/MBoundingBox.h>

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

using namespace Alembic::AbcGeom;

class AlembicGeometry{
protected:
	AlembicGeometry();
	static MGLFunctionTable* g_GLFT;
public:
   virtual ~AlembicGeometry();

   // simple polygons, without normals
   virtual void DrawPolygons() const = 0;

   // wireframe cage
   virtual void DrawWireframe() const = 0;

   // points from vertices
   virtual void DrawPoints() const = 0;

   // polygons and normals for shaded mode
   virtual void DrawNormalAndPolygons() const = 0;

   static void setGLFTable(MGLFunctionTable* table);
};

class AlembicPolymeshGeometry : public AlembicGeometry{
private:
   std::vector<AtVector> m_vlist;
   std::vector<AtUInt> m_vidxs;
   std::vector<AtVector> m_nlist;
   std::vector<AtUInt> m_nidxs;
   std::vector<AtUInt> m_nsides;
public:
   AlembicPolymeshGeometry(IObject* object, AtMatrix transform_matrix, MBoundingBox& bbox);
   ~AlembicPolymeshGeometry();

   void DrawPolygons() const;
   void DrawWireframe() const;
   void DrawPoints() const;
   void DrawNormalAndPolygons() const;
};

class AlembicSubDGeometry : public AlembicGeometry{
private:
   std::vector<AtVector> m_vlist;
   std::vector<AtUInt> m_vidxs;
   std::vector<AtVector> m_nlist;
   std::vector<AtUInt> m_nidxs;
   std::vector<AtUInt> m_nsides;
public:
   AlembicSubDGeometry(IObject* object, AtMatrix transform_matrix, MBoundingBox& bbox);
   ~AlembicSubDGeometry();

   void DrawPolygons() const;
   void DrawWireframe() const;
   void DrawPoints() const;
   void DrawNormalAndPolygons() const;
};

#endif /* ALEMBICGEOMETRY_H_ */
