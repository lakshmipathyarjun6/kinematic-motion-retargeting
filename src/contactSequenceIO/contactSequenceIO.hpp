#ifndef CONTACTSEQUENCEIO_H
#define CONTACTSEQUENCEIO_H

#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "JSONUtils.hpp"

#define OBJECT_NAME MString("object")
#define TABLE_NAME MString("table")

#define CONTACT_POINTS_ATTRIBUTE MString("ContactPoints")
#define CONTACT_OMISSION_INDICES_ATTRIBUTE MString("ContactOmissionIndices")

#define CONTACT_GROUP_PREFIX MString("contacts_")

#define VISUAL_MESH_LINK_SUBSTRING MString(":Mesh")
#define COLLISION_MESH_LINK_SUBSTRING MString("_collision")

using namespace std;

class ContactSequenceIO : public MPxFileTranslator
{
public:
    ContactSequenceIO();

    virtual ~ContactSequenceIO();

    static void *creator();

    virtual bool canBeOpened() const;

    virtual MString defaultExtension() const;

    MStatus exportAll(string &filename);

    MStatus exportAllContacts(string &filename);

    MStatus exportSelection(string &filename);

    virtual bool haveReadMethod() const;

    virtual bool haveWriteMethod() const;

    MPxFileTranslator::MFileKind identifyFile(const MFileObject &fileName,
                                              const char *buffer,
                                              short size) const;

    MStatus import(string &fileName);

    virtual MStatus reader(const MFileObject &file,
                           const MString &optionsString, FileAccessMode mode);

    virtual MStatus writer(const MFileObject &file, const MString &optionString,
                           FileAccessMode mode);

protected:
    MStatus getContactAttribute(MString &contactGroupName,
                                MStringArray &serializedContactPoints);

    MStatus getOmissionIndicesAttribute(MString &contactGroupName,
                                        MIntArray &omissionIndices);

    MStatus loadAllGeometries();

    MStatus pluckExportHandMesh();

    MStatus setContactAttribute(int frame, MString &contactGroupName,
                                MStringArray &serializedContactPoints);

    MStatus setOmissionIndicesAttribute(MString &contactGroupName,
                                        MIntArray &omissionIndices);

    // All discovered candidate mesh names

    set<string> m_candidate_mesh_names;

    // Mesh vars

    MString m_mesh_name;
};

#endif // CONTACTSEQUENCEIO_HPP_
