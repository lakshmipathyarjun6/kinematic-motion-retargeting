#ifndef SMOOTHMOTIONEDITCONTEXT_H
#define SMOOTHMOTIONEDITCONTEXT_H

#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MPxContext.h>
#include <maya/MSelectionList.h>

#include "BSplineCurve.h"
#include "BSplineCurveFit.h"
#include "Vector2.h"

#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#define COMMAND_BUFFER_SIZE 300

#define SPLINE_DEGREE 3
#define SPLINE_DIMENSION 2

#define OBJECT_NAME MString("object")

#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")
#define MARKER_POINTS_ATTRIBUTE MString("MarkerPoints")
#define ACCELERATION_ERROR_STORAGE_ATTRIBUTE                                   \
    MString("AccelerationViolationIndices")
#define SPLINE_RANGE_END MString("ControlSplineRangeEnd")
#define SPLINE_RANGE_START MString("ControlSplineRangeStart")

#define CONTACT_PREFIX MString("contact_")
#define CONTACT_GROUP_PREFIX MString("contacts_")

#define MOCAP_MARKER_BASE_PREFIX MString("mm")
#define VIRTUAL_MARKER_BASE_PREFIX MString("mp")
#define PATCH_VIS_BASE_PREFIX MString("pv")
#define PATCH_SHADER_SUFFIX MString("Shader")
#define PATCH_POINT_SPHERE_NAME MString("sphere_")

#define OBJECT_PATCH_COLOR MColor(1.0, 0.0, 1.0)
#define HAND_PATCH_COLOR MColor(0.0, 1.0, 1.0)
#define VIRTUAL_MARKER_PATCH_COLOR MColor(0.0, 1.0, 0.0)

#define SPLINE_DOF_PREFIX MString("degreeOfFreedom")
#define DEFAULT_HAND_DOF_SPLINE_STORAGE_GROUP MString("dofSplines")

#define CONTACT_PAIRING_LINES_GROUP MString("cplines")
#define MARKER_PAIRING_LINES_GROUP MString("mplines")

#define MESH_EXCLUSION_SUBSTRING MString(":Mesh")

#define ACCELERATION_ERROR_STORAGE_GROUP MString("accerrors")

#define DEFAULT_SPHERE_SIZE 0.1f // Size of the spheres that form a patch

#define SPLINE_SEARCH_EPSILON 0.1
#define SPLINE_SEARCH_BOUND_SEARCH_STEP_SIZE 0.01
#define SPLINE_SEARCH_MAX_ITERATIONS 100

using namespace std;

class SmoothMotionEditContext : public MPxContext
{
public:
    // Setup and Teardown

    SmoothMotionEditContext();

    virtual ~SmoothMotionEditContext();

    static void *creator();

    virtual void toolOnSetup(MEvent &event);

    virtual void toolOffCleanup();

    // Tool Sheet Reference Properties

    virtual void getClassName(MString &name) const;

    MStatus fitSplines(int frameStart, int frameEnd, int numControlPoints);

    MStatus jumpToFrame(int frame, bool suppressVisualization = false);

    MStatus loadControlSplines();

    // Core Context Setup

    MStatus loadAllGeometries();

    MStatus loadAllVirtualMarkers();

    MStatus loadMarkerPatchPairing(MString &markerPatchName);

    MStatus parseKinematicTree();

    MStatus pluckObjectAndHandMesh();

    // Visualization Utils

    MStatus createVisualizationSphere(MFnMesh &fnMesh, MString &serializedPoint,
                                      MString name, float radius,
                                      MString shadingNodeName,
                                      MFnTransform &fnTransform);

    MStatus redrawContactVisualizations();

    MStatus redrawMarkerVisualizations();

    MStatus visualizeAllContacts();

    MStatus visualizeAllVirtualMarkers();

    MStatus visualizePatch(MFnMesh &fnMesh, MString &patchName,
                           MString attributeName, MColor vizColor);

    // Core Utils

    MStatus
    computePairedMarkerPatchLocations(vector<MPointArray> &pointLocations);

    MStatus fitHandDofSplines(int numControlPoints);

    MStatus getMocapMarker(MString &mocapMarkerName, MDagPath &mocapMarkerDag);

    MStatus getPairedFrameContactPoints(
        vector<MPointArray> &pairedContactPointLocations,
        vector<MVectorArray> &pairedContactPointNormals);

    MStatus
    getSerializedViolationsAttribute(MStringArray &serializedFrameViolations);

    MStatus getNumericAttribute(MString objName, MString attributeName,
                                int &value);

    MStatus getSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus interpolateSerializedPoint(MFnMesh &fnMesh,
                                       vector<int> &vertexIndices,
                                       vector<double> &coords,
                                       MFloatPoint &position,
                                       MFloatVector &normal);

    MStatus keyframeRig();

    MStatus loadSingleRigDofFromControlSpline(int rigDofIndex, int frame);

    MStatus loadRigDofSolutionFull();

    MStatus loadRigDofSolutionSingle(int rigDofIndex);

    MStatus loadSceneRigState();

    MStatus localDofSplineSearch(BSplineCurve<double> &dofSpline,
                                 Vector2<double> &solution, int frameDesired,
                                 double tInitialGuess);

    MStatus parseSerializedPoint(MFnMesh &fnMesh, MString &serializedPoint,
                                 vector<int> &vertices, vector<double> &coords);

    MStatus setNumericAttribute(MString objName, MString attributeName,
                                int &value);

    MStatus wipeContactPairingLines();

    MStatus wipeMarkerPairingLines();

    MStatus wipePairingLines(MString prefix);

    // Core Context Teardown

    MStatus clearVisualizations(MString filterString = MString());

private:
    // Global tables, indexed by mesh name

    map<string, MDagPath>
        m_global_geometry_map; // Map of mesh name to its geometry DAG

    // Object mesh var

    MDagPath m_object_geometry;

    // Hand mesh vars

    MString m_hand_name;
    MDagPath m_hand_geometry;

    // Animation vars

    int m_frame;
    MTime::Unit m_framerate;
    int m_start_frame;
    int m_end_frame;
    MStringArray m_joint_names;

    // Kinematic vars

    int m_rig_n_dofs;
    MDagPath m_rig_base;
    MDagPathArray m_rig_joints;
    MDoubleArray m_rig_dof_vector;
    vector<pair<int, int>> m_rig_dof_vec_mappings;

    // Marker pairing vars

    map<string, MString> m_paired_marker_patches;

    // Visualization vars

    map<string, MObject> m_patch_visualization_map;

    // View vars

    M3dView m_view;
};

#endif
