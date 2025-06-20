#ifndef VIRTUALMARKERIO_H
#define VIRTUALMARKERIO_H

#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MSelectionList.h>

#include "JSONUtils.hpp"

#include <map>

#define MARKER_POINTS_ATTRIBUTE MString("MarkerPoints")

#define VIRTUAL_MARKER_BASE_PREFIX MString("mp")
#define PATCH_SHADER_SUFFIX MString("Shader")

using namespace std;

class VirtualMarkerIO : public MPxFileTranslator
{
public:
    VirtualMarkerIO();

    virtual ~VirtualMarkerIO();

    static void *creator();

    virtual bool canBeOpened() const;

    virtual MString defaultExtension() const;

    MStatus exportAllVirtualMarkers(string &filename);

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
    MStatus createMarker(MString &markerName);

    MStatus getMarkerPartialName(MString &fullMarkerName, MString &partialName);

    MStatus getMarkerPointsAttribute(MString &markerName,
                                     MStringArray &serializedMarkerPoints);

    MStatus getSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MStatus setMarkerPointsAttribute(MString &markerName,
                                     MStringArray &serializedMarkerPoints);

    MStatus setSetSerializedPointsAttribute(MString &setName,
                                            MString attributeName,
                                            MStringArray &serializedPoints);

    MString m_mesh_name;
};

#endif // VIRTUALMARKERIO_HPP_
