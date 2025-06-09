#include "contactSequenceIO.hpp"

ContactSequenceIO::ContactSequenceIO() {}

ContactSequenceIO::~ContactSequenceIO() {}

void *ContactSequenceIO::creator() { return new ContactSequenceIO(); }

bool ContactSequenceIO::canBeOpened() const { return true; }

MString ContactSequenceIO::defaultExtension() const { return "json"; }

MStatus ContactSequenceIO::exportAll(string &filename)
{
    MStatus status;
    MGlobal::displayInfo("Writing file...");

    status = loadAllGeometries();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = pluckExportHandMesh();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = exportAllContacts(filename);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactSequenceIO::exportAllContacts(string &filename)
{
    MStatus status;

    MString meshShapeName = m_mesh_name + "Shape";

    MSelectionList selectionList;
    MSelectionList groupMemberList;

    MAnimControl animCtrl;

    Document d;
    d.SetObject();

    Document::AllocatorType &allocator = d.GetAllocator();

    Value meshName(kStringType);
    meshName.SetString(meshShapeName.asChar(), meshShapeName.length());
    d.AddMember("meshName", meshName, allocator);

    int numFrames = (int)animCtrl.maxTime().value();

    Value contactFrames(kArrayType);

    for (int frame = 1; frame <= numFrames; frame++)
    {
        selectionList.clear();
        groupMemberList.clear();

        MString meshContactGroupName = CONTACT_GROUP_PREFIX + meshShapeName +
                                       "_" + MString(to_string(frame).c_str());

        status = MGlobal::getSelectionListByName(meshContactGroupName,
                                                 selectionList);

        if (status != MS::kSuccess)
        {
            continue;
        }

        Value frameData(kObjectType);

        Value frameNumber(kNumberType);
        frameNumber.SetInt(frame);
        frameData.AddMember("frame", frameNumber, allocator);

        Value frameContacts(kArrayType);
        Value frameContactOmissionIndices(kArrayType);

        MStringArray serializedContactPoints;
        status =
            getContactAttribute(meshContactGroupName, serializedContactPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MIntArray omissionIndices;
        status =
            getOmissionIndicesAttribute(meshContactGroupName, omissionIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (int i = 0; i < serializedContactPoints.length(); i++)
        {
            MString serializedContactPoint = serializedContactPoints[i];

            Value contactPointData(kStringType);
            contactPointData.SetString(serializedContactPoint.asChar(),
                                       allocator);

            frameContacts.PushBack(contactPointData, allocator);
        }

        for (int i = 0; i < omissionIndices.length(); i++)
        {
            int omissionIndex = omissionIndices[i];

            Value omissionData(kNumberType);
            omissionData.SetInt(omissionIndex);

            frameContactOmissionIndices.PushBack(omissionData, allocator);
        }

        if (serializedContactPoints.length() > 0)
        {
            frameData.AddMember("contacts", frameContacts, allocator);

            if (omissionIndices.length() > 0)
            {
                frameData.AddMember("omissionIndices",
                                    frameContactOmissionIndices, allocator);
            }

            contactFrames.PushBack(frameData, allocator);
        }
    }

    d.AddMember("data", contactFrames, allocator);

    writeJSON(d, filename);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus ContactSequenceIO::exportSelection(string &filename)
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

    MGlobal::displayInfo("Loading contacts for mesh " + m_mesh_name + "...");

    status = exportAllContacts(filename);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

bool ContactSequenceIO::haveReadMethod() const { return true; }

bool ContactSequenceIO::haveWriteMethod() const { return true; }

MPxFileTranslator::MFileKind
ContactSequenceIO::identifyFile(const MFileObject &fileName, const char *buffer,
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

MStatus ContactSequenceIO::import(string &fileName)
{
    MStatus status;

    MSelectionList selectionList;

    Document d = loadJSON(fileName);

    MString meshName = MString(d["meshName"].GetString());
    MString contactGroupName = CONTACT_GROUP_PREFIX + meshName;

    for (const auto &frameData : d["data"].GetArray())
    {
        int frame = frameData["frame"].GetInt();

        MString contactFrameGroupName =
            contactGroupName + "_" + MString(to_string(frame).c_str());

        status = MGlobal::getSelectionListByName(contactFrameGroupName,
                                                 selectionList);

        if (status != MS::kSuccess)
        {
            status = selectionList.clear();
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnSet fnContactGroupSetGenerator;
            MObject newContactGroupSet = fnContactGroupSetGenerator.create(
                selectionList, MFnSet::Restriction::kNone, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnSet fnNewContactGroupSet(newContactGroupSet, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            fnNewContactGroupSet.setName(contactFrameGroupName, false, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
        else
        {
            MGlobal::displayInfo("Contact group " + contactFrameGroupName +
                                 " already exists - skipping creation");
        }

        MStringArray serializedContactPoints;

        for (const auto &point : frameData["contacts"].GetArray())
        {
            MString serializedPoint = MString(point.GetString());

            status = serializedContactPoints.append(serializedPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        status = setContactAttribute(frame, contactFrameGroupName,
                                     serializedContactPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (frameData.HasMember("omissionIndices"))
        {
            MIntArray omissionIndices;

            for (const auto &omissionIndexData :
                 frameData["omissionIndices"].GetArray())
            {
                int omissionIndex = omissionIndexData.GetInt();

                status = omissionIndices.append(omissionIndex);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }

            status = setOmissionIndicesAttribute(contactFrameGroupName,
                                                 omissionIndices);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

MStatus ContactSequenceIO::reader(const MFileObject &file,
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

MStatus ContactSequenceIO::writer(const MFileObject &file,
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
    if (mode == MPxFileTranslator::kExportAccessMode)
    {
        status = exportAll(fileNameChar);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus
ContactSequenceIO::getContactAttribute(MString &contactGroupName,
                                       MStringArray &serializedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;

    status = serializedContactPoints.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject setObject;
    status = selectionList.getDependNode(0, setObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!hasAttribute)
    {
        MGlobal::displayInfo("WARNING: Points attribute " +
                             CONTACT_POINTS_ATTRIBUTE + " not found");
        return MS::kFailure;
    }

    MObject attrObj = fnDepNode.attribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(setObject, attrObj);

    MObject attributeData = attributePlug.asMObject();

    MFnStringArrayData fnStringArrayData(attributeData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnStringArrayData.length(); i++)
    {
        MString item = fnStringArrayData[i];
        serializedContactPoints.append(item);
    }

    return MS::kSuccess;
}

MStatus
ContactSequenceIO::getOmissionIndicesAttribute(MString &contactGroupName,
                                               MIntArray &omissionIndices)
{
    MStatus status;

    MSelectionList selectionList;

    status = omissionIndices.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject setObject;
    status = selectionList.getDependNode(0, setObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(CONTACT_OMISSION_INDICES_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!hasAttribute)
    {
        MGlobal::displayInfo("No attribute " +
                             CONTACT_OMISSION_INDICES_ATTRIBUTE + " found");
        return MS::kSuccess;
    }

    MObject attrObj =
        fnDepNode.attribute(CONTACT_OMISSION_INDICES_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(setObject, attrObj);

    MObject attributeData = attributePlug.asMObject();

    MFnIntArrayData fnIntArrayData(attributeData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnIntArrayData.length(); i++)
    {
        int item = fnIntArrayData[i];
        omissionIndices.append(item);
    }

    return MS::kSuccess;
}

MStatus ContactSequenceIO::loadAllGeometries()
{
    MStatus status;

    // Search every mesh
    MItDag itList(MItDag::TraversalType::kDepthFirst, MFn::kMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (itList.isDone())
    {
        MGlobal::displayInfo("No geometries found - did nothing");
        return MS::kSuccess;
    }

    for (; !itList.isDone(); itList.next())
    {
        MDagPath meshPath;
        status = itList.getPath(meshPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDagNode meshDn(meshPath, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MObject meshTransform = meshDn.parent(0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDagNode transformDn(meshTransform, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath transformPath;
        status = transformDn.getPath(transformPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString meshName = itList.partialPathName(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString transformName = transformDn.partialPathName(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        string transformNameChar = transformName.asChar();

        int objectSubstringIndex = transformName.indexW(OBJECT_NAME);
        int tableSubstringIndex = transformName.indexW(TABLE_NAME);
        int visualMeshLinkSubstringIndex =
            transformName.indexW(VISUAL_MESH_LINK_SUBSTRING);
        int collisionMeshSubstringIndex =
            transformName.indexW(COLLISION_MESH_LINK_SUBSTRING);

        bool excludeMesh = objectSubstringIndex != -1 ||
                           tableSubstringIndex != -1 ||
                           visualMeshLinkSubstringIndex != -1 ||
                           collisionMeshSubstringIndex != -1;

        if (excludeMesh || m_candidate_mesh_names.contains(transformNameChar))
        {
            MGlobal::displayInfo("Skipping " + meshName + " - transform " +
                                 transformName +
                                 " already exists or is explicitly excluded");
            continue;
        }

        m_candidate_mesh_names.insert(transformNameChar);
    }

    return MS::kSuccess;
}

MStatus ContactSequenceIO::pluckExportHandMesh()
{
    MStatus status;

    if (m_candidate_mesh_names.size() > 1)
    {
        MGlobal::displayInfo("Multiple valid meshes found - only one expected");
        MGlobal::displayInfo("Please select a single mesh to export and "
                             "use Export Selection instead");

        for (const auto &entry : m_candidate_mesh_names)
        {
            MString meshName = entry.c_str();
            MGlobal::displayInfo("Candidate: " + meshName);
        }

        return MS::kFailure;
    }

    m_mesh_name = m_candidate_mesh_names.begin()->c_str();

    MGlobal::displayInfo("Found mesh " + m_mesh_name);

    return MS::kSuccess;
}

MStatus
ContactSequenceIO::setContactAttribute(int frame, MString &contactGroupName,
                                       MStringArray &serializedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);

    // Set does not exist yet - create it
    if (status != MS::kSuccess)
    {
        selectionList.clear();

        MFnSet fnContactSetGenerator;

        MObject newContactGroupSet = fnContactSetGenerator.create(
            selectionList, MFnSet::Restriction::kNone, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnSet fnNewContactGroupSet(newContactGroupSet, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        fnNewContactGroupSet.setName(contactGroupName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        selectionList.clear();

        status = selectionList.add(newContactGroupSet);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MObject contactGroupSet;
    status = selectionList.getDependNode(0, contactGroupSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(contactGroupSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(
            CONTACT_POINTS_ATTRIBUTE, CONTACT_POINTS_ATTRIBUTE,
            MFnData::kStringArray, MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnDepNode.attribute(CONTACT_POINTS_ATTRIBUTE, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(contactGroupSet, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedContactPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString contactFrameGroupName =
        CONTACT_GROUP_PREFIX + MString(to_string(frame).c_str());
    status =
        MGlobal::getSelectionListByName(contactFrameGroupName, selectionList);

    // Set group does not exist - create and add member
    if (status != MS::kSuccess)
    {
        selectionList.clear();

        MFnSet fnFrameContactGroupSetGenerator;

        MObject newFrameContactGroupSet =
            fnFrameContactGroupSetGenerator.create(
                selectionList, MFnSet::Restriction::kNone, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnSet fnNewFrameContactGroupSet(newFrameContactGroupSet, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        fnNewFrameContactGroupSet.setName(contactFrameGroupName, false,
                                          &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = selectionList.add(newFrameContactGroupSet);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MObject frameContactGroupSet;
    status = selectionList.getDependNode(0, frameContactGroupSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnFrameContactGroupSet(frameContactGroupSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnFrameContactGroupSet.addMember(contactGroupSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus
ContactSequenceIO::setOmissionIndicesAttribute(MString &contactGroupName,
                                               MIntArray &omissionIndices)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject contactGroupObject;
    status = selectionList.getDependNode(0, contactGroupObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(contactGroupObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(CONTACT_OMISSION_INDICES_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj =
            fnTypedAttr.create(CONTACT_OMISSION_INDICES_ATTRIBUTE,
                               CONTACT_OMISSION_INDICES_ATTRIBUTE,
                               MFnData::kIntArray, MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj =
            fnDepNode.attribute(CONTACT_OMISSION_INDICES_ATTRIBUTE, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(contactGroupObject, attrObj);

    MFnIntArrayData fnIntArrayData;

    MObject intArrayData = fnIntArrayData.create(omissionIndices, &status);

    status = attributePlug.setMObject(intArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
