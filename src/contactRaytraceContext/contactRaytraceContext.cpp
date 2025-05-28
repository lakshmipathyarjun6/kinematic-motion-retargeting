#include "contactRaytraceContext.hpp"

// Setup and Teardown

ContactRaytraceContext::ContactRaytraceContext() {}

ContactRaytraceContext::~ContactRaytraceContext() {}

void *ContactRaytraceContext::creator() { return new ContactRaytraceContext(); }

void ContactRaytraceContext::toolOnSetup(MEvent &event)
{
    MStatus status;

    MGlobal::displayInfo("Setting up contact raytrace context...");

    m_view = M3dView::active3dView();

    MAnimControl animCtrl;
    MTime time = animCtrl.currentTime();
    m_frame = (int)time.value();
    m_framerate = time.unit();

    status = loadScene();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

void ContactRaytraceContext::toolOffCleanup()
{
    MStatus status;

    MGlobal::displayInfo("Tearing down contact raytrace context...");

    status = wipeTraceLines();
    CHECK_MSTATUS(status);

    status = clearVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

// Tool Sheet Reference Properties

void ContactRaytraceContext::getClassName(MString &name) const
{
    name.set("contactRaytraceContext");
}

MStatus ContactRaytraceContext::dumpContactDistances(int frameStart,
                                                     int frameEnd)
{
    MStatus status;

    ofstream dumpfile(DISTANCE_DUMP_FILENAME);

    if (dumpfile.is_open())
    {
        vector<MPointArray> framePairedContactPoints;

        for (int i = frameStart; i <= frameEnd; i++)
        {
            MGlobal::displayInfo("Dumping contacts for frame " +
                                 MString(to_string(i).c_str()));

            status = jumpToFrame(i, true);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = getPairedFrameContactPoints(framePairedContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int numPairs = framePairedContactPoints.size();

            for (int j = 0; j < numPairs; j++)
            {
                MPointArray pointPair = framePairedContactPoints[j];

                double distance = pointPair[0].distanceTo(pointPair[1]);

                dumpfile << distance << "\n";
            }

            framePairedContactPoints.clear();
        }

        dumpfile.close();
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::jumpToFrame(int frame,
                                            bool suppressVisualization)
{
    MStatus status;

    status = wipeTraceLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = clearVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    m_frame = frame;

    MAnimControl animCtrl;
    MTime newFrame((double)m_frame, m_framerate);

    status = animCtrl.setCurrentTime(newFrame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!suppressVisualization)
    {
        status = loadExistingContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(true, true);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::purgeContactsAboveCutoff(int frameStart,
                                                         int frameEnd,
                                                         double cutoffDistance)
{
    MStatus status;

    MSelectionList selectionList;

    MGlobal::displayInfo("Purging contacts beyond EUCLIDIAN distance " +
                         MString(to_string(cutoffDistance).c_str()));

    ofstream dumpfile(FRAME_PRUNE_FILENAME);

    if (dumpfile.is_open())
    {
        MFnMesh fnHandMesh(m_hand_geometry, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnMesh fnObjectMesh(m_object_geometry, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        set<int> purgeIndices;

        MStringArray serializedObjectContactPoints;
        MStringArray serializedHandContactPoints;

        MFloatPoint handPointPosition;
        MFloatVector handPointNormal;

        MFloatPoint objectPointPosition;
        MFloatVector objectPointNormal;

        vector<int> vertexIndices;
        vector<double> coords;

        for (int frameIndex = frameStart; frameIndex <= frameEnd; frameIndex++)
        {
            selectionList.clear();
            purgeIndices.clear();

            status = jumpToFrame(frameIndex, true);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int numPrunes = 0;

            MString frameStringSuffix = MString(to_string(m_frame).c_str());

            MString contactFrameGroupSetName =
                CONTACT_GROUP_PREFIX + frameStringSuffix;

            status = MGlobal::getSelectionListByName(contactFrameGroupSetName,
                                                     selectionList);

            // No contacts in frame - do nothing
            if (status != MS::kSuccess)
            {
                dumpfile << numPrunes << "\n";
                continue;
            }

            MString objectContactGroupName = CONTACT_GROUP_PREFIX +
                                             OBJECT_NAME + MString("Shape_") +
                                             frameStringSuffix;

            MString handContactGroupName =
                CONTACT_GROUP_PREFIX + SOURCE_HAND_NAME + MString("Shape_") +
                frameStringSuffix;

            status = getContactAttribute(objectContactGroupName,
                                         serializedObjectContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = getContactAttribute(handContactGroupName,
                                         serializedHandContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int numContactPoints = serializedObjectContactPoints.length();

            for (int contactPointIndex = 0;
                 contactPointIndex < numContactPoints; contactPointIndex++)
            {
                MString serializedObjectContactPoint =
                    serializedObjectContactPoints[contactPointIndex];
                MString serializedHandContactPoint =
                    serializedHandContactPoints[contactPointIndex];

                vertexIndices.clear();
                coords.clear();

                status = parseSerializedPoint(fnObjectMesh,
                                              serializedObjectContactPoint,
                                              vertexIndices, coords);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = interpolateSerializedPoint(fnObjectMesh, vertexIndices,
                                                    coords, objectPointPosition,
                                                    objectPointNormal);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                vertexIndices.clear();
                coords.clear();

                status =
                    parseSerializedPoint(fnHandMesh, serializedHandContactPoint,
                                         vertexIndices, coords);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = interpolateSerializedPoint(fnHandMesh, vertexIndices,
                                                    coords, handPointPosition,
                                                    handPointNormal);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                double distance =
                    objectPointPosition.distanceTo(handPointPosition);

                if (distance > cutoffDistance)
                {
                    purgeIndices.insert(contactPointIndex);
                    numPrunes++;
                }
            }

            status =
                purgeContactPairs(serializedHandContactPoints, purgeIndices);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            dumpfile << numPrunes << "\n";
        }

        // Clear fully empty groups
        for (int frameIndex = frameStart; frameIndex <= frameEnd; frameIndex++)
        {
            selectionList.clear();

            status = jumpToFrame(frameIndex, true);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MString frameStringSuffix = MString(to_string(m_frame).c_str());

            MString contactFrameGroupSetName =
                CONTACT_GROUP_PREFIX + frameStringSuffix;

            status = MGlobal::getSelectionListByName(contactFrameGroupSetName,
                                                     selectionList);

            // No contacts in frame - do nothing
            if (status != MS::kSuccess)
            {
                continue;
            }

            MString handContactGroupName =
                CONTACT_GROUP_PREFIX + SOURCE_HAND_NAME + MString("Shape_") +
                frameStringSuffix;

            status = getContactAttribute(handContactGroupName,
                                         serializedHandContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            // No contacts found - delete group
            if (serializedHandContactPoints.length() <= 0)
            {
                cout << "Clearing group " << contactFrameGroupSetName << endl;

                MObject contactSetGroup;
                status = selectionList.getDependNode(0, contactSetGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = MGlobal::deleteNode(contactSetGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }

        dumpfile.close();
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::raytraceContacts(bool suppressVisualization)
{
    MStatus status;

    MGlobal::displayInfo("Raytracing all contacts in frame " +
                         MString(to_string(m_frame).c_str()));

    MSelectionList selectionList;

    MString objectShapeName = OBJECT_NAME + "Shape_";

    MString objectContactGroup = CONTACT_GROUP_PREFIX + objectShapeName +
                                 MString(to_string(m_frame).c_str());

    status = MGlobal::getSelectionListByName(objectContactGroup, selectionList);

    if (status == MStatus::kSuccess)
    {
        MStringArray serializedObjectContactPoints;
        status = getContactAttribute(objectContactGroup,
                                     serializedObjectContactPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = raytraceContactPoints(serializedObjectContactPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = cleanBadContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (!suppressVisualization)
        {
            status = clearVisualizations();
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = loadExistingContacts();
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }
    else
    {
        MGlobal::displayInfo("No contacts found in frame");
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::raytraceContactsBulk(int frameStart,
                                                     int frameEnd)
{
    MStatus status;

    for (int i = frameStart; i <= frameEnd; i++)
    {
        status = jumpToFrame(i, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = raytraceContacts(true);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

// Core Context Setup

MStatus ContactRaytraceContext::loadExistingContacts()
{
    MStatus status;

    MSelectionList selectionList;

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = SOURCE_HAND_NAME + "Shape_";

    MString objectContactGroupName =
        CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;

    status =
        MGlobal::getSelectionListByName(objectContactGroupName, selectionList);

    // No object contacts in frame - do nothing
    if (status != MS::kSuccess)
    {
        return MS::kSuccess;
    }

    status = visualizeObjectContact(objectContactGroupName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString handContactGroupName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

    status =
        MGlobal::getSelectionListByName(handContactGroupName, selectionList);

    // No hand contacts in frame - do nothing
    if (status != MS::kSuccess)
    {
        return MS::kSuccess;
    }

    status = visualizeHandContact(handContactGroupName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = redrawTraceLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::loadScene()
{
    MStatus status;

    MString objectShapeName = OBJECT_NAME + "Shape";
    MString handShapeName = SOURCE_HAND_NAME + "Shape";

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(handShapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.getDagPath(0, m_hand_geometry);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(objectShapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.getDagPath(0, m_object_geometry);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

// Visualization Utils

MStatus ContactRaytraceContext::createVisualizationSphere(
    MFnMesh &fnMesh, MString &serializedPoint, MString name, float radius,
    MString shadingNodeName, MFnTransform &fnTransform)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    vector<int> vertexIndices;
    vector<double> coords;

    status =
        parseSerializedPoint(fnMesh, serializedPoint, vertexIndices, coords);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFloatPoint position;
    MFloatVector normal;

    status = interpolateSerializedPoint(fnMesh, vertexIndices, coords, position,
                                        normal);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Create a sphere
    MStringArray sphereNames;
    snprintf(command, COMMAND_BUFFER_SIZE, "polySphere -name %s -radius %f",
             name.asChar(), radius);
    status = MGlobal::executeCommand(command, sphereNames);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set the sphere location to the contact point position
    MString objectName = sphereNames[0];
    status = MGlobal::getSelectionListByName(objectName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject sphere;
    status = selectionList.getDependNode(0, sphere);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDagNode sphereDagNode(sphere, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath sphereDag;
    status = sphereDagNode.getPath(sphereDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnSphereTransform(sphereDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MVector trans(position[0], position[1], position[2]);
    fnSphereTransform.setTranslation(trans, MSpace::kWorld);

    // Add sphere to global list of geometries
    status = sphereDag.push(sphereDag.child(0));
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set the sphere color
    snprintf(command, COMMAND_BUFFER_SIZE, "hyperShade -assign %s",
             shadingNodeName.asChar());
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Add to transform group
    status = fnTransform.addChild(sphere);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::visualizeContact(MFnMesh &fnMesh,
                                                 MString meshName,
                                                 MString &contactName,
                                                 MColor contactColor)
{
    MStatus status;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject pointGroup = fnTransform.create(MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString contactPatchName = CONTACT_PATCH_BASE_PREFIX + meshName + "_" +
                               MString(to_string(m_frame).c_str());

    fnTransform.setName(contactPatchName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString shadingNodeName = contactPatchName + CONTACT_SHADER_SUFFIX;

    memset(command, 0, sizeof(command));
    snprintf(command, COMMAND_BUFFER_SIZE,
             "shadingNode -asShader lambert -name %s",
             shadingNodeName.asChar());
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    snprintf(command, COMMAND_BUFFER_SIZE,
             "setAttr %s.color -type double3 %f %f %f",
             shadingNodeName.asChar(), contactColor[0], contactColor[1],
             contactColor[2]);
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedContactPoints;
    status = getContactAttribute(contactName, serializedContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < serializedContactPoints.length(); i++)
    {
        MString serializedContactPoint = serializedContactPoints[i];

        status = createVisualizationSphere(
            fnMesh, serializedContactPoint, CONTACT_POINT_SPHERE_NAME,
            DEFAULT_SPHERE_SIZE, shadingNodeName, fnTransform);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::visualizeHandContact(MString &handContactName)
{
    MStatus status;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = visualizeContact(fnHandMesh, SOURCE_HAND_NAME, handContactName,
                              HAND_CONTACT_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus
ContactRaytraceContext::visualizeObjectContact(MString &objectContactName)
{
    MStatus status;

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = visualizeContact(fnObjectMesh, OBJECT_NAME, objectContactName,
                              OBJECT_CONTACT_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

// Core Utils

MStatus ContactRaytraceContext::cleanBadContacts()
{
    MStatus status;

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MStringArray serializedObjectContactPoints;
    MStringArray serializedHandContactPoints;

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = SOURCE_HAND_NAME + "Shape_";

    MString objectContactGroupName =
        CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;
    MString handContactGroupName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

    status = getContactAttribute(objectContactGroupName,
                                 serializedObjectContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        getContactAttribute(handContactGroupName, serializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (serializedObjectContactPoints.length() !=
        serializedHandContactPoints.length())
    {
        MGlobal::displayInfo(
            "ERROR: Unequal number of contact points found in frame " +
            frameStringSuffix + " - unable to clean.");
        return MS::kFailure;
    }

    int numOriginalPoints = serializedObjectContactPoints.length();

    MStringArray cleanedSerializedObjectContactPoints;
    MStringArray cleanedSerializedHandContactPoints;

    for (int i = 0; i < numOriginalPoints; i++)
    {
        MString serializedObjectContactPoint = serializedObjectContactPoints[i];
        MString serializedHandContactPoint = serializedHandContactPoints[i];

        if (serializedObjectContactPoint != DEFAULT_MISS_FILLER &&
            serializedHandContactPoint != DEFAULT_MISS_FILLER)
        {
            status = cleanedSerializedObjectContactPoints.append(
                serializedObjectContactPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = cleanedSerializedHandContactPoints.append(
                serializedHandContactPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    if (cleanedSerializedObjectContactPoints.length() !=
        cleanedSerializedHandContactPoints.length())
    {
        MGlobal::displayInfo("ERROR: Unequal number of contact points found "
                             "for cleaned in frame " +
                             frameStringSuffix);
        return MS::kFailure;
    }

    int numCleanedPoints = cleanedSerializedObjectContactPoints.length();

    int totalPointsCleaned = numOriginalPoints - numCleanedPoints;

    MGlobal::displayInfo("Cleaned " +
                         MString(to_string(totalPointsCleaned).c_str()) +
                         " points in frame " + frameStringSuffix);

    status = setContactAttribute(objectContactGroupName,
                                 cleanedSerializedObjectContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = setContactAttribute(handContactGroupName,
                                 cleanedSerializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::getContactAttribute(
    MString &contactName, MStringArray &serializedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(contactName, selectionList);
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

    status = serializedContactPoints.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnStringArrayData.length(); i++)
    {
        MString item = fnStringArrayData[i];

        status = serializedContactPoints.append(item);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::getPairedFrameContactPoints(
    vector<MPointArray> &pairedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MString contactFrameGroupSetName = CONTACT_GROUP_PREFIX + frameStringSuffix;

    status = MGlobal::getSelectionListByName(contactFrameGroupSetName,
                                             selectionList);

    // No contacts in frame - do nothing
    if (status != MS::kSuccess)
    {
        return MS::kSuccess;
    }

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = SOURCE_HAND_NAME + "Shape_";

    MString objectContactGroupFullName =
        CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;
    MString handContactGroupFullName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedObjectContactPoints;
    MStringArray serializedHandContactPoints;

    status = getContactAttribute(objectContactGroupFullName,
                                 serializedObjectContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = getContactAttribute(handContactGroupFullName,
                                 serializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFloatPoint handPointPosition;
    MFloatVector handPointNormal;

    MFloatPoint objectPointPosition;
    MFloatVector objectPointNormal;

    vector<int> vertexIndices;
    vector<double> coords;

    int numContactPoints = serializedObjectContactPoints.length();

    for (int j = 0; j < numContactPoints; j++)
    {
        MString serializedObjectContactPoint = serializedObjectContactPoints[j];
        MString serializedHandContactPoint = serializedHandContactPoints[j];

        MPointArray pointPair;

        vertexIndices.clear();
        coords.clear();

        status = parseSerializedPoint(
            fnObjectMesh, serializedObjectContactPoint, vertexIndices, coords);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status =
            interpolateSerializedPoint(fnObjectMesh, vertexIndices, coords,
                                       objectPointPosition, objectPointNormal);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        vertexIndices.clear();
        coords.clear();

        status = parseSerializedPoint(fnHandMesh, serializedHandContactPoint,
                                      vertexIndices, coords);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = interpolateSerializedPoint(fnHandMesh, vertexIndices, coords,
                                            handPointPosition, handPointNormal);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = pointPair.append(objectPointPosition);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = pointPair.append(handPointPosition);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        pairedContactPoints.push_back(pointPair);
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::interpolateSerializedPoint(
    MFnMesh &fnMesh, vector<int> &vertexIndices, vector<double> &coords,
    MFloatPoint &position, MFloatVector &normal)
{
    MStatus status;

    if (vertexIndices.size() == 3) // Face
    {
        if (coords.size() != 3)
        {
            return MS::kFailure;
        }

        int v1 = vertexIndices[0];
        int v2 = vertexIndices[1];
        int v3 = vertexIndices[2];

        double v1Coords = coords[0];
        double v2Coords = coords[1];
        double v3Coords = coords[2];

        MPoint vPos1;
        status = fnMesh.getPoint(v1, vPos1, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPoint vPos2;
        status = fnMesh.getPoint(v2, vPos2, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPoint vPos3;
        status = fnMesh.getPoint(v3, vPos3, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm1;
        status = fnMesh.getVertexNormal(v1, false, vNorm1, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm2;
        status = fnMesh.getVertexNormal(v2, false, vNorm2, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm3;
        status = fnMesh.getVertexNormal(v3, false, vNorm3, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Barycentric interpolation
        position = (vPos1 * v1Coords) + (vPos2 * v2Coords) + (vPos3 * v3Coords);
        normal =
            (vNorm1 * v1Coords) + (vNorm2 * v2Coords) + (vNorm3 * v3Coords);

        normal.normalize();
    }
    else if (vertexIndices.size() == 2) // Edge
    {
        if (coords.size() != 1)
        {
            return MS::kFailure;
        }

        int v1 = vertexIndices[0];
        int v2 = vertexIndices[1];

        double v2Coords = coords[0];

        MPoint vPos1;
        status = fnMesh.getPoint(v1, vPos1, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPoint vPos2;
        status = fnMesh.getPoint(v2, vPos2, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm1;
        status = fnMesh.getVertexNormal(v1, false, vNorm1, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm2;
        status = fnMesh.getVertexNormal(v2, false, vNorm2, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Linear interpolation
        position = (vPos1 * (1.0 - v2Coords)) + (vPos2 * v2Coords);
        normal = (vNorm1 * (1.0 - v2Coords)) + (vNorm2 * v2Coords);

        normal.normalize();
    }
    else if (vertexIndices.size() == 1)
    {
        int v = vertexIndices[0];

        MPoint vPos;
        status = fnMesh.getPoint(v, vPos, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector vNorm;
        status = fnMesh.getVertexNormal(v, false, vNorm, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // No interpolation needed
        position = vPos;
        normal = vNorm;

        normal.normalize();
    }
    else
    {
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::parseSerializedPoint(MFnMesh &fnMesh,
                                                     MString &serializedPoint,
                                                     vector<int> &vertices,
                                                     vector<double> &coords)
{
    MStatus status;

    string elementType;
    int index;

    double edgeInterpWeight;
    double vertexCoordX, vertexCoordY, vertexCoordZ;

    istringstream iss(serializedPoint.asChar());
    iss >> elementType;

    if (elementType == "v")
    {
        iss >> index;
        vertices.push_back(index);
    }
    else if (elementType == "e")
    {
        iss >> index >> edgeInterpWeight;
        vertices.push_back(index);
        coords.push_back(edgeInterpWeight);
    }
    else if (elementType == "f")
    {
        iss >> index >> vertexCoordX >> vertexCoordY >> vertexCoordZ;

        MIntArray faceVertices;
        status = fnMesh.getPolygonVertices(index, faceVertices);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (faceVertices.length() != 3)
        {
            MGlobal::displayInfo("ERROR: Polygon id " +
                                 MString(to_string(index).c_str()) +
                                 " is not a triangle");
            return MS::kFailure;
        }

        vertices.push_back(faceVertices[0]);
        vertices.push_back(faceVertices[1]);
        vertices.push_back(faceVertices[2]);

        coords.push_back(vertexCoordX);
        coords.push_back(vertexCoordY);
        coords.push_back(vertexCoordZ);
    }
    else
    {
        MGlobal::displayInfo("Unknown serialized surface point type - "
                             "unable to resolve. Skipping");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::purgeContactPairs(
    MStringArray &serializedHandContactPoints, set<int> &purgeIndices)
{
    MStatus status;

    if (purgeIndices.size() == 0)
    {
        MGlobal::displayInfo("Nothing to purge - did nothing");
        return MS::kSuccess;
    }

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MString handShapeName = SOURCE_HAND_NAME + "Shape_";

    MString handContactGroupName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

    int numPoints = serializedHandContactPoints.length();

    for (int i = 0; i < numPoints; i++)
    {
        if (purgeIndices.contains(i))
        {
            serializedHandContactPoints[i] = DEFAULT_MISS_FILLER;
        }
    }

    status =
        setContactAttribute(handContactGroupName, serializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = cleanBadContacts();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::raytraceContactPoints(
    MStringArray &serializedObjectContactPoints)
{
    MStatus status;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFloatPoint objectRaySource;
    MFloatVector objectRayDirection;
    MFloatPointArray hitPoints;
    MIntArray hitFaces, hitTriangles;
    MFloatArray hitBary1s, hitBary2s;

    MString handShapeName = SOURCE_HAND_NAME + "Shape";
    MString partialContactName;

    MString handContactGroupName = CONTACT_GROUP_PREFIX + handShapeName +
                                   MString("_") +
                                   MString(to_string(m_frame).c_str());

    MStringArray serializedHandContactPoints;

    vector<int> vertexIndices;
    vector<double> coords;

    for (int i = 0; i < serializedObjectContactPoints.length(); i++)
    {
        vertexIndices.clear();
        coords.clear();

        MString serializedObjectContactPoint = serializedObjectContactPoints[i];

        status = parseSerializedPoint(
            fnObjectMesh, serializedObjectContactPoint, vertexIndices, coords);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status =
            interpolateSerializedPoint(fnObjectMesh, vertexIndices, coords,
                                       objectRaySource, objectRayDirection);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int objectPointIndex = i;
        int handPointIndex = -1;

        if (fnHandMesh.allIntersections(objectRaySource, objectRayDirection,
                                        NULL, NULL, false, MSpace::kWorld,
                                        1000.0, false, NULL, false, hitPoints,
                                        NULL, &hitFaces, &hitTriangles,
                                        &hitBary1s, &hitBary2s, 1e-6, &status))
        {
            MString serializedHitPoint;

            status = selectTrueContactIntersection(
                objectRaySource, objectRayDirection, fnHandMesh,
                serializedHitPoint, hitPoints.length());
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = serializedHandContactPoints.append(serializedHitPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
        else
        {
            status = serializedHandContactPoints.append(DEFAULT_MISS_FILLER);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    if (serializedHandContactPoints.length() !=
        serializedObjectContactPoints.length())
    {
        MGlobal::displayInfo("ERROR: Unequal number of pairings generated");
        return MS::kFailure;
    }

    status =
        setContactAttribute(handContactGroupName, serializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::redrawTraceLines()
{
    MStatus status;

    MSelectionList selectionList;

    status = wipeTraceLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject traceGroup = fnTransform.create();

    MFnTransform fnLineTransform(traceGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnLineTransform.setName(TRACE_LINES_GROUP, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vector<MPointArray> pairedContactPoints;

    status = getPairedFrameContactPoints(pairedContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numPoints = pairedContactPoints.size();

    MFnNurbsCurve fnCurveGenerator;

    for (int i = 0; i < numPoints; i++)
    {
        MPointArray points = pairedContactPoints[i];

        MObject curve = fnCurveGenerator.createWithEditPoints(
            points, 1, MFnNurbsCurve::kOpen, false, true, true, traceGroup,
            &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(false, true);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::selectTrueContactIntersection(
    MFloatPoint &raySource, MFloatVector &rayDirection, MFnMesh &fnHandMesh,
    MString &serializedHitPoint, int numHitPoints)
{
    MStatus status;

    MFloatPoint resolvedHitPoint;
    int hitFace, hitTriangle;
    float hitBary1, hitBary2;

    // If even number of hits, hand has undershot - use current ray
    // Otherwise hand has overshot and is penetrating object - invert ray
    bool isPenetrating = numHitPoints % 2 != 0;

#ifndef OBJECT_SUBSTITUTION
    MFloatVector resolvedRayDir =
        (isPenetrating) ? -1.0 * rayDirection : rayDirection;
#else
    MFloatVector resolvedRayDir = rayDirection;
#endif

    // Take closest intersection with corrected ray direction
    if (fnHandMesh.closestIntersection(
            raySource, resolvedRayDir, NULL, NULL, false, MSpace::kWorld,
            1000.0, false, NULL, resolvedHitPoint, NULL, &hitFace, &hitTriangle,
            &hitBary1, &hitBary2, 1e-6, &status))
    {
        string serializedHitPointChar =
            "f " + to_string(hitFace) + " " + to_string(hitBary1) + " " +
            to_string(hitBary2) + " " + to_string(1.0 - hitBary1 - hitBary2);

        serializedHitPoint = serializedHitPointChar.c_str();
    }

    // If no hits just take closest of original hits
    else if (fnHandMesh.closestIntersection(
                 raySource, rayDirection, NULL, NULL, false, MSpace::kWorld,
                 1000.0, false, NULL, resolvedHitPoint, NULL, &hitFace,
                 &hitTriangle, &hitBary1, &hitBary2, 1e-6, &status))
    {
        string serializedHitPointChar =
            "f " + to_string(hitFace) + " " + to_string(hitBary1) + " " +
            to_string(hitBary2) + " " + to_string(1.0 - hitBary1 - hitBary2);

        serializedHitPoint = serializedHitPointChar.c_str();
    }

    else
    {
        // Should never happen
        MGlobal::displayInfo("ERROR: Unable to find resolved ray intersection");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::setContactAttribute(
    MString &contactGroupName, MStringArray &serializedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);

    // Set does not exist yet - create it
    if (status != MS::kSuccess)
    {
        selectionList.clear();

        MFnSet fnContactGroupGenerator;

        MObject newContactGroup = fnContactGroupGenerator.create(
            selectionList, MFnSet::Restriction::kNone, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnSet fnNewContactGroup(newContactGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        fnNewContactGroup.setName(contactGroupName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        selectionList.clear();

        status = selectionList.add(newContactGroup);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MObject contactSet;
    status = selectionList.getDependNode(0, contactSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnContactSet(contactSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnContactSet.hasAttribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(
            CONTACT_POINTS_ATTRIBUTE, CONTACT_POINTS_ATTRIBUTE,
            MFnData::kStringArray, MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnContactSet.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnContactSet.attribute(CONTACT_POINTS_ATTRIBUTE, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(contactSet, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedContactPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString contactFrameGroupName =
        CONTACT_GROUP_PREFIX + MString(to_string(m_frame).c_str());

    status =
        MGlobal::getSelectionListByName(contactFrameGroupName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject contactFrameGroupSet;
    status = selectionList.getDependNode(0, contactFrameGroupSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnContactFrameGroupSet(contactFrameGroupSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnContactFrameGroupSet.addMember(contactSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContext::wipeTraceLines()
{
    MStatus status;

    MSelectionList selectionList;
    MString linesRegex = TRACE_LINES_GROUP + MString("*");
    status = MGlobal::getSelectionListByName(linesRegex, selectionList);

    if (status == MS::kSuccess)
    {
        MItSelectionList lineItList(selectionList);

        for (; !lineItList.isDone(); lineItList.next())
        {
            MObject lineGroup;
            status = lineItList.getDependNode(lineGroup);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = MGlobal::deleteNode(lineGroup);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

// Core Context Teardown

MStatus ContactRaytraceContext::clearVisualizations()
{
    MStatus status;

    MString contactPatchRegex = CONTACT_PATCH_BASE_PREFIX + MString("*");

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(contactPatchRegex, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("No visualizations found - cleared nothing");
        return MS::kSuccess;
    }

    MItSelectionList itList(selectionList);
    for (; !itList.isDone(); itList.next())
    {
        MObject component;
        status = itList.getDependNode(component);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = MGlobal::deleteNode(component);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}
