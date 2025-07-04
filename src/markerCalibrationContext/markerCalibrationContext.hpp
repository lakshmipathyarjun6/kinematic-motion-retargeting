#ifndef MARKERCALIBRATIONCONTEXT_H
#define MARKERCALIBRATIONCONTEXT_H

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MPxContext.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>

#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define COMMAND_BUFFER_SIZE 300

#define SOURCE_HAND_NAME MString("hand")

#define MARKER_POINTS_ATTRIBUTE MString("MarkerPoints")

#define SOURCE_MARKER_BASE_PREFIX MString("mm")
#define TARGET_MARKER_BASE_PREFIX MString("mp")
#define TARGET_MARKER_PATCH_BASE_PREFIX MString("tp")
#define TARGET_MARKER_PATCH_SHADER_SUFFIX MString("Shader")
#define TARGET_MARKER_SPHERE_NAME MString("markerSphere")

#define DEFAULT_SPHERE_SIZE 0.1f // Size of the spheres that form a patch

#define MARKER_PAIRING_LINES_PREFIX MString("mplines")

using namespace std;

class MarkerCalibrationContext : public MPxContext
{
public:
    // Basic Setup and Teardown

    MarkerCalibrationContext();

    virtual ~MarkerCalibrationContext();

    static void *creator();

    virtual void toolOnSetup(MEvent &event);

    virtual void toolOffCleanup();

    // Core Context Setup

    MStatus loadAllGeometries();

    MStatus loadTargetVirtualMarkers();

    MStatus pairAllMarkers();

    MStatus pluckTargetMesh();

    // Visualization Utils

    MStatus createVisualizationSphere(MFnMesh &fnMesh, MString &serializedPoint,
                                      MString name, float radius,
                                      MString shadingNodeName,
                                      MFnTransform &fnTransform,
                                      MSelectionList &selectionList);

    MStatus visualizeAllTargetMarkers();

    MStatus visualizeSourceMarkers();

    MStatus visualizeTargetMarker(MString &targetMarkerName);

    // Core Utils

    MStatus digestNewTargetMarker(MString &targetMarkerName);

    MStatus getSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus
    getTargetMarkerPointsAttribute(MString &targetMarkerName,
                                   MStringArray &serializedMarkerPoints);

    MStatus getTargetMarkerPartialName(MString &fullTargetMarkerName,
                                       MString &partialName);

    MStatus getTargetMarkerPatchName(MString &targetMarkerName,
                                     MString &targetMarkerPatchName);

    MStatus interpolateSerializedPoint(MFnMesh &fnMesh,
                                       vector<int> &vertexIndices,
                                       vector<double> &coords,
                                       MFloatPoint &position,
                                       MFloatVector &normal);

    MStatus parseSerializedPoint(MFnMesh &fnMesh, MString &serializedPoint,
                                 vector<int> &vertices, vector<double> &coords);

    MStatus redrawPairingLines();

    MStatus setSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus setTargetMarkerPointsAttribute(MString &targetMarkerName,
                                           MStringArray &serializedPoints);

    MStatus validateTargetMarkerNamingConvention(MString &targetMarkerName);

    MStatus wipePairingLines();

    // Core Context Teardown

    MStatus clearTargetMarkerVisualizations();

private:
    // Mesh Data

    MString m_target_mesh_name;
    map<string, MDagPath> m_global_geometry_map;

    // Marker data

    map<int, MString> m_source_marker_vertex_mappings;
    map<string, MDagPath> m_source_markers;

    set<string> m_source_marker_names;
    set<string> m_target_marker_names;

    // Marker coloring

    int m_color_pallette_index;
    vector<MColor> m_target_marker_color_pallette;
    map<string, MColor> m_target_marker_color_table;

    // Pairing vars

    map<string, MString> m_source_marker_partial_lookup_table;
    map<string, MString> m_paired_markers;

    // View var

    M3dView m_view;
};

#endif // MARKERCALIBRATIONCONTEXT_H
