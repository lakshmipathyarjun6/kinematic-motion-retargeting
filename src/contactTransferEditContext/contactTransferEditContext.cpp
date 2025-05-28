#include "contactTransferEditContext.hpp"

// Basic Setup and Teardown

ContactTransferEditContext::ContactTransferEditContext()
    : m_color_pallette_index(0), m_contact_parameterized_distance_threshold(2.0)
{
    MColor ac0 = MColor(0.0, 1.0, 1.0);       // Index
    MColor ac1 = MColor(0.403, 0.934, 0.403); // Middle
    MColor ac2 = MColor(1.0, 0.462, 0.020);   // Pinky
    MColor ac3 = MColor(0.447, 0.294, 1.0);   // Ring
    MColor ac4 = MColor(0.934, 0.256, 0.256); // Thumb

    m_color_pallette.push_back(ac0);
    m_color_pallette.push_back(ac1);
    m_color_pallette.push_back(ac2);
    m_color_pallette.push_back(ac3);
    m_color_pallette.push_back(ac4);

    string sourceHandNameChar = SOURCE_HAND_NAME.asChar();

    m_contact_colors[sourceHandNameChar] = SOURCE_CONTACT_COLOR;
    m_contact_sphere_names[sourceHandNameChar] = SOURCE_CONTACT_SPHERE_NAME;
}

ContactTransferEditContext::~ContactTransferEditContext() {}

void *ContactTransferEditContext::creator()
{
    return new ContactTransferEditContext();
}

void ContactTransferEditContext::toolOnSetup(MEvent &event)
{
    MStatus status;

    MGlobal::displayInfo("Setting up contact transfer edit context...");

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(SOURCE_HAND_NAME, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("Source hand MUST have the name " +
                             SOURCE_HAND_NAME);
        return;
    }

    m_color_table.clear();
    m_color_pallette_index = 0;

    m_paired_axes.clear();
    m_paired_axes_pair_keys.clear();
    m_paired_axes_unique_pairs.clear();

    m_view = M3dView::active3dView();

    MAnimControl animCtrl;
    MTime time = animCtrl.currentTime();
    m_frame = (int)time.value();

    status = loadAllGeometries();
    CHECK_MSTATUS(status);

    status = pluckTargetMesh();
    CHECK_MSTATUS(status);

    status = loadAllAxes();
    CHECK_MSTATUS(status);

    status = loadSavedPairings();
    CHECK_MSTATUS(status);

    status = visualizeAllAxes();
    CHECK_MSTATUS(status);

    MGlobal::clearSelectionList();

    MGlobal::displayInfo("Done");
}

void ContactTransferEditContext::toolOffCleanup()
{
    MStatus status;

    MGlobal::displayInfo("Tearing down contact transfer edit context...");

    status = clearVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

// Tool Sheet Reference Properties

void ContactTransferEditContext::getClassName(MString &name) const
{
    name.set("contactTransferEditContext");
}

MStatus ContactTransferEditContext::commitParameterizedDistancesFilter(
    int frameStart, int frameEnd, double parameterizedFilterDistanceThreshold)
{
    MStatus status;

    MString targetTransformName = m_target_mesh_name;

    for (int i = frameStart; i <= frameEnd; i++)
    {
        status = jumpToFrame(i, false);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDoubleArray frameParameterizedContactPointDistances;
        MIntArray ommissionIndices;

        MString sourceContactGroupName = CONTACT_GROUP_PREFIX +
                                         SOURCE_HAND_NAME + "Shape_" +
                                         MString(to_string(m_frame).c_str());

        MString targetContactGroupName = CONTACT_GROUP_PREFIX +
                                         targetTransformName + "Shape_" +
                                         MString(to_string(m_frame).c_str());

        status = getParameterizedContactDistanceAttribute(
            targetContactGroupName, frameParameterizedContactPointDistances);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numContactPoints = frameParameterizedContactPointDistances.length();

        for (int i = 0; i < numContactPoints; i++)
        {
            double distance = frameParameterizedContactPointDistances[i];

            if (distance > m_contact_parameterized_distance_threshold)
            {
                status = ommissionIndices.append(i);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }

        if (numContactPoints > 0)
        {
            status = setOmissionIndicesAttribute(sourceContactGroupName,
                                                 ommissionIndices);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = setOmissionIndicesAttribute(targetContactGroupName,
                                                 ommissionIndices);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::dumpParameterizedDistances(int frameStart,
                                                               int frameEnd)
{
    MStatus status;

    ofstream dumpfile(DISTANCE_DUMP_FILENAME);

    MString targetTransformName = m_target_mesh_name;

    if (dumpfile.is_open())
    {
        for (int i = frameStart; i <= frameEnd; i++)
        {
            MGlobal::displayInfo(
                "Dumping parameterized contact distances for frame " +
                MString(to_string(i).c_str()));

            status = jumpToFrame(i, false);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MDoubleArray frameParameterizedContactPointDistances;

            MString targetContactGroupName =
                CONTACT_GROUP_PREFIX + targetTransformName + "Shape_" +
                MString(to_string(m_frame).c_str());

            status = getParameterizedContactDistanceAttribute(
                targetContactGroupName,
                frameParameterizedContactPointDistances);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int numContactPoints =
                frameParameterizedContactPointDistances.length();

            for (int j = 0; j < numContactPoints; j++)
            {
                double parameterizedDistance =
                    frameParameterizedContactPointDistances[j];

                dumpfile << parameterizedDistance << "\n";
            }
        }

        dumpfile.close();
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::jumpToFrame(int frame, bool visualize)
{
    MStatus status;

    m_frame = frame;

    MAnimControl animCtrl;
    MTime newFrame((double)m_frame, FRAMERATE);

    status = animCtrl.setCurrentTime(newFrame);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (visualize)
    {
        status = clearContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = loadAllFrameContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MGlobal::clearSelectionList();

    return MS::kSuccess;
}

MStatus
ContactTransferEditContext::setContactParameterizedFilterDistanceThreshold(
    double parameterizedFilterDistanceThreshold)
{
    MStatus status;

    MGlobal::displayInfo(
        "Adjusting contact parameterized filter distance to: " +
        MString(to_string(parameterizedFilterDistanceThreshold).c_str()));

    m_contact_parameterized_distance_threshold =
        parameterizedFilterDistanceThreshold;

    status = clearContactVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = loadAllFrameContacts();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::transferContacts(bool visualize,
                                                     bool performInitialization)
{
    MStatus status;

    MGlobal::displayInfo("Transferring contacts in frame " +
                         MString(to_string(m_frame).c_str()) + "...");

    MSelectionList selectionList;

    // Try loading source frame contacts

    MString sourceHandContactGroup = CONTACT_GROUP_PREFIX + SOURCE_HAND_NAME +
                                     "Shape_" +
                                     MString(to_string(m_frame).c_str());

    status =
        MGlobal::getSelectionListByName(sourceHandContactGroup, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo(
            "No source hand contact points found in frame - did nothing");
        return MS::kSuccess;
    }

    MString sourceTransformName = SOURCE_HAND_NAME;
    MString targetTransformName = m_target_mesh_name;

    string sourceTransformNameChar = sourceTransformName.asChar();
    string targetTransformNameChar = targetTransformName.asChar();

    GeometryProcessingContext *sourceGPC =
        m_global_geometry_processing_context_map[sourceTransformNameChar];

    GeometryProcessingContext *targetGPC =
        m_global_geometry_processing_context_map[targetTransformNameChar];

    MStringArray serializedSourceContactPoints;
    MStringArray serializedTargetContactPoints;
    MDoubleArray targetContactPointParameterizedDistances;

    MString targetContactGroupName = CONTACT_GROUP_PREFIX +
                                     targetTransformName + "Shape_" +
                                     MString(to_string(m_frame).c_str());

    status = getContactAttribute(sourceHandContactGroup,
                                 serializedSourceContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    map<string, double> targetContactSpreadScales;

    for (auto const &sourceTargetNameMappingEntry : m_paired_axes)
    {
        MString targetAxisName = sourceTargetNameMappingEntry.second;
        string targetAxisNameChar = targetAxisName.asChar();

        double targetContactSpreadScale =
            m_target_radial_scales[targetAxisNameChar];

        targetContactSpreadScales[targetAxisNameChar] =
            targetContactSpreadScale;
    }

    if (performInitialization)
    {
        status = sourceGPC->initializeLandmarkParameterization();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    status = sourceGPC->parameterizeAllContactsFromLandmarks(
        serializedSourceContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = sourceGPC->transferAllContacts(targetGPC, m_paired_axes,
                                            targetContactSpreadScales);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = targetGPC->getReassembledContactsFromAxisGroups(
        serializedTargetContactPoints,
        targetContactPointParameterizedDistances);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = setContactAttribute(targetContactGroupName,
                                 serializedTargetContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = setParameterizedContactDistanceAttribute(
        targetContactGroupName, targetContactPointParameterizedDistances);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (visualize)
    {
        status = clearContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = loadAllFrameContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MGlobal::clearSelectionList();

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::transferContactsBulk(int frameStart,
                                                         int frameEnd)
{
    MStatus status;

    MAnimControl animCtrl;
    int maxFrame = (int)animCtrl.maxTime().as(FRAMERATE);

    MGlobal::displayInfo("Transferring contact in all frames...");

    status = clearContactVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString sourceTransformName = SOURCE_HAND_NAME;

    string sourceTransformNameChar = sourceTransformName.asChar();

    GeometryProcessingContext *sourceGPC =
        m_global_geometry_processing_context_map[sourceTransformNameChar];

    status = sourceGPC->initializeLandmarkParameterization();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int frame = frameStart; frame <= frameEnd; frame++)
    {
        status = jumpToFrame(frame, false);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = transferContacts(false, false);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    status = loadAllFrameContacts();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

// Core Context Setup

MStatus ContactTransferEditContext::loadAllAxes()
{
    MStatus status;

    for (auto const &it : m_global_geometry_map)
    {
        string transformNameChar = it.first;
        MString transformName = transformNameChar.c_str();

        GeometryProcessingContext *gpc =
            m_global_geometry_processing_context_map[transformNameChar];

        // Find existing mesh axes
        MString axisRegex = AXIS_BASE_PREFIX + transformName + "Shape*";

        MSelectionList axisSelectionList;
        status = MGlobal::getSelectionListByName(axisRegex, axisSelectionList);

        // If failure, then the mesh has no contacts
        if (status == MStatus::kFailure)
        {
            MGlobal::displayInfo("No axes found for mesh " + transformName);
            continue;
        }

        MItSelectionList axisItList(axisSelectionList);
        for (; !axisItList.isDone(); axisItList.next())
        {
            MObject axisSet;
            status = axisItList.getDependNode(axisSet);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            if (axisSet.hasFn(MFn::kSet))
            {
                MFnSet fnSet(axisSet, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MString axisName = fnSet.name(&status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MGlobal::displayInfo("Found axis " + axisName);

                string axisNameChar = axisName.asChar();

                string patchShaderSuffixChar = PATCH_SHADER_SUFFIX.asChar();

                if (axisNameChar.find(patchShaderSuffixChar) != string::npos)
                {
                    MGlobal::displayInfo("Skipping shader set " + axisName);
                    continue;
                }

                // Assign color
                m_color_table[axisNameChar] =
                    m_color_pallette[m_color_pallette_index];
                m_color_pallette_index =
                    (m_color_pallette_index + 1) % m_color_pallette.size();

                // Add axis to global map
                if (!m_global_axis_map[transformNameChar].contains(
                        axisNameChar))
                {
                    m_global_axis_map[transformNameChar].insert(axisNameChar);

                    m_global_axis_meshes[axisNameChar] = transformName;

                    m_all_axes.insert(axisNameChar);

                    MStringArray serializedAxisPoints;
                    status = getAxisAttribute(axisName, serializedAxisPoints);
                    CHECK_MSTATUS_AND_RETURN_IT(status);

                    status = gpc->registerAxis(axisName, serializedAxisPoints);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                }

                double radialScale;
                status = getContactSpreadScaleAttribute(axisName, radialScale);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                cout << axisName << " radial scale: " << radialScale << endl;

                m_target_radial_scales[axisNameChar] = radialScale;
            }
        }
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::loadAllGeometries()
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

        if (!m_global_geometry_map.contains(transformNameChar))
        {
            m_global_geometry_map[transformNameChar] = meshPath;

            m_global_geometry_processing_context_map[transformNameChar] =
                new GeometryProcessingContext();

            status = m_global_geometry_processing_context_map[transformNameChar]
                         ->registerGeometry(meshPath, transformPath);
            CHECK_MSTATUS_AND_RETURN_IT(status);

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

MStatus ContactTransferEditContext::loadSavedPairings()
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(PERSISTANT_STORAGE_OBJECT_NAME,
                                             selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject defaultSet;
    status = selectionList.getDependNode(0, defaultSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDN(defaultSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString attrName1(PERSISTANT_STORAGE_PAIRED_AXIS_FIRST_NAME);
    MString attrName2(PERSISTANT_STORAGE_PAIRED_AXIS_SECOND_NAME);

    bool hasAttributes = fnDN.hasAttribute(attrName1, &status) &&
                         fnDN.hasAttribute(attrName2, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (hasAttributes)
    {
        MObject pAttr1 = fnDN.attribute(attrName1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MObject pAttr2 = fnDN.attribute(attrName2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPlug attr1p(defaultSet, pAttr1);
        MPlug attr2p(defaultSet, pAttr2);

        MObject pd1 = attr1p.asMObject();
        MObject pd2 = attr2p.asMObject();

        MFnStringArrayData fnStrDataSourceAxisSet(pd1);
        MFnStringArrayData fnStrDataTargetAxisSet(pd2);

        if (fnStrDataSourceAxisSet.length() != fnStrDataTargetAxisSet.length())
        {
            MGlobal::displayError("Unequal number of axis pairings detected");
            return MS::kFailure;
        }

        int numPairings = fnStrDataSourceAxisSet.length();

        for (int i = 0; i < numPairings; i++)
        {
            MString sourceAxisName = fnStrDataSourceAxisSet[i];
            MString targetAxisName = fnStrDataTargetAxisSet[i];

            status = pairAxes(sourceAxisName, targetAxisName);

            if (status != MS::kSuccess)
            {
                MGlobal::displayError("Axis pairing failed - ignoring");
            }
        }
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::pluckTargetMesh()
{
    MStatus status;

    bool found = false;

    for (const auto &entry : m_global_geometry_map)
    {
        string meshNameChar = entry.first;

        MString meshName = meshNameChar.c_str();

        // Should only ever be one per scene
        if (meshName != SOURCE_HAND_NAME)
        {
            m_target_mesh_name = meshName;
            found = true;

            m_contact_colors[meshNameChar] = TARGET_CONTACT_COLOR;
            m_contact_sphere_names[meshNameChar] = TARGET_CONTACT_SPHERE_NAME;

            break;
        }
    }

    if (!found)
    {
        MGlobal::displayInfo("No target mesh found");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

// Visualization Utils

MStatus ContactTransferEditContext::createVisualizationSphere(
    MFnMesh &fnMesh, GeometryProcessingContext *gpc, MString &serializedPoint,
    MString name, float radius, MString shadingNodeName,
    MFnTransform &fnTransform)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    vector<int> vertexIndices;
    vector<double> coords;

    status = gpc->parseSerializedPoint(serializedPoint, vertexIndices, coords);
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

MStatus ContactTransferEditContext::visualizeAllAxes()
{
    MStatus status;

    // Iterate through meshes
    for (auto const &meshIt : m_global_axis_map)
    {
        string transformNameChar = meshIt.first;
        MString transformName = MString(transformNameChar.c_str());

        // Iterate through the mesh's axes
        for (auto const &axisNameChar : m_global_axis_map[transformNameChar])
        {
            MString axisName = MString(axisNameChar.c_str());

            status = visualizeAxis(transformName, axisName);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::visualizeAxis(MString &transformName,
                                                  MString &axisName)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    MStringArray serializedAxisPoints;
    status = getAxisAttribute(axisName, serializedAxisPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string transformNameChar = transformName.asChar();
    string axisNameChar = axisName.asChar();

    MDagPath meshDag = m_global_geometry_map[transformNameChar];
    GeometryProcessingContext *meshGPC =
        m_global_geometry_processing_context_map[transformNameChar];

    MFnMesh fnMesh(meshDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject axisGroup;

    // Create a visualization group for the axis if it does not already exist
    if (!m_axis_visualization_map.contains(axisNameChar))
    {
        // Create a visualization group for the axis
        MFnTransform fnTransform;
        axisGroup = fnTransform.create(MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString axisPatchName;
        status = getAxisPatchName(axisName, axisPatchName);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        fnTransform.setName(axisPatchName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString shadingNodeName =
            axisPatchName + AXIS_START_PREFIX + PATCH_SHADER_SUFFIX;

        memset(command, 0, sizeof(command));
        snprintf(command, COMMAND_BUFFER_SIZE,
                 "shadingNode -asShader lambert -name %s",
                 shadingNodeName.asChar());
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        snprintf(command, COMMAND_BUFFER_SIZE,
                 "setAttr %s.color -type double3 %f %f %f",
                 shadingNodeName.asChar(), AXIS_START_COLOR[0],
                 AXIS_START_COLOR[1], AXIS_START_COLOR[2]);
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Create the first sphere in the axis
        MString startSphereName = AXIS_START_PREFIX + axisPatchName;
        status = createVisualizationSphere(
            fnMesh, meshGPC, serializedAxisPoints[0], startSphereName,
            AXIS_START_SIZE, shadingNodeName, fnTransform);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Get a colored Lambertian shading node for the rest of the axis
        shadingNodeName = axisPatchName + PATCH_SHADER_SUFFIX;

        MColor axisColor = m_color_table[axisNameChar];

        snprintf(command, COMMAND_BUFFER_SIZE,
                 "shadingNode -asShader lambert -name %s",
                 shadingNodeName.asChar());
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        snprintf(command, COMMAND_BUFFER_SIZE,
                 "setAttr %s.color -type double3 %f %f %f",
                 shadingNodeName.asChar(), axisColor[0], axisColor[1],
                 axisColor[2]);
        status = MGlobal::executeCommand(command);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numAxisPoints = serializedAxisPoints.length();
        for (int i = 1; i < numAxisPoints; i++)
        {
            MString serializedAxisPoint = serializedAxisPoints[i];

            status = createVisualizationSphere(
                fnMesh, meshGPC, serializedAxisPoint, AXIS_SPHERE_NAME,
                DEFAULT_SPHERE_SIZE, shadingNodeName, fnTransform);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        m_axis_visualization_map[axisNameChar] = axisGroup;
    }

    // otherwise just update the locations and colors

    else
    {
        axisGroup = m_axis_visualization_map[axisNameChar];

        MFnDagNode fnGroupDagNode(axisGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numAxisPoints = serializedAxisPoints.length();
        int sphereCount = fnGroupDagNode.childCount();

        if (numAxisPoints != sphereCount)
        {
            MGlobal::displayInfo(
                "Error - axis sphere count does not match axis point count");
            return MS::kFailure;
        }

        vector<int> vertexIndices;
        vector<double> coords;

        MFloatPoint position;
        MFloatVector normal;

        for (int i = 0; i < numAxisPoints; i++)
        {
            vertexIndices.clear();
            coords.clear();

            MString serializedAxisPoint = serializedAxisPoints[i];

            MObject sphere = fnGroupDagNode.child(i, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnDagNode sphereDagNode(sphere, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MDagPath sphereDag;
            status = sphereDagNode.getPath(sphereDag);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnTransform fnSphereTransform(sphereDag, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = meshGPC->parseSerializedPoint(serializedAxisPoint,
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

MStatus
ContactTransferEditContext::visualizeContactGroup(MString &transformName,
                                                  MString &contactGroupName,
                                                  set<int> &ommissionIndices)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    MStringArray serializedContactPoints;
    status = getContactAttribute(contactGroupName, serializedContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string transformNameChar = transformName.asChar();
    string contactGroupNameChar = contactGroupName.asChar();

    MDagPath meshDag = m_global_geometry_map[transformNameChar];
    GeometryProcessingContext *meshGPC =
        m_global_geometry_processing_context_map[transformNameChar];

    MFnMesh fnMesh(meshDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Create a visualization group for the contact
    MFnTransform fnTransform;
    MObject contactGroup = fnTransform.create(MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString contactPatchName =
        CONTACT_PATCH_BASE_PREFIX + transformName + "Shape";

    fnTransform.setName(contactPatchName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Get a colored Lambertian shading node for the patch
    MString shadingNodeName = contactPatchName + PATCH_SHADER_SUFFIX;
    MColor contactColor = m_contact_colors[transformNameChar];

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

    MString sphereName = m_contact_sphere_names[transformNameChar];

    for (int i = 0; i < serializedContactPoints.length(); i++)
    {
        MString serializedContactPoint = serializedContactPoints[i];

        if (!ommissionIndices.contains(i))
        {
            status = createVisualizationSphere(
                fnMesh, meshGPC, serializedContactPoint, sphereName,
                DEFAULT_SPHERE_SIZE, shadingNodeName, fnTransform);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    m_contact_visualization_map[contactGroupNameChar] = contactGroup;

    return MS::kSuccess;
}

// Core Utilities

MStatus
ContactTransferEditContext::getAxisAttribute(MString &axisName,
                                             MStringArray &serializedAxisPoints)
{
    MStatus status;

    status = getSetSerializedPointsAttribute(axisName, AXIS_POINTS_ATTRIBUTE,
                                             serializedAxisPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getAxisPatchName(MString &axisName,
                                                     MString &axisPatchName)
{
    MStatus status;

    MString shapeName;
    status = getAxisShapeName(axisName, shapeName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    axisPatchName = AXIS_PATCH_BASE_PREFIX + shapeName;

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getContactSpreadScaleAttribute(
    MString &axisName, double &contactSpreadScale)
{
    MStatus status;

    status = getSetNumericScaleAttribute(
        axisName, CONTACT_SPREAD_SCALE_ATTRIBUTE, contactSpreadScale);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getAxisShapeName(MString &axisName,
                                                     MString &shapeName)
{
    MStatus status;

    string axisNameChar = axisName.asChar();

    string basePrefix = AXIS_BASE_PREFIX.asChar();
    int basePrefixPos = axisNameChar.find(basePrefix);

    axisNameChar.erase(basePrefixPos, basePrefix.length());

    shapeName = MString(axisNameChar.c_str());

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getContactAttribute(
    MString &contactName, MStringArray &serializedContactPoints)
{
    MStatus status;

    status = getSetSerializedPointsAttribute(
        contactName, CONTACT_POINTS_ATTRIBUTE, serializedContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getParameterizedContactDistanceAttribute(
    MString &contactGroupName, MDoubleArray &parameterizedContactDistances)
{
    MStatus status;

    MSelectionList selectionList;

    status = parameterizedContactDistances.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("No parameterized distances found for " +
                             contactGroupName);
        return MS::kSuccess;
    }

    MObject contactGroupObject;
    status = selectionList.getDependNode(0, contactGroupObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(contactGroupObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(
        CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!hasAttribute)
    {
        MGlobal::displayInfo(
            "WARNING: Parameterized contact distance attribute not found");
        return MS::kFailure;
    }

    MObject attrObj =
        fnDepNode.attribute(CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(contactGroupObject, attrObj);

    MObject attributeData = attributePlug.asMObject();

    MFnDoubleArrayData fnDoubleArrayData(attributeData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = parameterizedContactDistances.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < fnDoubleArrayData.length(); i++)
    {
        double item = fnDoubleArrayData[i];
        parameterizedContactDistances.append(item);
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getSetNumericScaleAttribute(
    MString &setName, MString attributeName, double &scaleValue)
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
        MGlobal::displayInfo("WARNING: Numeric attribute " + attributeName +
                             " not found");
        return MS::kFailure;
    }

    MObject attrObj = fnDepNode.attribute(attributeName, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(setObject, attrObj);

    scaleValue = attributePlug.asDouble(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::getSetSerializedPointsAttribute(
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

MStatus ContactTransferEditContext::interpolateSerializedPoint(
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

MStatus ContactTransferEditContext::loadAllFrameContacts()
{
    MStatus status;

    MSelectionList selectionList;

    MObject contactFrameGroupSet;
    MObject conntactSet;

    MString sourceHandContactGroup = CONTACT_GROUP_PREFIX + SOURCE_HAND_NAME +
                                     "Shape_" +
                                     MString(to_string(m_frame).c_str());

    MString targetHandFrameContactGroup = CONTACT_GROUP_PREFIX +
                                          m_target_mesh_name + "Shape_" +
                                          MString(to_string(m_frame).c_str());

    status = MGlobal::getSelectionListByName(targetHandFrameContactGroup,
                                             selectionList);

    set<int> ommissionIndices;

    if (status == MS::kSuccess)
    {
        MGlobal::displayInfo("Found target hand contacts");

        MDoubleArray frameParameterizedContactPointDistances;

        status = getParameterizedContactDistanceAttribute(
            targetHandFrameContactGroup,
            frameParameterizedContactPointDistances);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (int i = 0; i < frameParameterizedContactPointDistances.length();
             i++)
        {
            double distance = frameParameterizedContactPointDistances[i];

            if (distance > m_contact_parameterized_distance_threshold)
            {
                ommissionIndices.insert(i);
            }
        }

        MString targetTransformName = m_target_mesh_name;

        status = visualizeContactGroup(
            targetTransformName, targetHandFrameContactGroup, ommissionIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        MGlobal::displayInfo("No target hand contacts found");
    }

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        MGlobal::getSelectionListByName(sourceHandContactGroup, selectionList);

    if (status == MS::kSuccess)
    {
        MGlobal::displayInfo("Found source hand contacts");

        MString sourceTransformName = SOURCE_HAND_NAME;

        status = visualizeContactGroup(
            sourceTransformName, sourceHandContactGroup, ommissionIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        MGlobal::displayInfo("No source hand contacts found");
    }

    return MS::kSuccess;
}

MStatus
ContactTransferEditContext::loadAllSetGroupContacts(MFnSet &fnSet,
                                                    set<string> &storageBuffer)
{
    MStatus status;

    MSelectionList selectionList;

    MObject conntactSet;

    status = fnSet.getMembers(selectionList, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MItSelectionList itList(selectionList);
    for (; !itList.isDone(); itList.next())
    {
        status = itList.getDependNode(conntactSet);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnSet fnContactSet(conntactSet, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString contactName = fnContactSet.name(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        string contactNameChar = contactName.asChar();

        storageBuffer.insert(contactNameChar);
    }

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::pairAxes(MString &axisName1,
                                             MString &axisName2)
{
    MStatus status;

    MGlobal::displayInfo("Attempting to pair axes " + axisName1 + " and " +
                         axisName2 + "...");

    status = validateAxisExistence(axisName1);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = validateAxisExistence(axisName2);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedAxis1Points;
    status = getAxisAttribute(axisName1, serializedAxis1Points);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedAxis2Points;
    status = getAxisAttribute(axisName2, serializedAxis2Points);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (serializedAxis1Points.length() != serializedAxis2Points.length())
    {
        MGlobal::displayInfo("Contacts " + axisName1 + " and " + axisName2 +
                             " have unequal numbers of points. Unable to pair");
        return MS::kFailure;
    }

    string axisName1Char = axisName1.asChar();
    string axisName2Char = axisName2.asChar();

    m_paired_axes[axisName1Char] = axisName2;
    m_paired_axes[axisName2Char] = axisName1;

    MString pairingKey = axisName1 + "<->" + axisName2;
    MString pairingKeyAlt = axisName2 + "<->" + axisName1;

    string pairingKeyChar = pairingKey.asChar();
    string pairingKeyAltChar = pairingKeyAlt.asChar();

    // Do not double count if vertex pairing already exists in either form
    if (!m_paired_axes_unique_pairs.contains(pairingKeyChar) &&
        !m_paired_axes_unique_pairs.contains(pairingKeyAltChar))
    {
        MGlobal::displayInfo("Found unique pairing string: " + pairingKey);

        m_paired_axes_pair_keys[axisName1Char] = pairingKey;
        m_paired_axes_pair_keys[axisName2Char] = pairingKey;

        pair<MString, MString> pairedAxes = make_pair(axisName1, axisName2);

        m_paired_axes_unique_pairs[pairingKeyChar] = pairedAxes;

        MColor jointColor = m_color_table[axisName1Char];

        m_color_table[axisName1Char] = jointColor;
        m_color_table[axisName2Char] = jointColor;

        status = updateAxis(axisName1);
        CHECK_MSTATUS(status);

        status = updateAxis(axisName2);
        CHECK_MSTATUS(status);
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus
ContactTransferEditContext::setAxisAttribute(MString &axisName,
                                             MStringArray &serializedAxisPoints)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(axisName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject axisObject;
    status = selectionList.getDependNode(0, axisObject);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fnDepNode(axisObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute = fnDepNode.hasAttribute(AXIS_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(
            AXIS_POINTS_ATTRIBUTE, AXIS_POINTS_ATTRIBUTE, MFnData::kStringArray,
            MObject::kNullObj, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnDepNode.attribute(AXIS_POINTS_ATTRIBUTE, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(axisObject, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedAxisPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::setContactAttribute(
    MString &contactGroupName, MStringArray &serializedContactPoints)
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
        CONTACT_GROUP_PREFIX + MString(to_string(m_frame).c_str());
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

MStatus ContactTransferEditContext::setOmissionIndicesAttribute(
    MString &contactGroupName, MIntArray &omissionIndices)
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

MStatus ContactTransferEditContext::setParameterizedContactDistanceAttribute(
    MString &contactGroupName, MDoubleArray &parameterizedContactDistances)
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

    bool hasAttribute = fnDepNode.hasAttribute(
        CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE,
                                     CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE,
                                     MFnData::kDoubleArray, MObject::kNullObj,
                                     &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj = fnDepNode.attribute(CONTACT_PARAMETERIZED_DISTANCE_ATTRIBUTE,
                                      &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(contactGroupObject, attrObj);

    MFnDoubleArrayData fnDoubleArrayData;

    MObject doubleArrayData =
        fnDoubleArrayData.create(parameterizedContactDistances, &status);

    status = attributePlug.setMObject(doubleArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::updateAxis(MString &axisName)
{
    MStatus status;

    status = validateAxisExistence(axisName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string axisNameChar = axisName.asChar();

    MString transformName = m_global_axis_meshes[axisNameChar];

    string transformNameChar = transformName.asChar();

    GeometryProcessingContext *gpc =
        m_global_geometry_processing_context_map[transformNameChar];

    MStringArray serializedAxisPoints;
    status = gpc->getAxis(axisName, serializedAxisPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // update axis
    status = setAxisAttribute(axisName, serializedAxisPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // revisualize
    status = visualizeAxis(transformName, axisName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::validateAxisExistence(MString &axisName)
{
    MStatus status;

    string axisNameChar = axisName.asChar();

    if (!m_all_axes.contains(axisNameChar))
    {
        MGlobal::displayInfo("No axis with name " + axisName +
                             " found. Doing nothing.");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

// Core Context Teardown

MStatus ContactTransferEditContext::clearVisualizations()
{
    MStatus status;

    status = clearAxisVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = clearContactVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::clearAxisVisualizations()
{
    MStatus status;

    MString axisPatchRegex = AXIS_PATCH_BASE_PREFIX + MString("*");

    status = wipeAllRegexNodes(axisPatchRegex);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    m_axis_visualization_map.clear();

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::clearContactVisualizations()
{
    MStatus status;

    MString contactPatchRegex = CONTACT_PATCH_BASE_PREFIX + MString("*");

    status = wipeAllRegexNodes(contactPatchRegex);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    m_contact_visualization_map.clear();

    return MS::kSuccess;
}

MStatus ContactTransferEditContext::wipeAllRegexNodes(MString &wipeRegex)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(wipeRegex, selectionList);

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
