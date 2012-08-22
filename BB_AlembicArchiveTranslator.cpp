/* (c)2012 BlueBolt Ltd. All rights reserved.
 *
 * BB_AlembicArchiveTranslator.cpp
 *
 *  Created on: 20 Jul 2012
 *      Author: ashley-r
 */

#include "maya/MFnDagNode.h"
#include "maya/MBoundingBox.h"
#include "maya/MPlugArray.h"

#include "extension/Extension.h"
#include "translators/shape/ShapeTranslator.h"

#include "bb_alembicArchiveShape.h"

// TODO: detect instance

class BB_AlembicArchiveTranslator : public CShapeTranslator
{

        public :

                virtual AtNode *CreateArnoldNodes()
                {
                  m_isMasterDag =  IsMasterInstance(m_masterDag);

                  if (m_isMasterDag)
                  {
                      return AddArnoldNode( "procedural" );
                  }
                  else
                  {
                      return AddArnoldNode( "ginstance" );
                  }
                }



                void Export( AtNode *node )
                {
                  const char* nodeType = AiNodeEntryGetName(AiNodeGetNodeEntry(node));
                  if (strcmp(nodeType, "ginstance") == 0)
                  {
                     ExportInstance(node, m_masterDag);
                  }
                  else
                  {
                     ExportProcedural(node);
                  }
                }
/*
                void Update(AtNode* node)
                {
                   const char* nodeType = AiNodeEntryGetName(AiNodeGetNodeEntry(node));
                   if (strcmp(nodeType, "ginstance") == 0)
                   {
                      ExportInstance(node, m_masterDag);
                   }
                   else
                   {
                      ExportProcedural(node);
                   }
                }
*/
                virtual AtNode* ExportInstance(AtNode *instance, const MDagPath& masterInstance)
                {
                   AtNode* masterNode = AiNodeLookUpByName(masterInstance.partialPathName().asChar());


                   int instanceNum = m_dagPath.instanceNumber();

                   if ( instanceNum > 0 )
                     {
                       std::cout << "ExportInstance::instanceNum :: " << instanceNum << std::endl;

//                       char nodeName[65535];
                       AiNodeSetStr(instance, "name", m_dagPath.partialPathName().asChar());

                       ExportMatrix(instance, 0);

                       AiNodeSetPtr(instance, "node", masterNode);
                       AiNodeSetBool(instance, "inherit_xform", false);
                       int visibility = AiNodeGetInt(masterNode, "visibility");
                       AiNodeSetInt(instance, "visibility", visibility);

                       AiNodeSetPtr( instance, "shader", arnoldShader(instance) );

                       // Export light linking per instance
                       ExportLightLinking(instance);
                     }
                   return instance;
                }

                virtual void ExportProcedural( AtNode *node )
                {
                        // do basic node export
                        ExportMatrix( node, 0 );

                        AiNodeSetPtr( node, "shader", arnoldShader(node) );

                        AiNodeSetInt( node, "visibility", ComputeVisibility() );

                        MPlug plug = FindMayaObjectPlug( "receiveShadows" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "receive_shadows", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "aiSelfShadows" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "self_shadows", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "aiOpaque" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "opaque", plug.asBool() );
                        }

                        // now set the procedural-specific parameters

                        MFnDagNode fnDagNode( m_dagPath );
                        MBoundingBox bound = fnDagNode.boundingBox();

                        AiNodeSetPnt( node, "min", bound.min().x-m_dispPadding, bound.min().y-m_dispPadding, bound.min().z-m_dispPadding );
                        AiNodeSetPnt( node, "max", bound.max().x+m_dispPadding, bound.max().y, bound.max().z+m_dispPadding );

                        const char *dsoPath = getenv( "ALEMBIC_ARNOLD_PROCEDURAL_PATH" );
                        AiNodeSetStr( node, "dso",  dsoPath ? dsoPath : "bb_AlembicArnoldProcedural.so" );

//                        AiNodeDeclare( node, "className", "constant STRING" );
//                        AiNodeDeclare( node, "classVersion", "constant INT" );
//                        AiNodeDeclare( node, "parameterValues", "constant ARRAY STRING" );

                        // cast should be ok as we're registered to only work on procedural holders
//                        IECoreMaya::ProceduralHolder *pHolder = static_cast<IECoreMaya::ProceduralHolder *>( fnDagNode.userNode() );

                        std::string className;
                        int classVersion;
//                        IECore::ParameterisedProceduralPtr procedural = pHolder->getProcedural( &className, &classVersion );

//                        AiNodeSetStr( node, "className", className.c_str() );
//                        AiNodeSetInt( node, "classVersion", classVersion );

                        // Set the parameters for the procedural

                        //abcFile path
                        MString abcFile = fnDagNode.findPlug("abcFile").asString().expandEnvironmentVariablesAndTilde();

                        //object path
                        MString objectPath = fnDagNode.findPlug("objectPath").asString();

                        float shutterOpen = fnDagNode.findPlug("shutterOpen").asFloat();

                        float shutterClose = fnDagNode.findPlug("shutterClose").asFloat();

                        int subDIterations = fnDagNode.findPlug("ai_subDIterations").asInt();

                        bool exportFaceIds = fnDagNode.findPlug("exportFaceIds").asBool();

                        bool makeInstance = fnDagNode.findPlug("makeInstance").asBool();

                        short i_subDUVSmoothing = fnDagNode.findPlug("ai_subDUVSmoothing").asShort();

                        MString  subDUVSmoothing;

                        switch (i_subDUVSmoothing)
                        {
                          case 0:
                            subDUVSmoothing = "pin_corners";
                            break;
                          case 1:
                            subDUVSmoothing = "pin_borders";
                            break;
                          case 2:
                            subDUVSmoothing = "linear";
                            break;
                          case 3:
                            subDUVSmoothing = "smooth";
                            break;
                          default :
                            subDUVSmoothing = "pin_corners";
                            break;
                        }

                        MTime frame;
                        fnDagNode.findPlug("time").getValue( frame );

                        MTime frameOffset;
                        fnDagNode.findPlug("timeOffset").getValue( frameOffset );

                        float time = frame.as(MTime::kFilm)+frameOffset.as(MTime::kFilm);

                        MString argsString;
                        if (objectPath != ""){
                                argsString += "-objectpath ";
                                argsString += objectPath;
                        }
                        if (shutterOpen != 0.0){
                                argsString += " -shutteropen ";
                                argsString += shutterOpen;
                        }
                        if (shutterClose != 0.0){
                                argsString += " -shutterclose ";
                                argsString += shutterClose;
                        }
                        if (subDIterations != 0){
                                argsString += " -subditerations ";
                                argsString += subDIterations;
                        }
                        if (makeInstance){
                                argsString += " -makeinstance ";
                        }
                        argsString += " -subduvsmoothing ";
                        argsString += subDUVSmoothing;
                        argsString += " -filename ";
                        argsString += abcFile;
                        argsString += " -frame ";
                        argsString += time;

                        if (m_displaced){

                            argsString += " -disp_map ";
                            argsString += AiNodeGetName(m_dispNode);

                        }

                        AiNodeSetStr(node, "data", argsString.asChar());

                        // Export light linking per instance
                        ExportLightLinking(node);

                }

                virtual bool RequiresMotionData()
                {
                        return IsMotionBlurEnabled( MTOA_MBLUR_OBJECT ) && IsLocalMotionBlurEnabled();
                }

                virtual void ExportMotion( AtNode *node, AtUInt step )
                {
                        if( !IsMotionBlurEnabled() )
                        {
                                return;
                        }

                        ExportMatrix( node, step );
                }

                static void nodeInitialiser( CAbTranslator context )
                {
                        CExtensionAttrHelper helper( context.maya, "procedural" );
                        CShapeTranslator::MakeCommonAttributes(helper);
                        MakeArnoldVisibilityFlags( helper );

                        helper.MakeInput( "self_shadows" );
                        helper.MakeInput( "opaque" );

                }

                static void *creator()
                {
                        return new BB_AlembicArchiveTranslator();
                }

        protected :

                void GetDisplacement(MObject& obj,
                                                          float& dispPadding,
                                                          bool& enableAutoBump)
                {
                   MFnDependencyNode dNode(obj);
                   MPlug plug = dNode.findPlug("aiDisplacementPadding");
                   if (!plug.isNull())
                      dispPadding = MAX(dispPadding, plug.asFloat());
                   if (!enableAutoBump)
                   {
                      plug = dNode.findPlug("aiDisplacementAutoBump");
                      if (!plug.isNull())
                         enableAutoBump = enableAutoBump || plug.asBool();
                   }
                }

                /// Returns the arnold shader assigned to the procedural. This duplicates
                /// code in GeometryTranslator.h, but there's not much can be done about that
                /// since the GeometryTranslator isn't part of the MtoA public API.
                AtNode *arnoldShader(AtNode* node)
                {
                  m_displaced = false;

                  float maximumDisplacementPadding = -AI_BIG;
                  bool enableAutoBump = false;

                  unsigned instNumber = m_dagPath.isInstanced() ? m_dagPath.instanceNumber() : 0;
                  MPlug shadingGroupPlug = GetNodeShadingGroup(m_dagPath.node(), instNumber);

                  //find and export any displacment shaders attached
                  // DISPLACEMENT MATERIAL EXPORT
                  MPlugArray        connections;
                  MFnDependencyNode fnDGShadingGroup(shadingGroupPlug.node());
                  MPlug shaderPlug = fnDGShadingGroup.findPlug("displacementShader");
                  shaderPlug.connectedTo(connections, true, false);

                  // are there any connections to displacementShader?
                  if (connections.length() > 0)
                  {
                     m_displaced = true;
                     MObject dispNode = connections[0].node();
                     GetDisplacement(dispNode, maximumDisplacementPadding, enableAutoBump);
                     m_dispPadding = maximumDisplacementPadding;
                     AtNode* dispImage(ExportNode(connections[0]));

                     m_dispNode = dispImage;
                  }

                  // Only export displacement attributes if a displacement is applied
                  if (m_displaced)
                  {
                      std::cout << "arnoldShader::m_displaced :: " << m_displaced << std::endl;
                     // Note that disp_height has no actual influence on the scale of the displacement if it is vector based
                     // it only influences the computation of the displacement bounds
                    // AiNodeSetFlt(node, "disp_padding", maximumDisplacementPadding);
                  }

                  // return the exported surface shader
                  return m_session->ExportNode( shadingGroupPlug );
                }

        protected :
                MFnDagNode m_DagNode;
                bool m_isMasterDag;
                bool m_displaced;
                float m_dispPadding;
                MDagPath m_dagPathRef;
                MDagPath m_masterDag;
                AtNode* m_dispNode;
};

extern "C"
{

DLLEXPORT void initializeExtension( CExtension &extension )
{
        extension.Requires( "bb_alembicArchiveShape" );
        extension.RegisterTranslator(
                "bb_alembicArchiveShape",
                "",
                BB_AlembicArchiveTranslator::creator,
                BB_AlembicArchiveTranslator::nodeInitialiser
        );
}

DLLEXPORT void deinitializeExtension( CExtension &extension )
{
}

} // extern "C"





