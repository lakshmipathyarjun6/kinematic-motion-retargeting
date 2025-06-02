#ifndef GRABMotionSEQUENCEIO_H
#define GRABMotionSEQUENCEIO_H

#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "cnpy.h"

#define HAND_NAME MString("hand")
#define OBJECT_NAME MString("object")
#define TABLE_NAME MString("table")

#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")

#define CONTACT_BASE_PREFIX MString("contact_")
#define CONTACT_GROUP_PREFIX MString("contacts_")

#define XYZ_BLOCK_SIZE 3

#define SCALE_MULT 100.0

#define NUM_MOCAP_MARKERS 25

using namespace std;

class GRABMotionSequenceIO : public MPxFileTranslator
{
public:
    GRABMotionSequenceIO();

    virtual ~GRABMotionSequenceIO();

    static void *creator();

    virtual bool canBeOpened() const;

    virtual MString defaultExtension() const;

    MPxFileTranslator::MFileKind identifyFile(const MFileObject &fileName,
                                              const char *buffer,
                                              short size) const;

    virtual bool haveReadMethod() const;

    virtual bool haveWriteMethod() const;

    virtual MStatus reader(const MFileObject &file, const MString &optionString,
                           FileAccessMode mode);

    MStatus readImport(string &fileName);

protected:
    template <typename T>
    void extractElements(T srcArray[], vector<T> &result, int chunkSize);

    MStatus generateFrameContactSet(int frame, MString shapeName,
                                    vector<uint32_t> &contactFrameLocations,
                                    int numContacts);

    MStatus
    saveGenericKeyframe(MString objectName); // saves rotation and translation

    MStatus saveHandVertexKeyframe(int numVertices);

    MStatus setContactAttribute(MObject &contactSet,
                                MStringArray &serializedContactPoints);

    map<string, MDagPath> mVirtualMocapMarkers;
    map<int, string> mVirtualMarkerVertexMappings;

    string mVirtualMocapMarkerNames[NUM_MOCAP_MARKERS];
};

#endif // GRABMotionSEQUENCEIO_HPP_
