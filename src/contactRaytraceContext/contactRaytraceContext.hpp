#ifndef CONTACTRAYTRACECONTEXT_H
#define CONTACTRAYTRACECONTEXT_H

#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MPxContext.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

using namespace std;

#define COMMAND_BUFFER_SIZE 300

#define OBJECT_NAME MString("object")
#define SOURCE_HAND_NAME MString("hand")

#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")

#define CONTACT_PATCH_BASE_PREFIX MString("contact_patch_")
#define CONTACT_GROUP_PREFIX MString("contacts_")
#define CONTACT_SHADER_SUFFIX MString("_shader")

#define CONTACT_POINT_SPHERE_NAME MString("contact_sphere_")

#define DEFAULT_MISS_FILLER MString("X")

#define OBJECT_CONTACT_COLOR MColor(1.0, 0.0, 1.0)
#define HAND_CONTACT_COLOR MColor(0.0, 1.0, 1.0)

#define DEFAULT_SPHERE_SIZE 0.1f // Size of the spheres that form a patch

#define TRACE_LINES_GROUP MString("tlines")

#define DISTANCE_DUMP_FILENAME "contactdistancedump.txt"
#define FRAME_PRUNE_FILENAME "prunesperframedump.txt"

// #define OBJECT_SUBSTITUTION

class ContactRaytraceContext : public MPxContext
{
public:
    // Setup and Teardown

    ContactRaytraceContext();

    virtual ~ContactRaytraceContext();

    static void *creator();

    virtual void toolOnSetup(MEvent &event);

    virtual void toolOffCleanup();

    // Tool Sheet Reference Properties

    virtual void getClassName(MString &name) const;

    MStatus dumpContactDistances(int frameStart, int frameEnd);

    MStatus jumpToFrame(int frame, bool suppressVisualization = false);

    MStatus purgeContactsAboveCutoff(int frameStart, int frameEnd,
                                     double cutoffDistance);

    MStatus raytraceContacts(bool suppressVisualization = false);

    MStatus raytraceContactsBulk(int frameStart, int frameEnd);

    // Core Context Setup

    MStatus loadExistingContacts();

    MStatus loadScene();

    // Visualization Utils

    MStatus createVisualizationSphere(MFnMesh &fnMesh, MString &serializedPoint,
                                      MString name, float radius,
                                      MString shadingNodeName,
                                      MFnTransform &fnTransform);

    MStatus visualizeContact(MFnMesh &fnMesh, MString meshName,
                             MString &contactName, MColor contactColor);

    MStatus visualizeHandContact(MString &handContactName);

    MStatus visualizeObjectContact(MString &objectContactName);

    // Core Utils

    MStatus cleanBadContacts();

    MStatus getContactAttribute(MString &contactName,
                                MStringArray &serializedContactPoints);

    MStatus
    getPairedFrameContactPoints(vector<MPointArray> &pairedContactPoints);

    MStatus interpolateSerializedPoint(MFnMesh &fnMesh,
                                       vector<int> &vertexIndices,
                                       vector<double> &coords,
                                       MFloatPoint &position,
                                       MFloatVector &normal);

    MStatus parseSerializedPoint(MFnMesh &fnMesh, MString &serializedPoint,
                                 vector<int> &vertices, vector<double> &coords);

    MStatus purgeContactPairs(MStringArray &serializedHandContactPoints,
                              set<int> &purgeIndices);

    MStatus raytraceContactPoints(MStringArray &serializedObjectContactPoints);

    MStatus redrawTraceLines();

    MStatus selectTrueContactIntersection(MFloatPoint &raySource,
                                          MFloatVector &rayDirection,
                                          MFnMesh &fnHandMesh,
                                          MString &serializedHitPoint,
                                          int numHitPoints);

    MStatus setContactAttribute(MString &contactGroupName,
                                MStringArray &serializedContactPoints);

    MStatus wipeTraceLines();

    // Core Context Teardown

    MStatus clearVisualizations();

private:
    // Animation vars

    int m_frame;
    MTime::Unit m_framerate;

    // Object mesh var

    MDagPath m_object_geometry;

    // Hand mesh var

    MDagPath m_hand_geometry;

    // View vars

    M3dView m_view;
};

#endif // CONTACTRAYTRACECONTEXT_H
