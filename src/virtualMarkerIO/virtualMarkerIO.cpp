#include "virtualMarkerIO.hpp"

VirtualMarkerIO::VirtualMarkerIO() {}

VirtualMarkerIO::~VirtualMarkerIO() {}

void *VirtualMarkerIO::creator() { return new VirtualMarkerIO(); }

bool VirtualMarkerIO::canBeOpened() const { return true; }

MString VirtualMarkerIO::defaultExtension() const { return "virtualmarkers"; }

MStatus VirtualMarkerIO::exportAllVirtualMarkers(string &filename)
{
    MStatus status;

    MString meshShapeName = m_mesh_name + "Shape";

    MSelectionList selectionList;

    // Get all associated virtual markers

    MString markerPrefix = VIRTUAL_MARKER_BASE_PREFIX + meshShapeName;

    MString markerRegex = markerPrefix + MString("*");

    MStringArray allMarkerNames;

    status = MGlobal::getSelectionListByName(markerRegex, selectionList);

    MItSelectionList itList(selectionList);
    for (; !itList.isDone(); itList.next())
    {
        MObject markerGroup;
        status = itList.getDependNode(markerGroup);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDependencyNode fnMarkerGroupDepNode(markerGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString markerGroupName = fnMarkerGroupDepNode.name(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = allMarkerNames.append(markerGroupName);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    Document d;
    d.SetObject();

    Document::AllocatorType &allocator = d.GetAllocator();

    Value meshName(kStringType);
    meshName.SetString(meshShapeName.asChar(), meshShapeName.length());
    d.AddMember("meshName", meshName, allocator);

    int numMarkers = allMarkerNames.length();

    Value allMarkerData(kArrayType);

    for (int i = 0; i < numMarkers; i++)
    {
        MString markerName = allMarkerNames[i];

        int exclusionSubstringIndex = markerName.indexW(PATCH_SHADER_SUFFIX);
        bool excludeMarker = exclusionSubstringIndex != -1;

        if (excludeMarker)
        {
            continue;
        }

        MString markerPartialName;
        status = getMarkerPartialName(markerName, markerPartialName);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        Value markerData(kObjectType);

        Value markerNameData(kStringType);
        markerNameData.SetString(markerPartialName.asChar(), allocator);
        markerData.AddMember("name", markerNameData, allocator);

        Value markerPoints(kArrayType);

        MStringArray serializedMarkerPoints;
        status = getMarkerPointsAttribute(markerName, serializedMarkerPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numMarkerPoints = serializedMarkerPoints.length();

        for (int i = 0; i < numMarkerPoints; i++)
        {
            MString serializedMarkerPoint = serializedMarkerPoints[i];

            Value markerPointData(kStringType);
            markerPointData.SetString(serializedMarkerPoint.asChar(),
                                      allocator);

            markerPoints.PushBack(markerPointData, allocator);
        }

        markerData.AddMember("MarkerPoints", markerPoints, allocator);

        allMarkerData.PushBack(markerData, allocator);
    }

    d.AddMember("data", allMarkerData, allocator);

    writeJSON(d, filename);

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::exportSelection(string &filename)
{
    MStatus status;
    MGlobal::displayInfo("Writing file...");

    MSelectionList selectionList;

    status = MGlobal::getActiveSelectionList(selectionList);

    if (status != MS::kSuccess || selectionList.length() == 0)
    {
        MGlobal::displayInfo("Nothing loaded - please select a mesh "
                             "transform in the outliner.");
        return MS::kFailure;
    }

    if (selectionList.length() > 1)
    {
        MGlobal::displayInfo("Only 1 mesh can be loaded at a time.");
        return MS::kFailure;
    }

    MObject meshTransformObj;
    status = selectionList.getDependNode(0, meshTransformObj);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDagNode fnGroupDagNode(meshTransformObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    m_mesh_name = fnGroupDagNode.name(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Loading virtual markers for mesh " + m_mesh_name +
                         "...");

    status = exportAllVirtualMarkers(filename);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

bool VirtualMarkerIO::haveReadMethod() const { return true; }

bool VirtualMarkerIO::haveWriteMethod() const { return true; }

MPxFileTranslator::MFileKind
VirtualMarkerIO::identifyFile(const MFileObject &fileName, const char *buffer,
                              short size) const
{
    MStatus status;
    MString name = fileName.resolvedName();
    unsigned int nameLength = name.numChars();

    name.toLowerCase();
    MStringArray tokens;
    status = name.split('.', tokens);
    CHECK_MSTATUS(status);

    MString lastToken = tokens[tokens.length() - 1];

    if (nameLength > 3 && lastToken == defaultExtension())
    {
        return MPxFileTranslator::kIsMyFileType;
    }

    return MPxFileTranslator::kNotMyFileType;
}

MStatus VirtualMarkerIO::import(string &fileName)
{
    MStatus status;

    MSelectionList selectionList;

    Document d = loadJSON(fileName);

    MString meshName = MString(d["meshName"].GetString());
    MString markerPrefix = VIRTUAL_MARKER_BASE_PREFIX + meshName;

    for (const auto &markerData : d["data"].GetArray())
    {
        MString markerPartialName = MString(markerData["name"].GetString());

        MStringArray serializedMarkerPoints;

        for (const auto &point : markerData["MarkerPoints"].GetArray())
        {
            MString serializedPoint = MString(point.GetString());

            status = serializedMarkerPoints.append(serializedPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        MString markerFullName = markerPrefix + markerPartialName;

        status = createMarker(markerFullName);

        status =
            setMarkerPointsAttribute(markerFullName, serializedMarkerPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::reader(const MFileObject &file,
                                const MString &optionString,
                                FileAccessMode mode)
{
    MStatus status;

    MString resolvedPath = file.resolvedPath();
    MString resolvedName = file.resolvedName();

    MString resolvedFullName = resolvedPath + resolvedName;

    string fileNameChar = resolvedFullName.asChar();

    status = import(fileNameChar);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::writer(const MFileObject &file,
                                const MString &optionString,
                                FileAccessMode mode)
{
    MStatus status;

    MString resolvedPath = file.resolvedPath();
    MString resolvedName = file.resolvedName();

    MString resolvedFullName = resolvedPath + resolvedName;

    string fileNameChar = resolvedFullName.asChar();

    if (mode == MPxFileTranslator::kExportActiveAccessMode)
    {
        status = exportSelection(fileNameChar);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

// Begin protected utils

MStatus VirtualMarkerIO::createMarker(MString &markerName)
{
    MStatus status;

    MSelectionList selectionList;

    // Create the marker set
    MFnSet fnMarkerSetGenerator;
    MObject newMarkerSet = fnMarkerSetGenerator.create(
        selectionList, MFnSet::Restriction::kNone, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnNewMarkerSet(newMarkerSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnNewMarkerSet.setName(markerName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::getMarkerPartialName(MString &fullMarkerName,
                                              MString &partialName)
{
    MStatus status;

    string fullMarkerNameChar = fullMarkerName.asChar();

    MString basePrefix = VIRTUAL_MARKER_BASE_PREFIX + m_mesh_name + "Shape";

    string basePrefixChar = basePrefix.asChar();
    int basePrefixPos = fullMarkerNameChar.find(basePrefixChar);

    fullMarkerNameChar.erase(basePrefixPos, basePrefixChar.length());

    partialName = MString(fullMarkerNameChar.c_str());

    return MS::kSuccess;
}

MStatus
VirtualMarkerIO::getMarkerPointsAttribute(MString &markerName,
                                          MStringArray &serializedMarkerPoints)
{
    MStatus status;

    status = getSetSerializedPointsAttribute(
        markerName, MARKER_POINTS_ATTRIBUTE, serializedMarkerPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::getSetSerializedPointsAttribute(
    MString &setName, MString attributeName, MStringArray &serializedPoints)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(setName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject setObject;
    status = selectionList.getDependNode(0, setObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!hasAttribute)
    {
        MGlobal::displayInfo("WARNING: Points attribute " + attributeName +
                             " not found");
        return MS::kFailure;
    }

    MObject attrObj = fnDepNode.attribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(setObject, attrObj);

    MObject attributeData = attributePlug.asMObject();

    MFnStringArrayData fnStringArrayData(attributeData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    serializedPoints.clear();

    for (int i = 0; i < fnStringArrayData.length(); i++)
    {
        MString item = fnStringArrayData[i];
        serializedPoints.append(item);
    }

    return MS::kSuccess;
}

MStatus
VirtualMarkerIO::setMarkerPointsAttribute(MString &markerName,
                                          MStringArray &serializedMarkerPoints)
{
    MStatus status;

    status = setSetSerializedPointsAttribute(
        markerName, MARKER_POINTS_ATTRIBUTE, serializedMarkerPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus VirtualMarkerIO::setSetSerializedPointsAttribute(
    MString &setName, MString attributeName, MStringArray &serializedPoints)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(setName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject setObject;
    status = selectionList.getDependNode(0, setObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(attributeName, attributeName,
                                     MFnData::kStringArray, MObject::kNullObj,
                                     &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnDepNode.attribute(attributeName, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(setObject, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
