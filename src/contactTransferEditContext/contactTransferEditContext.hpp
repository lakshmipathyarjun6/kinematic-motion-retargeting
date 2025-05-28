#ifndef CONTACTTRANSFEREDITCONTEXT_H
#define CONTACTTRANSFEREDITCONTEXT_H

#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPxContext.h>
#include <maya/MSelectionList.h>

#include "geometryProcessingContext.hpp"

#define COMMAND_BUFFER_SIZE 300

#define SOURCE_HAND_NAME MString("hand")

#define AXIS_POINTS_ATTRIBUTE MString("AxisPoints")
#define CONTACT_SPREAD_SCALE_ATTRIBUTE MString("AxisRadialScale")
#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")
#define CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE                               \
    MString("ContactParameterizedDistances")
#define CONTACT_OMISSION_INDICES_ATTRIBUTE MString("ContactOmissionIndices")

#define AXIS_BASE_PREFIX MString("axis_")
#define AXIS_PATCH_BASE_PREFIX MString("axis_patch_")
#define PATCH_SHADER_SUFFIX MString("_shader")

#define CONTACT_PATCH_BASE_PREFIX MString("contact_patch_")
#define CONTACT_GROUP_PREFIX MString("contacts_")

#define AXIS_START_COLOR MColor(0.157, 0.157, 0.157)
#define AXIS_START_PREFIX MString("start_")
#define AXIS_SPHERE_NAME MString("axis_sphere")

#define SOURCE_CONTACT_COLOR MColor(1.0, 0.0, 1.0)
#define SOURCE_CONTACT_SPHERE_NAME MString("source_contact_sphere")

#define TARGET_CONTACT_COLOR MColor(0.0, 1.0, 1.0)
#define TARGET_CONTACT_SPHERE_NAME MString("target_contact_sphere")

#define AXIS_START_SIZE 0.2f     // Size of the start of the axis
#define DEFAULT_SPHERE_SIZE 0.1f // Size of the spheres that form an axis

#define PERSISTANT_STORAGE_OBJECT_NAME MString("defaultObjectSet")
#define PERSISTANT_STORAGE_PAIRED_AXIS_FIRST_NAME MString("paired_axes_first")
#define PERSISTANT_STORAGE_PAIRED_AXIS_SECOND_NAME MString("paired_axes_second")

#define DISTANCE_DUMP_FILENAME "parameterizeddistancedump.txt"

using namespace std;

#define FRAMERATE MTime::k120FPS

class ContactTransferEditContext : public MPxContext
{
public:
    // Basic Setup and Teardown

    ContactTransferEditContext();

    virtual ~ContactTransferEditContext();

    static void *creator();

    virtual void toolOnSetup(MEvent &event);

    virtual void toolOffCleanup();

    // Tool Sheet Reference Properties

    virtual void getClassName(MString &name) const;

    MStatus commitParameterizedDistancesFilter(
        int frameStart, int frameEnd,
        double parameterizedFilterDistanceThreshold);

    MStatus dumpParameterizedDistances(int frameStart, int frameEnd);

    MStatus jumpToFrame(int frame, bool visualize = true);

    MStatus setContactParameterizedFilterDistanceThreshold(
        double parameterizedFilterDistanceThreshold);

    MStatus transferContacts(bool visualize = true,
                             bool performInitialization = true);

    MStatus transferContactsBulk(int frameStart, int frameEnd);

    // Core Context Setup

    MStatus loadAllAxes();

    MStatus loadAllGeometries();

    MStatus loadSavedPairings();

    MStatus pluckTargetMesh();

    // Visualization Utils

    MStatus createVisualizationSphere(MFnMesh &fnMesh,
                                      GeometryProcessingContext *gpc,
                                      MString &serializedPoint, MString name,
                                      float radius, MString shadingNodeName,
                                      MFnTransform &fnTransform);

    MStatus visualizeAllAxes();

    MStatus visualizeAxis(MString &transformName, MString &axisName);

    MStatus visualizeContactGroup(MString &transformName,
                                  MString &contactGroupName,
                                  set<int> &ommissionIndices);

    // Core Utilities

    MStatus getAxisAttribute(MString &axisName,
                             MStringArray &serializedAxisPoints);

    MStatus getAxisPatchName(MString &axisName, MString &axisPatchName);

    MStatus getContactSpreadScaleAttribute(MString &axisName,
                                           double &contactSpreadScale);

    MStatus getAxisShapeName(MString &axisName, MString &shapeName);

    MStatus getContactAttribute(MString &contactName,
                                MStringArray &serializedContactPoints);

    MStatus getParameterizedContactDistanceAttribute(
        MString &contactName, MDoubleArray &parameterizedContactDistances);

    MStatus getSetNumericScaleAttribute(MString &setName, MString attributeName,
                                        double &scaleValue);

    MStatus getSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus interpolateSerializedPoint(MFnMesh &fnMesh,
                                       vector<int> &vertexIndices,
                                       vector<double> &coords,
                                       MFloatPoint &position,
                                       MFloatVector &normal);

    MStatus loadAllFrameContacts();

    MStatus loadAllSetGroupContacts(MFnSet &fnSet, set<string> &storageBuffer);

    MStatus pairAxes(MString &axisName1, MString &axisName2);

    MStatus setAxisAttribute(MString &axisnName,
                             MStringArray &serializedAxisPoints);

    MStatus setContactAttribute(MString &contactGroupName,
                                MStringArray &serializedContactPoints);

    MStatus setOmissionIndicesAttribute(MString &contactGroupName,
                                        MIntArray &omissionIndices);

    MStatus setParameterizedContactDistanceAttribute(
        MString &contactGroupName, MDoubleArray &parameterizedContactDistances);

    MStatus updateAxis(MString &axisName);

    MStatus validateAxisExistence(MString &axisName);

    // Core Context Teardown

    MStatus clearVisualizations();

    MStatus clearAxisVisualizations();

    MStatus clearContactVisualizations();

    MStatus wipeAllRegexNodes(MString &wipeRegex);

private:
    // Mesh and Axis Data

    set<string> m_all_axes;

    MString m_target_mesh_name;
    map<string, MString> m_global_axis_meshes;
    map<string, set<string>> m_global_axis_map;
    map<string, MDagPath> m_global_geometry_map;
    map<string, GeometryProcessingContext *>
        m_global_geometry_processing_context_map;

    // Visualization vars

    map<string, MObject> m_axis_visualization_map;
    map<string, MObject> m_contact_visualization_map;
    map<string, MString> m_contact_sphere_names;

    // Coloring

    int m_color_pallette_index;
    vector<MColor> m_color_pallette;
    map<string, MColor> m_color_table;
    map<string, MColor> m_contact_colors;

    // Pairing

    map<string, MString> m_paired_axes;
    map<string, MString> m_paired_axes_pair_keys;
    map<string, pair<MString, MString>> m_paired_axes_unique_pairs;

    // Transfer buffers

    map<string, double> m_target_radial_scales;

    // Filter vars

    double m_contact_parameterized_distance_threshold;

    // Animation vars

    int m_frame;

    // View vars

    M3dView m_view;
};

#endif // CONTACTTRANSFEREDITCONTEXT_H
