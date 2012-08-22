/* (c)2012 BlueBolt Ltd. All rights reserved.
 *
 * AlembicGeometry.cpp
 *
 *  Created on: 21 Aug 2012
 *      Author: ashley-r
 */


#include "AlembicGeometry.h"

MGLFunctionTable* AlembicGeometry::g_GLFT = 0;

AlembicGeometry::AlembicGeometry()
{

}

AlembicGeometry::~AlembicGeometry()
{

}

void AlembicGeometry::setGLFTable(MGLFunctionTable* table)
{
   g_GLFT = table;
}

