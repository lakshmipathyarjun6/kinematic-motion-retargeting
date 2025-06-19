#include "smoothMotionEditContext.hpp"

// Setup and Teardown

SmoothMotionEditContext::SmoothMotionEditContext() {}

SmoothMotionEditContext::~SmoothMotionEditContext() {}

void *SmoothMotionEditContext::creator()
{
    return new SmoothMotionEditContext();
}

void SmoothMotionEditContext::toolOnSetup(MEvent &event)
{
    MStatus status;

    MGlobal::displayInfo("Setting up smooth motion edit context...");

    m_view = M3dView::active3dView();

    m_rig_base = MDagPath();
    m_rig_n_dofs = 0;

    m_rig_joints.clear();
    m_rig_dof_vector.clear();
    m_rig_dof_vec_mappings.clear();
    m_joint_names.clear();

    MAnimControl animCtrl;
    MTime time = animCtrl.currentTime();
    m_frame = (int)time.value();
    m_framerate = time.unit();

    status = loadAllGeometries();
    CHECK_MSTATUS(status);

    status = pluckObjectAndHandMesh();
    CHECK_MSTATUS(status);

    status = loadAllVirtualMarkers();
    CHECK_MSTATUS(status);

    status = parseKinematicTree();
    CHECK_MSTATUS(status);

    status = redrawContactVisualizations();
    CHECK_MSTATUS(status);

    status = redrawMarkerVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

void SmoothMotionEditContext::toolOffCleanup()
{
    MStatus status;

    MGlobal::displayInfo("Tearing down smooth motion edit context...");

    status = wipeContactPairingLines();
    CHECK_MSTATUS(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS(status);

    status = clearVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

// Tool Sheet Reference Properties

void SmoothMotionEditContext::getClassName(MString &name) const
{
    name.set("smoothMotionEditContext");
}

MStatus SmoothMotionEditContext::fitSplines(int frameStart, int frameEnd,
                                            int numControlPoints)
{
    MStatus status;

    m_start_frame = frameStart;
    m_end_frame = frameEnd;

    MGlobal::displayInfo("Fitting splines...");

    // Hide visualizations for better performance

    status = wipeContactPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = clearVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fitHandDofSplines(numControlPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = jumpToFrame(m_start_frame, true);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = redrawMarkerVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::jumpToFrame(int frame,
                                             bool suppressVisualization)
{
    MStatus status;

    m_frame = frame;

    MAnimControl animCtrl;
    MTime newFrame((double)m_frame, m_framerate);

    status = animCtrl.setCurrentTime(newFrame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!suppressVisualization)
    {
        status = redrawContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawMarkerVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(true, true);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::loadControlSplines()
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getActiveSelectionList(selectionList);

    if (status != MS::kSuccess || selectionList.length() == 0)
    {
        MGlobal::displayInfo(
            "Nothing loaded - please select a spline group in the outliner.");
        return MS::kFailure;
    }

    if (selectionList.length() > 1)
    {
        MGlobal::displayInfo("Only 1 spline group can be loaded at a time.");
        return MS::kFailure;
    }

    MObject splineGroup;
    status = selectionList.getDependNode(0, splineGroup);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Loading spline group...");

    MFnDagNode fnGroupDagNode(splineGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString splineGroupName = fnGroupDagNode.name(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        getNumericAttribute(splineGroupName, SPLINE_RANGE_END, m_end_frame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        getNumericAttribute(splineGroupName, SPLINE_RANGE_START, m_start_frame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numChildren = fnGroupDagNode.childCount();

    if (m_rig_n_dofs != numChildren)
    {
        MGlobal::displayInfo(
            "Error - control spline count does not match rig DOF count");
        return MS::kFailure;
    }

    float multiplier = 1.0f / (m_end_frame - 1.0f);

    vector<map<int, double>> allFrameDofValues;

    for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
    {
        MObject splineDofCurve = fnGroupDagNode.child(rigDofIndex, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDagNode splineDofCurveDn(splineDofCurve, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath splineDofCurveDag;
        status = splineDofCurveDn.getPath(splineDofCurveDag);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnNurbsCurve fnCurve(splineDofCurveDag, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPointArray controlPoints;

        status = fnCurve.getCVs(controlPoints, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numControlPoints = controlPoints.length();

        vector<double> controlData;

        for (int cpIndex = 0; cpIndex < numControlPoints; cpIndex++)
        {
            // point[x] = t, point[y] = dof value
            MPoint controlPoint = controlPoints[cpIndex];

            controlData.push_back(controlPoint.x);
            controlData.push_back(controlPoint.y);
        }

        BSplineCurve<double> dofSpline(SPLINE_DIMENSION, SPLINE_DEGREE,
                                       numControlPoints);

        dofSpline.SetControlData(controlData);

        map<int, double> frameDofValues;

        for (int frame = m_start_frame; frame <= m_end_frame; frame++)
        {
            int relIndex = frame - m_start_frame;
            float t = multiplier * relIndex;

            Vector2<double> samplePoint;

            localDofSplineSearch(dofSpline, samplePoint, frame, t);

            frameDofValues[frame] = samplePoint[1];
        }

        allFrameDofValues.push_back(frameDofValues);
    }

    for (int frame = m_start_frame; frame <= m_end_frame; frame++)
    {
        status = jumpToFrame(frame, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
        {
            map<int, double> frameDofValues = allFrameDofValues[rigDofIndex];

            double value = frameDofValues[frame];

            // convert back to radians
            if (rigDofIndex < 3 || rigDofIndex > 5)
            {
                value *= M_PI / 180.0;
            }

            m_rig_dof_vector[rigDofIndex] = value;
        }

        status = loadRigDofSolutionFull();
        CHECK_MSTATUS(status);

        status = keyframeRig();
        CHECK_MSTATUS(status);
    }

    status = jumpToFrame(m_start_frame, true);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = redrawMarkerVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

// Core Context Setup

MStatus SmoothMotionEditContext::loadAllGeometries()
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

        string meshNameChar = meshName.asChar();
        string transformNameChar = transformName.asChar();

        int exclusionSubstringIndex =
            transformName.indexW(MESH_EXCLUSION_SUBSTRING);
        bool excludeMesh = exclusionSubstringIndex != -1;

        if (!m_global_geometry_map.contains(transformNameChar) && !excludeMesh)
        {
            m_global_geometry_map[transformNameChar] = meshPath;

            MGlobal::displayInfo("Digested " + meshName + " with transform " +
                                 transformName);
        }
        else
        {
            MGlobal::displayInfo("Skipping " + meshName + " - transform " +
                                 transformName + " already exists");
            continue;
        }
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::loadAllVirtualMarkers()
{
    MStatus status;

    MSelectionList selectionList;

    MString markerPatchRegex =
        VIRTUAL_MARKER_BASE_PREFIX + m_hand_name + "Shape" + MString("*");

    status = MGlobal::getSelectionListByName(markerPatchRegex, selectionList);

    if (status == MStatus::kSuccess)
    {
        MItSelectionList markerPatchItList(selectionList);

        for (; !markerPatchItList.isDone(); markerPatchItList.next())
        {
            MObject markerPatchSet;
            status = markerPatchItList.getDependNode(markerPatchSet);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            if (markerPatchSet.hasFn(MFn::kSet))
            {
                MFnSet fnSet(markerPatchSet, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MString markerPatchName = fnSet.name(&status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MGlobal::displayInfo("Loading virtual marker " +
                                     markerPatchName);

                status = loadMarkerPatchPairing(markerPatchName);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }
    }

    return MS::kSuccess;
}

MStatus
SmoothMotionEditContext::loadMarkerPatchPairing(MString &markerPatchName)
{
    MStatus status;

    MFnMesh handMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string markerPatchNameChar = markerPatchName.asChar();

    string basePrefix = VIRTUAL_MARKER_BASE_PREFIX.asChar();
    int basePrefixPos = markerPatchNameChar.find(basePrefix);

    markerPatchNameChar.erase(basePrefixPos, basePrefix.length());

    MString associatedMocapMarkerName =
        MOCAP_MARKER_BASE_PREFIX + MString(markerPatchNameChar.c_str());

    MGlobal::displayInfo("Initializing marker patch: " + markerPatchName);

    // Store association
    string markerPatchNameOrigChar = markerPatchName.asChar();

    m_paired_marker_patches[markerPatchNameOrigChar] =
        associatedMocapMarkerName;

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::parseKinematicTree()
{
    MStatus status;

    MItDag itList(MItDag::TraversalType::kDepthFirst, MFn::kJoint, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (itList.isDone())
    {
        MGlobal::displayInfo(
            "No joints found - skipping kinematic tree parsing");
        return MS::kSuccess;
    }

    for (; !itList.isDone(); itList.next())
    {
        MDagPath jointPath;
        status = itList.getPath(jointPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MGlobal::displayInfo("Found root joint " + jointPath.partialPathName());

        m_rig_base = jointPath;
        break;
    }

    int nDofs = 0;
    int numJoints = 0;

    stack<MDagPath> stack;
    stack.push(m_rig_base);

    while (!stack.empty())
    {
        MDagPath dp = stack.top();
        stack.pop();

        // Ignore everything except joints
        if (!dp.hasFn(MFn::kJoint))
        {
            continue;
        }

        MFnIkJoint fnJoint(dp, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        bool freeX = false;
        bool freeY = false;
        bool freeZ = false;

        status = fnJoint.getDegreesOfFreedom(freeX, freeY, freeZ);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numJointDofs = 0;

        double jointRotations[3];
        MTransformationMatrix::RotationOrder rOrder;

        status = fnJoint.getRotation(jointRotations, rOrder);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (freeX)
        {
            pair<int, int> xRotDof = make_pair(numJoints, 0);
            m_rig_dof_vec_mappings.push_back(xRotDof);
            m_rig_dof_vector.append(jointRotations[0]);
            numJointDofs++;
            nDofs++;
        }
        if (freeY)
        {
            pair<int, int> yRotDof = make_pair(numJoints, 1);
            m_rig_dof_vec_mappings.push_back(yRotDof);
            m_rig_dof_vector.append(jointRotations[1]);
            numJointDofs++;
            nDofs++;
        }
        if (freeZ)
        {
            pair<int, int> zRotDof = make_pair(numJoints, 2);
            m_rig_dof_vec_mappings.push_back(zRotDof);
            m_rig_dof_vector.append(jointRotations[2]);
            numJointDofs++;
            nDofs++;
        }

        // Add translation dofs for the root
        if (numJoints == 0)
        {
            pair<int, int> xTransDof = make_pair(numJoints, 3);
            numJointDofs++;
            pair<int, int> yTransDof = make_pair(numJoints, 4);
            numJointDofs++;
            pair<int, int> zTransDof = make_pair(numJoints, 5);
            numJointDofs++;

            m_rig_dof_vec_mappings.push_back(xTransDof);
            m_rig_dof_vec_mappings.push_back(yTransDof);
            m_rig_dof_vec_mappings.push_back(zTransDof);

            MVector vTrans = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_rig_dof_vector.append(vTrans[0]);
            m_rig_dof_vector.append(vTrans[1]);
            m_rig_dof_vector.append(vTrans[2]);

            nDofs += 3;
        }

        // Ignore all welded joints
        if (numJointDofs > 0)
        {
            m_rig_joints.append(dp);
            numJoints++;
        }

        MString jointName = dp.partialPathName();
        m_joint_names.append(jointName);

        MGlobal::displayInfo(jointName + " " +
                             MString(to_string(numJoints).c_str()) + " " +
                             MString(to_string(numJointDofs).c_str()));

        int numChildren = dp.childCount();

        for (int i = 0; i < numChildren; i++)
        {
            status = dp.push(dp.child(i));
            CHECK_MSTATUS_AND_RETURN_IT(status);

            stack.push(dp);
            dp.pop();
        }
    }

    m_rig_n_dofs = nDofs;

    MGlobal::displayInfo(to_string(m_rig_n_dofs).c_str());
    MGlobal::displayInfo(to_string(m_rig_dof_vec_mappings.size()).c_str());

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        pair<int, int> jm = m_rig_dof_vec_mappings.at(i);
        float value = m_rig_dof_vector[i];

        int jointIndex = jm.first;
        int dofIndex = jm.second;

        MDagPath joint = m_rig_joints[jointIndex];
        MString dofName;

        switch (dofIndex)
        {
        case 0:
            dofName = "Rotate_X";
            break;
        case 1:
            dofName = "Rotate_Y";
            break;
        case 2:
            dofName = "Rotate_Z";
            break;
        case 3:
            dofName = "Translate_X";
            break;
        case 4:
            dofName = "Translate_Y";
            break;
        case 5:
            dofName = "Translate_Z";
            break;
        default:
            cout << "ERROR: Unknown dof index" << endl;
            break;
        }

        // Easier to look up in command lined
        cout << i << " " << joint.partialPathName() << " " << dofName << " "
             << value << endl;
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::pluckObjectAndHandMesh()
{
    MStatus status;

    MSelectionList selectionList;

    MString objectShapeName = OBJECT_NAME + "Shape";

    status = MGlobal::getSelectionListByName(objectShapeName, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("No object mesh found");
        return MS::kFailure;
    }

    status = selectionList.getDagPath(0, m_object_geometry);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool foundHandMesh = false;

    for (const auto &entry : m_global_geometry_map)
    {
        string meshNameChar = entry.first;

        MString meshName = meshNameChar.c_str();

        // Should only ever be one per scene
        if (meshName != OBJECT_NAME)
        {
            m_hand_name = meshName;
            foundHandMesh = true;
            break;
        }
    }

    if (!foundHandMesh)
    {
        MGlobal::displayInfo("No hand mesh found");
        return MS::kFailure;
    }

    selectionList.clear();

    MString handShapeName = m_hand_name + "Shape";

    status = MGlobal::getSelectionListByName(m_hand_name, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    selectionList.clear();

    status = MGlobal::getSelectionListByName(handShapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.getDagPath(0, m_hand_geometry);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

// Visualization Utils

MStatus SmoothMotionEditContext::createVisualizationSphere(
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

MStatus SmoothMotionEditContext::redrawContactVisualizations()
{
    MStatus status;

    status = clearVisualizations(CONTACT_PREFIX);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = visualizeAllContacts();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = wipeContactPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vector<MPointArray> pairedPointLocations;
    vector<MVectorArray> pairedPointNormals;

    status =
        getPairedFrameContactPoints(pairedPointLocations, pairedPointNormals);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject contactLineGroup = fnTransform.create();

    MFnTransform fnContactLineTransform(contactLineGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnContactLineTransform.setName(CONTACT_PAIRING_LINES_GROUP, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numPairs = pairedPointLocations.size();

    MFnNurbsCurve fnCurve;

    for (int i = 0; i < numPairs; i++)
    {
        MPointArray pairedPoints = pairedPointLocations[i];

        fnCurve.createWithEditPoints(pairedPoints, 1, MFnNurbsCurve::kOpen,
                                     false, true, true, contactLineGroup,
                                     &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(false, true);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::redrawMarkerVisualizations()
{
    MStatus status;

    status = visualizeAllVirtualMarkers();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vector<MPointArray> pairedPointLocations;

    status = computePairedMarkerPatchLocations(pairedPointLocations);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject markerLineGroup = fnTransform.create();

    MFnTransform fnMarkerLineTransform(markerLineGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnMarkerLineTransform.setName(MARKER_PAIRING_LINES_GROUP, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numPairs = pairedPointLocations.size();

    MFnNurbsCurve fnCurve;

    for (int i = 0; i < numPairs; i++)
    {
        MPointArray pairedPoints = pairedPointLocations[i];

        fnCurve.createWithEditPoints(pairedPoints, 1, MFnNurbsCurve::kOpen,
                                     false, true, true, markerLineGroup,
                                     &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(false, true);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::visualizeAllContacts()
{
    MStatus status;

    MSelectionList selectionList;

    MString allContactsGroupName =
        CONTACT_GROUP_PREFIX + MString(to_string(m_frame).c_str());

    status =
        MGlobal::getSelectionListByName(allContactsGroupName, selectionList);

    // No contact information available
    if (status != MS::kSuccess)
    {
        return MS::kSuccess;
    }

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = m_hand_name + "Shape_";

    MString objectContactGroupName = CONTACT_GROUP_PREFIX + objectShapeName +
                                     MString(to_string(m_frame).c_str());

    MString handContactGroupName = CONTACT_GROUP_PREFIX + handShapeName +
                                   MString(to_string(m_frame).c_str());

    status = visualizePatch(fnObjectMesh, objectContactGroupName,
                            CONTACT_POINTS_ATTRIBUTE, OBJECT_PATCH_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = visualizePatch(fnHandMesh, handContactGroupName,
                            CONTACT_POINTS_ATTRIBUTE, HAND_PATCH_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::visualizeAllVirtualMarkers()
{
    MStatus status;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Iterate through markers
    for (auto const &entry : m_paired_marker_patches)
    {
        string markerPatchNameChar = entry.first;
        MString markerPatchName = MString(markerPatchNameChar.c_str());

        status =
            visualizePatch(fnHandMesh, markerPatchName, MARKER_POINTS_ATTRIBUTE,
                           VIRTUAL_MARKER_PATCH_COLOR);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::visualizePatch(MFnMesh &fnMesh,
                                                MString &patchName,
                                                MString attributeName,
                                                MColor vizColor)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    string patchNameChar = patchName.asChar();

    MStringArray serializedPatchPoints;
    status = getSetSerializedPointsAttribute(patchName, attributeName,
                                             serializedPatchPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject patchGroup;

    if (!m_patch_visualization_map.contains(patchNameChar))
    {
        MFnTransform fnTransform;
        patchGroup = fnTransform.create(MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString patchVisName = PATCH_VIS_BASE_PREFIX + patchName;

        fnTransform.setName(patchVisName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Get a colored Lambertian shading node for the patch
        MString shadingNodeName = patchVisName + PATCH_SHADER_SUFFIX;

        snprintf(command, COMMAND_BUFFER_SIZE,
                 "shadingNode -asShader lambert -name %s",
                 shadingNodeName.asChar());
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        snprintf(command, COMMAND_BUFFER_SIZE,
                 "setAttr %s.color -type double3 %f %f %f",
                 shadingNodeName.asChar(), vizColor[0], vizColor[1],
                 vizColor[2]);
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = selectionList.clear();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numPatchPoints = serializedPatchPoints.length();

        for (int i = 0; i < numPatchPoints; i++)
        {
            MString serializedPatchPoint = serializedPatchPoints[i];

            status = createVisualizationSphere(
                fnMesh, serializedPatchPoint, PATCH_POINT_SPHERE_NAME,
                DEFAULT_SPHERE_SIZE, shadingNodeName, fnTransform);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        m_patch_visualization_map[patchNameChar] = patchGroup;
    }

    else
    {
        patchGroup = m_patch_visualization_map[patchNameChar];

        MFnDagNode fnGroupDagNode(patchGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numPatchPoints = serializedPatchPoints.length();
        int sphereCount = fnGroupDagNode.childCount();

        if (numPatchPoints != sphereCount)
        {
            MGlobal::displayInfo(
                "Error - sphere count does not match point count");
            return MS::kFailure;
        }

        vector<int> vertexIndices;
        vector<double> coords;

        MFloatPoint position;
        MFloatVector normal;

        for (int i = 0; i < numPatchPoints; i++)
        {
            vertexIndices.clear();
            coords.clear();

            MString serializedPatchPoint = serializedPatchPoints[i];

            MObject sphere = fnGroupDagNode.child(i, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnDagNode sphereDagNode(sphere, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MDagPath sphereDag;
            status = sphereDagNode.getPath(sphereDag);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnTransform fnSphereTransform(sphereDag, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = parseSerializedPoint(fnMesh, serializedPatchPoint,
                                          vertexIndices, coords);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = interpolateSerializedPoint(fnMesh, vertexIndices, coords,
                                                position, normal);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MVector trans(position[0], position[1], position[2]);
            fnSphereTransform.setTranslation(trans, MSpace::kWorld);
        }
    }

    return MS::kSuccess;
}

// Core Utils

MStatus SmoothMotionEditContext::computePairedMarkerPatchLocations(
    vector<MPointArray> &pointLocations)
{
    MStatus status;

    MFnMesh fnMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MSelectionList selectionList;

    vector<int> vertexIndices;
    vector<double> coords;

    MFloatPoint position;
    MFloatVector normal;

    for (auto const &entry : m_paired_marker_patches)
    {
        vector<MPointArray> points;

        string markerPatchNameChar = entry.first;
        MString mocapMarkerName = entry.second;

        MString markerPatchName = MString(markerPatchNameChar.c_str());

        MStringArray serializedMarkerMatchPoints;
        status = getSetSerializedPointsAttribute(markerPatchName,
                                                 MARKER_POINTS_ATTRIBUTE,
                                                 serializedMarkerMatchPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath mocapMarkerDag;
        status = getMocapMarker(mocapMarkerName, mocapMarkerDag);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnTransform fnMocapTransform(mocapMarkerDag, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MTransformationMatrix tfmocap =
            fnMocapTransform.transformation(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPoint mocapMarkerPoint = MPoint::origin;

        mocapMarkerPoint *= tfmocap.asMatrix();

        int numPoints = serializedMarkerMatchPoints.length();

        for (int i = 0; i < numPoints; i++)
        {
            vertexIndices.clear();
            coords.clear();

            MString serializedMarkerMatchPoint = serializedMarkerMatchPoints[i];

            status = parseSerializedPoint(fnMesh, serializedMarkerMatchPoint,
                                          vertexIndices, coords);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = interpolateSerializedPoint(fnMesh, vertexIndices, coords,
                                                position, normal);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MPointArray endpoints;
            endpoints.append(position);
            endpoints.append(mocapMarkerPoint);

            pointLocations.push_back(endpoints);
        }
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::fitHandDofSplines(int numControlPoints)
{
    MStatus status;

    MSelectionList selectionList;

    vector<vector<Vector2<double>>> allFrameDofValues;
    map<int, set<int>> accelerationFrameViolations;

    for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
    {
        vector<Vector2<double>> frameDofValues;
        allFrameDofValues.push_back(frameDofValues);
    }

    MStringArray serializedAccelerationViolations;

    status = getSerializedViolationsAttribute(serializedAccelerationViolations);

    if (status != MS::kSuccess)
    {
        MGlobal::displayError("Failed to get serialized violations attribute - "
                              "using all frames by default");
    }

    MStringArray serializedAccelerationViolationTokens;

    for (int i = 0; i < serializedAccelerationViolations.length(); i++)
    {
        MString serializedAccelerationViolation =
            serializedAccelerationViolations[i];

        status = serializedAccelerationViolationTokens.clear();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = serializedAccelerationViolation.split(
            ' ', serializedAccelerationViolationTokens);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int violationFrame =
            atoi(serializedAccelerationViolationTokens[0].asChar());

        for (int j = 1; j < serializedAccelerationViolationTokens.length(); j++)
        {
            int violationDofIndex =
                atoi(serializedAccelerationViolationTokens[j].asChar());

            if (accelerationFrameViolations.contains(violationFrame))
            {
                accelerationFrameViolations[violationFrame].insert(
                    violationDofIndex);
            }
            else
            {
                set<int> newViolationSet = {violationDofIndex};
                accelerationFrameViolations[violationFrame] = newViolationSet;
            }
        }
    }

    status = MGlobal::getSelectionListByName(
        DEFAULT_HAND_DOF_SPLINE_STORAGE_GROUP, selectionList);

    if (status == MS::kSuccess)
    {
        MObject existingSplineGroup;
        status = selectionList.getDependNode(0, existingSplineGroup);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = MGlobal::deleteNode(existingSplineGroup);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MFnTransform fnTransform;
    MObject splineGroup = fnTransform.create();

    MFnTransform fnSplineGroupTransform(splineGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnSplineGroupTransform.setName(DEFAULT_HAND_DOF_SPLINE_STORAGE_GROUP, false,
                                   &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = setNumericAttribute(DEFAULT_HAND_DOF_SPLINE_STORAGE_GROUP,
                                 SPLINE_RANGE_START, m_start_frame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = setNumericAttribute(DEFAULT_HAND_DOF_SPLINE_STORAGE_GROUP,
                                 SPLINE_RANGE_END, m_end_frame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDoubleArray splineKnots;

    // Assume standard knot vector with full endpoint multiplicity
    double knotIndex = 0;

    for (int i = 0; i < SPLINE_DEGREE; i++)
    {
        status = splineKnots.append(knotIndex);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    for (int i = 0; i < numControlPoints - SPLINE_DEGREE - 1; i++)
    {
        knotIndex++;
        status = splineKnots.append(knotIndex);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    knotIndex++;

    for (int i = 0; i < SPLINE_DEGREE; i++)
    {
        status = splineKnots.append(knotIndex);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    double maxKnotIndex = 1.0 * knotIndex;

    // Normalize knot vector
    for (int i = 0; i < splineKnots.length(); i++)
    {
        splineKnots[i] /= maxKnotIndex;
    }

    for (int frame = m_start_frame; frame <= m_end_frame; frame++)
    {
        status = jumpToFrame(frame, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = loadSceneRigState();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numDofViolations = 0;

        for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
        {
            // If violation, do not use dof solution as control point
            if (accelerationFrameViolations.contains(frame) &&
                accelerationFrameViolations[frame].contains(rigDofIndex))
            {
                cout << "Ignoring index " << rigDofIndex << " at frame "
                     << frame << " due to accleration violation" << endl;

                numDofViolations++;
                continue;
            }

            Vector2<double> samplePoint;
            samplePoint[0] = (double)frame;
            samplePoint[1] = m_rig_dof_vector[rigDofIndex];

            allFrameDofValues[rigDofIndex].push_back(samplePoint);
        }
    }

    float multiplier = 1.0f / (m_end_frame - 1.0f);

    vector<MDoubleArray> allRigDofTrajectories;

    for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
    {
        unique_ptr<BSplineCurveFit<double>> rigDofSpline =
            make_unique<BSplineCurveFit<double>>(
                SPLINE_DIMENSION, allFrameDofValues[rigDofIndex].size(),
                reinterpret_cast<double const *>(
                    &allFrameDofValues[rigDofIndex][0]),
                SPLINE_DEGREE, numControlPoints);

        MPointArray controlPoints;

        const double *controlData = rigDofSpline->GetControlData();

        for (int controlIndex = 0; controlIndex < numControlPoints;
             controlIndex++)
        {
            int jumpIndex = controlIndex * SPLINE_DIMENSION;

            double controlPosition = controlData[jumpIndex];
            double controlValue = controlData[jumpIndex + 1];

            if (rigDofIndex < 3 || rigDofIndex > 5)
            {
                controlValue *= 180.0 / M_PI;
            }

            MPoint controlPoint(controlPosition, controlValue);

            status = controlPoints.append(controlPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        MFnNurbsCurve fnSplineGenerator;

        MObject splineDofCurve = fnSplineGenerator.create(
            controlPoints, splineKnots, SPLINE_DEGREE, MFnNurbsCurve::kOpen,
            true, true, splineGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString splineName = SPLINE_DOF_PREFIX + rigDofIndex;

        fnSplineGenerator.setName(splineName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::getMocapMarker(MString &mocapMarkerName,
                                                MDagPath &mocapMarkerDag)
{
    MStatus status;

    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(mocapMarkerName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject mocapMarker;
    status = selectionList.getDependNode(0, mocapMarker);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDagNode mocapMarkerDagNode(mocapMarker, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mocapMarkerDagNode.getPath(mocapMarkerDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::getPairedFrameContactPoints(
    vector<MPointArray> &pairedContactPointLocations,
    vector<MVectorArray> &pairedContactPointNormals)
{
    MStatus status;

    MSelectionList selectionList;

    pairedContactPointLocations.clear();
    pairedContactPointNormals.clear();

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedObjectContactPoints;
    MStringArray serializedHandContactPoints;

    MFloatPoint handPointPosition;
    MFloatVector handPointNormal;

    MFloatPoint objectPointPosition;
    MFloatVector objectPointNormal;

    vector<int> vertexIndices;
    vector<double> coords;

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = m_hand_name + "Shape_";

    MString allContactsGroupName =
        CONTACT_GROUP_PREFIX + MString(to_string(m_frame).c_str());

    MString objectContactGroupName = CONTACT_GROUP_PREFIX + objectShapeName +
                                     MString(to_string(m_frame).c_str());

    MString handContactGroupName = CONTACT_GROUP_PREFIX + handShapeName +
                                   MString(to_string(m_frame).c_str());

    status =
        MGlobal::getSelectionListByName(allContactsGroupName, selectionList);

    // No contact information available
    if (status != MS::kSuccess)
    {
        return MS::kSuccess;
    }

    status = getSetSerializedPointsAttribute(objectContactGroupName,
                                             CONTACT_POINTS_ATTRIBUTE,
                                             serializedObjectContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = getSetSerializedPointsAttribute(handContactGroupName,
                                             CONTACT_POINTS_ATTRIBUTE,
                                             serializedHandContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numContactPoints = serializedObjectContactPoints.length();

    for (int cpi = 0; cpi < numContactPoints; cpi++)
    {
        MString serializedObjectContactPoint =
            serializedObjectContactPoints[cpi];
        MString serializedHandContactPoint = serializedHandContactPoints[cpi];

        MPointArray pointLocationPair;
        MVectorArray pointNormalPair;

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

        status = pointLocationPair.append(objectPointPosition);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = pointLocationPair.append(handPointPosition);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = pointNormalPair.append(objectPointNormal);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = pointNormalPair.append(handPointNormal);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        pairedContactPointLocations.push_back(pointLocationPair);
        pairedContactPointNormals.push_back(pointNormalPair);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::getSerializedViolationsAttribute(
    MStringArray &serializedFrameViolations)
{
    MStatus status;

    MSelectionList selectionList;

    MObject setObject;

    status = MGlobal::getSelectionListByName(ACCELERATION_ERROR_STORAGE_GROUP,
                                             selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("No acceleration error storage group found");
        return MS::kFailure;
    }

    status = selectionList.getDependNode(0, setObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(ACCELERATION_ERROR_STORAGE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MGlobal::displayInfo("WARNING: Attribute " +
                             ACCELERATION_ERROR_STORAGE_ATTRIBUTE +
                             " not found");
        return MS::kFailure;
    }

    attrObj =
        fnDepNode.attribute(ACCELERATION_ERROR_STORAGE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(setObject, attrObj);

    MObject attributeData = attributePlug.asMObject();

    MFnStringArrayData fnStringArrayData(attributeData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = serializedFrameViolations.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnStringArrayData.length(); i++)
    {
        MString item = fnStringArrayData[i];
        serializedFrameViolations.append(item);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::getNumericAttribute(MString setName,
                                                     MString attributeName,
                                                     int &value)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(setName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject obj;
    status = selectionList.getDependNode(0, obj);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(obj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!hasAttribute)
    {
        MGlobal::displayInfo("WARNING: Numeric attribute " + attributeName +
                             " not found");
        return MS::kFailure;
    }

    MObject attrObj = fnDepNode.attribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(obj, attrObj);

    value = attributePlug.asInt();

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::getSetSerializedPointsAttribute(
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

    status = serializedPoints.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnStringArrayData.length(); i++)
    {
        MString item = fnStringArrayData[i];
        serializedPoints.append(item);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::interpolateSerializedPoint(
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

MStatus SmoothMotionEditContext::keyframeRig()
{
    MStatus status;

    for (int i = 0; i < m_joint_names.length(); i++)
    {
        MString jointName = m_joint_names[i];

        MString saveCommand = "setKeyframe { \"" + jointName + "\" }";
        status = MGlobal::executeCommand(saveCommand);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::loadRigDofSolutionFull()
{
    MStatus status;

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        status = loadRigDofSolutionSingle(i);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::loadRigDofSolutionSingle(int rigDofIndex)
{
    MStatus status;

    double value = m_rig_dof_vector[rigDofIndex];
    pair<int, int> indices = m_rig_dof_vec_mappings.at(rigDofIndex);

    int jointIndex = indices.first;
    int dofIndex = indices.second;

    MDagPath joint = m_rig_joints[jointIndex];

    MFnIkJoint fnJoint(joint, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (dofIndex > 2) // indicates translation dof
    {
        MVector transVec = fnJoint.getTranslation(MSpace::kWorld, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        transVec[dofIndex - 3] = value;

        status = fnJoint.setTranslation(transVec, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        double rotation[3];
        MTransformationMatrix::RotationOrder order;

        status = fnJoint.getRotation(rotation, order);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        rotation[dofIndex] = value;

        status = fnJoint.setRotation(rotation, order);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::loadSceneRigState()
{
    MStatus status;

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        pair<int, int> indices = m_rig_dof_vec_mappings.at(i);

        int jointIndex = indices.first;
        int dofIndex = indices.second;

        MDagPath joint = m_rig_joints[jointIndex];

        MFnIkJoint fnJoint(joint, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (dofIndex > 2) // indicates translation dof
        {
            MVector transVec = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_rig_dof_vector[i] = transVec[dofIndex - 3];
        }
        else
        {
            double rotation[3];
            MTransformationMatrix::RotationOrder order;

            status = fnJoint.getRotation(rotation, order);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_rig_dof_vector[i] = rotation[dofIndex];
        }
    }

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::localDofSplineSearch(
    BSplineCurve<double> &dofSpline, Vector2<double> &solution,
    int frameDesired, double tInitialGuess)
{
    MStatus status;

    double tGuess = tInitialGuess;
    double fd = (double)frameDesired;

    dofSpline.GetPosition(tGuess, reinterpret_cast<double *>(&solution[0]));

    double splineFrame = solution[0];

    if (abs(splineFrame - fd) < SPLINE_SEARCH_EPSILON)
    {
        cout << "Converged - found solution for frame " << frameDesired
             << " at " << splineFrame << endl;
        return MS::kSuccess;
    }

    // Find a decent lower and upper bound
    double tLB = 0.0;
    double tUB = 1.0;

    Vector2<double> tmp;
    double tGuessBound;
    int boundSearchDirection;

    if (splineFrame > fd)
    {
        tUB = tGuess;
        boundSearchDirection = -1;
    }
    else
    {
        tLB = tGuess;
        boundSearchDirection = 1;
    }

    bool opposingBoundFound = false;
    double boundStep = 0;

    do
    {
        tGuessBound =
            tGuess + (boundSearchDirection *
                      SPLINE_SEARCH_BOUND_SEARCH_STEP_SIZE * boundStep);

        dofSpline.GetPosition(tGuessBound, reinterpret_cast<double *>(&tmp[0]));

        splineFrame = tmp[0];

        if (boundSearchDirection == -1 && splineFrame < fd)
        {
            tLB = tGuessBound;
            opposingBoundFound = true;
        }
        else if (boundSearchDirection == 1 && splineFrame > fd)
        {
            tUB = tGuessBound;
            opposingBoundFound = true;
        }

        boundStep++;
    } while (!opposingBoundFound);

    // Then perform the binary search, which should terminate quickly
    int numIterations = 0;

    while (numIterations < SPLINE_SEARCH_MAX_ITERATIONS)
    {
        tGuess = (tLB + tUB) / 2.0;

        dofSpline.GetPosition(tGuess, reinterpret_cast<double *>(&solution[0]));

        splineFrame = solution[0];

        if (abs(splineFrame - fd) < SPLINE_SEARCH_EPSILON)
        {
            break;
        }
        else if (splineFrame > fd)
        {
            tUB = tGuess;
        }
        else
        {
            tLB = tGuess;
        }

        cout << "Iteration # " << numIterations << ", desired = " << fd
             << ", tGuess = " << tGuess << ", value = " << splineFrame << endl;

        numIterations++;
    }

    if (numIterations >= SPLINE_SEARCH_MAX_ITERATIONS)
    {
        cout << "Unable to find spline search solution - all iterations "
                "exhausted"
             << endl;
        return MS::kFailure;
    }

    cout << "Converged - found solution for frame " << frameDesired << " at "
         << splineFrame << endl;

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::parseSerializedPoint(MFnMesh &fnMesh,
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

MStatus SmoothMotionEditContext::setNumericAttribute(MString objName,
                                                     MString attributeName,
                                                     int &value)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(objName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject obj;
    status = selectionList.getDependNode(0, obj);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(obj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnNumericAttribute fnNumericAttr;

        attrObj = fnNumericAttr.create(attributeName, attributeName,
                                       MFnNumericData::kInt, 0.0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnDepNode.attribute(attributeName, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(obj, attrObj);

    status = attributePlug.setInt(value);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::wipeContactPairingLines()
{
    MStatus status;

    status = wipePairingLines(CONTACT_PAIRING_LINES_GROUP);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::wipeMarkerPairingLines()
{
    MStatus status;

    status = wipePairingLines(MARKER_PAIRING_LINES_GROUP);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContext::wipePairingLines(MString prefix)
{
    MStatus status;

    MSelectionList selectionList;
    MString linesRegex = MString(prefix) + MString("*");
    status = MGlobal::getSelectionListByName(linesRegex, selectionList);

    if (status == MS::kSuccess)
    {
        MItSelectionList lineItList(selectionList);

        for (; !lineItList.isDone(); lineItList.next())
        {
            MObject lineGroup;
            status = lineItList.getDependNode(lineGroup);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MGlobal::deleteNode(lineGroup);
        }
    }

    return MS::kSuccess;
}

// Core Context Teardown

MStatus SmoothMotionEditContext::clearVisualizations(MString filterString)
{
    MStatus status;

    MString patchRegex = PATCH_VIS_BASE_PREFIX + filterString + MString("*");

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(patchRegex, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("No visualizations found - cleared nothing");
        return MS::kSuccess;
    }

    MItSelectionList itList(selectionList);

    for (; !itList.isDone(); itList.next())
    {
        MObject node;
        status = itList.getDependNode(node);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = MGlobal::deleteNode(node);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (filterString != MString())
    {
        MStringArray keysToDelete;

        for (const auto &entry : m_patch_visualization_map)
        {
            MString key = entry.first.c_str();

            int filterStringIndex = key.indexW(filterString);

            if (filterStringIndex != -1)
            {
                status = keysToDelete.append(key);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }

        for (int i = 0; i < keysToDelete.length(); i++)
        {
            MString deleteKey = keysToDelete[i];

            string deleteKeyChar = deleteKey.asChar();
            m_patch_visualization_map.erase(deleteKeyChar);
        }
    }
    else
    {
        m_patch_visualization_map.clear();
    }

    return MS::kSuccess;
}
