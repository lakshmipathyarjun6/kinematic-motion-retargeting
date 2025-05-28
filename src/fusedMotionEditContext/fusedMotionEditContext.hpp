#ifndef FUSEDMOTIONEDITCONTEXT_H
#define FUSEDMOTIONEDITCONTEXT_H

#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnIntArrayData.h>
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

#include <nlopt.hpp>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#define COMMAND_BUFFER_SIZE 300

#define OBJECT_NAME MString("object")
#define TABLE_NAME MString("table")

#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")
#define CONTACT_OMISSION_INDICES_ATTRIBUTE MString("ContactOmissionIndices")
#define MARKER_POINTS_ATTRIBUTE MString("MarkerPoints")
#define ACCELERATION_ERROR_STORAGE_ATTRIBUTE                                   \
    MString("AccelerationViolationIndices")

#define CONTACT_PREFIX MString("contact_")
#define CONTACT_GROUP_PREFIX MString("contacts_")
#define ACCELERATION_ERROR_SPLINE_DOF_PREFIX MString("degreeOfFreedomTE_")

#define MOCAP_MARKER_BASE_PREFIX MString("mm")
#define VIRTUAL_MARKER_BASE_PREFIX MString("mp")
#define PATCH_VIS_BASE_PREFIX MString("pv")
#define PATCH_SHADER_SUFFIX MString("Shader")
#define PATCH_POINT_SPHERE_NAME MString("sphere_")

#define OBJECT_PATCH_COLOR MColor(1.0, 0.0, 1.0)
#define HAND_PATCH_COLOR MColor(0.0, 1.0, 1.0)
#define ACCELERATION_VIOLATION_COLOR MColor(1.0, 0.0, 0.0)
#define VIRTUAL_MARKER_PATCH_COLOR MColor(0.0, 1.0, 0.0)

#define CONTACT_PAIRING_LINES_GROUP MString("cplines")
#define MARKER_PAIRING_LINES_GROUP MString("mplines")
#define ACCELERATION_ERROR_LINES_GROUP MString("telines")
#define ACCELERATION_VIOLATION_CUTOFF_LINE MString("tvcutoff")

#define MESH_EXCLUSION_SUBSTRING MString(":Mesh")

#define ACCELERATION_ERROR_STORAGE_GROUP MString("accerrors")

#define DEFAULT_SPHERE_SIZE 0.1f // Size of the spheres that form a patch

// #define RUN_OPTIMIZATION_TIMER

#ifdef RUN_OPTIMIZATION_TIMER
#include <chrono>
#endif

using namespace std;

namespace fs = filesystem;

class FusedMotionEditContext : public MPxContext
{
public:
    // Setup and Teardown

    FusedMotionEditContext();

    virtual ~FusedMotionEditContext();

    static void *creator();

    virtual void toolOnSetup(MEvent &event);

    virtual void toolOffCleanup();

    // Tool Sheet Reference Properties

    virtual void getClassName(MString &name) const;

    MStatus computeAccelerationErrors(int frameStart, int frameEnd);

    MStatus enableOptimizationProgressVisualization(bool enable);

    MStatus finalizeOmissionIndices(int frameStart, int frameEnd);

    MStatus jumpToFrame(int frame, bool suppressVisualization = false);

    MStatus keyframeRig();

    MStatus keyframeRigBulk(int frameStart, int frameEnd,
                            bool saveKeysOnly = false);

    MStatus resetNonRootJoints(bool suppressVisualization = false);

    MStatus resolveAccelerationErrors(int frameStart, int frameEnd,
                                      int maxIterations);

    MStatus setAccelerationViolationEpsilon(int frameStart, int frameEnd,
                                            double epsilon);

    MStatus
    setCoefficientContactDistanceError(double contactDistanceCoefficient);

    MStatus setCoefficientContactError(double contactCoefficient);

    MStatus setCoefficientContactNormalError(double contactNormalCoefficient);

    MStatus setCoefficientIntersectionError(double intersectionCoefficient);

    MStatus setCoefficientMarkerError(double markerCoefficient);

    MStatus setCoefficientPriorError(double priorCoefficient);

    MStatus setNumOptIterations(int numIterations);

    MStatus storeAccelerationErrors();

    MStatus undoOptimization();

    MStatus wipeRigKeyframe(int frameStart, int frameEnd);

    // UI Event Handlers

    virtual void completeAction();

    // Core Context Setup

    MStatus loadAllGeometries();

    MStatus loadAllVirtualMarkers();

    MStatus loadMarkerPatchPairing(MString &markerPatchName);

    MStatus loadTable();

    MStatus parseKinematicTree();

    MStatus pluckObjectAndHandMesh();

    // Visualization Utils

    MStatus createMeshVisualizationSphere(MFnMesh &fnMesh,
                                          MString &serializedPoint,
                                          MString name, float radius,
                                          MString shadingNodeName,
                                          MFnTransform &fnTransform);

    MStatus createVisualizationSphere(MFloatPoint &position, MString name,
                                      float radius, MString shadingNodeName,
                                      MFnTransform &fnTransform);

    MStatus redrawAccelerationViolationCutoff(int frameStart, int frameEnd);

    MStatus redrawContactVisualizations();

    MStatus redrawMarkerVisualizations();

    MStatus visualizeAllContacts();

    MStatus visualizeAllVirtualMarkers();

    MStatus visualizePatch(MFnMesh &fnMesh, MString &patchName,
                           MString attributeName, MColor vizColor);

    // Core Utils

    MStatus
    computePairedMarkerPatchLocations(vector<MPointArray> &pointLocations);

    MStatus computeTableSDF(MPoint &queryPoint, double &signedDistance);

    MStatus
    generateHandTestPoints(vector<pair<MFloatPoint, MFloatVector>> &handPoints);

    MStatus getMocapMarker(MString &mocapMarkerName, MDagPath &mocapMarkerDag);

    MStatus getOmissionIndicesAttribute(MString &contactGroupName,
                                        MIntArray &omissionIndices);

    MStatus getPairedFrameContactPoints(
        vector<MPointArray> &pairedContactPointLocations,
        vector<MVectorArray> &pairedContactPointNormals);

    MStatus getSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus initializeDofSolution();

    nlopt::opt initializeOptimization();

    MStatus interpolateSerializedPoint(MFnMesh &fnMesh,
                                       vector<int> &vertexIndices,
                                       vector<double> &coords,
                                       MFloatPoint &position,
                                       MFloatVector &normal);

    MStatus loadDofSolutionFull();

    MStatus loadDofSolutionSingle(int index);

    MStatus parseSerializedPoint(MFnMesh &fnMesh, MString &serializedPoint,
                                 vector<int> &vertices, vector<double> &coords);

    MStatus runOptimization(nlopt::opt &optim);

    MStatus setContactAttribute(MString &contactGroupName,
                                MStringArray &serializedContactPoints);

    MStatus
    setSerializedViolationsAttribute(MStringArray &serializedFrameViolations);

    MStatus storeExistingFrameSolutions(int frameStart, int frameEnd);

    MStatus wipeContactPairingLines();

    MStatus wipeJointKeyframe(MString &jointName, int frameStart, int frameEnd);

    MStatus wipeMarkerPairingLines();

    MStatus wipePairingLines(MString prefix);

    // Core Context Teardown

    MStatus clearVisualizations(MString filterString = MString());

    // NLOPT Utils

    double computeDofGradient(const MDoubleArray &existingDofs, double step,
                              double currentObjectiveValue, int dofVectorIndex);

    double computeObjective(const MDoubleArray &existingDofs,
                            bool suppressVisualization = false);

    double computeOptimization(const vector<double> &x, vector<double> &grad);

    static double optimizerWrapper(const vector<double> &x,
                                   vector<double> &grad, void *data);

private:
    // Global tables, indexed by mesh name

    map<string, MDagPath>
        m_global_geometry_map; // Map of mesh name to its geometry DAG

    // Object mesh vars

    MDagPath m_object_geometry;

    // Table mesh vars

    MMatrix m_table_transform_inv;
    MVector m_table_box_dims;

    // Hand mesh vars

    MString m_hand_name;
    MDagPath m_hand_geometry;

    // Animation vars

    int m_frame;
    MTime::Unit m_framerate;
    double m_realtime_delta;
    MStringArray m_joint_names;
    map<MTime::Unit, double> m_fpsRealtimeConversionTable;

    // Kinematic vars

    int m_rig_n_dofs;
    MDagPath m_rig_base;
    MDagPathArray m_rig_joints;
    MDoubleArray m_dof_vector;
    stack<MDoubleArray> m_last_dof_vectors;
    vector<pair<int, int>> m_dof_vec_mappings;

    // Marker pairing vars

    map<string, MString> m_paired_marker_patches;

    // Visualization vars

    map<string, MObject> m_patch_visualization_map;

    // Optimization vars

    bool m_optimization_visualization_enabled;
    int m_num_opt_iterations;
    double m_contact_distance_penalty_coefficient;
    double m_contact_normal_penalty_coefficient;
    double m_marker_penalty_coefficient;
    double m_contact_penalty_coefficient;
    double m_intersection_penalty_coefficient;
    double m_prior_penalty_coefficient;

    // Refinement vars

    double m_acceleration_epsilon;
    map<int, MIntArray> m_acceleration_violations;
    map<int, MDoubleArray> m_existing_frame_solutions;
    default_random_engine m_rng;

    // View vars

    M3dView m_view;
};

#endif
