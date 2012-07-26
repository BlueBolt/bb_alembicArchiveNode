//-*****************************************************************************
//
// Copyright (c) 2009-2011,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include "ISubDDrw.h"
#include <Alembic/AbcGeom/Visibility.h>

namespace SimpleAbcViewer {

//-*****************************************************************************
ISubDDrw::ISubDDrw( ISubD &iPmesh , std::vector<std::string> path )
  : IObjectDrw( iPmesh, false, path )
  , m_subD( iPmesh )
{
    // Get out if problems.
    if ( !m_subD.valid() )
    {
        return;
    }

    if ( m_subD.getSchema().getNumSamples() > 0 )
    {
        m_subD.getSchema().get( m_samp );
    }

    m_boundsProp = m_subD.getSchema().getSelfBoundsProperty();

    // The object has already set up the min time and max time of
    // all the children.
    // if we have a non-constant time sampling, we should get times
    // out of it.
    TimeSamplingPtr iTsmp = m_subD.getSchema().getTimeSampling();
    if ( !m_subD.getSchema().isConstant() )
    {
        size_t numSamps =  m_subD.getSchema().getNumSamples();
        if ( numSamps > 0 )
        {
            chrono_t minTime = iTsmp->getSampleTime( 0 );
            m_minTime = std::min( m_minTime, minTime );
            chrono_t maxTime = iTsmp->getSampleTime( numSamps-1 );
            m_maxTime = std::max( m_maxTime, maxTime );
        }
    }
}

//-*****************************************************************************
ISubDDrw::~ISubDDrw()
{
    // Nothing!
}

//-*****************************************************************************
bool ISubDDrw::valid()
{
    return IObjectDrw::valid() && m_subD.valid();
}

//-*****************************************************************************
void ISubDDrw::setTime( chrono_t iSeconds )
{
    IObjectDrw::setTime( iSeconds );
    if ( !valid() )
    {
        m_drwHelper.makeInvalid();
        return;
    }

    // Use nearest for now.
    ISampleSelector ss( iSeconds, ISampleSelector::kNearIndex );
    ISubDSchema::Sample psamp;

    if ( m_subD.getSchema().isConstant() )
    {
        psamp = m_samp;
    }
    else if ( m_subD.getSchema().getNumSamples() > 0 )
    {
        m_subD.getSchema().get( psamp, ss );
    }

    // Get the stuff.
    P3fArraySamplePtr P = psamp.getPositions();
    Int32ArraySamplePtr indices = psamp.getFaceIndices();
    Int32ArraySamplePtr counts = psamp.getFaceCounts();

    Box3d bounds;
    bounds.makeEmpty();

    if ( m_boundsProp && m_boundsProp.getNumSamples() > 0 )
    { bounds = m_boundsProp.getValue( ss ); }

    // Update the mesh hoo-ha.
    m_drwHelper.update( P, V3fArraySamplePtr(),
                        indices, counts, bounds );

    if ( !m_drwHelper.valid() )
    {
        m_subD.reset();
        return;
    }

    // The Object update computed child bounds.
    // Extend them by this.
    if ( !m_drwHelper.getBounds().isEmpty() )
    {
        m_bounds.extendBy( m_drwHelper.getBounds() );
    }
}

//-*****************************************************************************
void ISubDDrw::draw( const DrawContext &iCtx )
{
    if ( !valid() )
    {
        return;
    }

    m_drwHelper.draw( iCtx );

    IObjectDrw::draw( iCtx );
}

} // End namespace SimpleAbcViewer
