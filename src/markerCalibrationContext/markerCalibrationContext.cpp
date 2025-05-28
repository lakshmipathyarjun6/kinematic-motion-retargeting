#include "markerCalibrationContext.hpp"

// Basic Setup and Teardown

MarkerCalibrationContext::MarkerCalibrationContext()
{
    MString sourceMarkerFullPrefix =
        SOURCE_MARKER_BASE_PREFIX + SOURCE_HAND_NAME + "Shape";

    MString m1Partial = "PinkyTip";
    MString m1Full = sourceMarkerFullPrefix + m1Partial;
    string m1PartialChar = m1Partial.asChar();
    string m1FullChar = m1Full.asChar();

    MString m2Partial = "PinkyMiddle";
    MString m2Full = sourceMarkerFullPrefix + m2Partial;
    string m2PartialChar = m2Partial.asChar();
    string m2FullChar = m2Full.asChar();

    MString m3Partial = "PinkyBase";
    MString m3Full = sourceMarkerFullPrefix + m3Partial;
    string m3PartialChar = m3Partial.asChar();
    string m3FullChar = m3Full.asChar();

    MString m4Partial = "RingTip";
    MString m4Full = sourceMarkerFullPrefix + m4Partial;
    string m4PartialChar = m4Partial.asChar();
    string m4FullChar = m4Full.asChar();

    MString m5Partial = "RingMiddle";
    MString m5Full = sourceMarkerFullPrefix + m5Partial;
    string m5PartialChar = m5Partial.asChar();
    string m5FullChar = m5Full.asChar();

    MString m6Partial = "RingBase";
    MString m6Full = sourceMarkerFullPrefix + m6Partial;
    string m6PartialChar = m6Partial.asChar();
    string m6FullChar = m6Full.asChar();

    MString m7Partial = "MiddleTip";
    MString m7Full = sourceMarkerFullPrefix + m7Partial;
    string m7PartialChar = m7Partial.asChar();
    string m7FullChar = m7Full.asChar();

    MString m8Partial = "MiddleMiddle";
    MString m8Full = sourceMarkerFullPrefix + m8Partial;
    string m8PartialChar = m8Partial.asChar();
    string m8FullChar = m8Full.asChar();

    MString m9Partial = "MiddleBase";
    MString m9Full = sourceMarkerFullPrefix + m9Partial;
    string m9PartialChar = m9Partial.asChar();
    string m9FullChar = m9Full.asChar();

    MString m10Partial = "IndexTip";
    MString m10Full = sourceMarkerFullPrefix + m10Partial;
    string m10PartialChar = m10Partial.asChar();
    string m10FullChar = m10Full.asChar();

    MString m11Partial = "IndexMiddle";
    MString m11Full = sourceMarkerFullPrefix + m11Partial;
    string m11PartialChar = m11Partial.asChar();
    string m11FullChar = m11Full.asChar();

    MString m12Partial = "IndexBase";
    MString m12Full = sourceMarkerFullPrefix + m12Partial;
    string m12PartialChar = m12Partial.asChar();
    string m12FullChar = m12Full.asChar();

    MString m13Partial = "ThumbTip";
    MString m13Full = sourceMarkerFullPrefix + m13Partial;
    string m13PartialChar = m13Partial.asChar();
    string m13FullChar = m13Full.asChar();

    MString m14Partial = "ThumbMiddle";
    MString m14Full = sourceMarkerFullPrefix + m14Partial;
    string m14PartialChar = m14Partial.asChar();
    string m14FullChar = m14Full.asChar();

    MString m15Partial = "ThumbBase";
    MString m15Full = sourceMarkerFullPrefix + m15Partial;
    string m15PartialChar = m15Partial.asChar();
    string m15FullChar = m15Full.asChar();

    MString m16Partial = "BackRing";
    MString m16Full = sourceMarkerFullPrefix + m16Partial;
    string m16PartialChar = m16Partial.asChar();
    string m16FullChar = m16Full.asChar();

    MString m17Partial = "BackIndex";
    MString m17Full = sourceMarkerFullPrefix + m17Partial;
    string m17PartialChar = m17Partial.asChar();
    string m17FullChar = m17Full.asChar();

    MString m18Partial = "WristRight";
    MString m18Full = sourceMarkerFullPrefix + m18Partial;
    string m18PartialChar = m18Partial.asChar();
    string m18FullChar = m18Full.asChar();

    MString m19Partial = "WristLeft";
    MString m19Full = sourceMarkerFullPrefix + m19Partial;
    string m19PartialChar = m19Partial.asChar();
    string m19FullChar = m19Full.asChar();

    MString m20Partial = "PinkyFingertip";
    MString m20Full = sourceMarkerFullPrefix + m20Partial;
    string m20PartialChar = m20Partial.asChar();
    string m20FullChar = m20Full.asChar();

    MString m21Partial = "RingFingertip";
    MString m21Full = sourceMarkerFullPrefix + m21Partial;
    string m21PartialChar = m21Partial.asChar();
    string m21FullChar = m21Full.asChar();

    MString m22Partial = "MiddleFingertip";
    MString m22Full = sourceMarkerFullPrefix + m22Partial;
    string m22PartialChar = m22Partial.asChar();
    string m22FullChar = m22Full.asChar();

    MString m23Partial = "IndexFingertip";
    MString m23Full = sourceMarkerFullPrefix + m23Partial;
    string m23PartialChar = m23Partial.asChar();
    string m23FullChar = m23Full.asChar();

    MString m24Partial = "ThumbFingertip";
    MString m24Full = sourceMarkerFullPrefix + m24Partial;
    string m24PartialChar = m24Partial.asChar();
    string m24FullChar = m24Full.asChar();

    MString m25Partial = "PalmRoot";
    MString m25Full = sourceMarkerFullPrefix + m25Partial;
    string m25PartialChar = m25Partial.asChar();
    string m25FullChar = m25Full.asChar();

    m_source_marker_names.insert(m1FullChar);
    m_source_marker_names.insert(m2FullChar);
    m_source_marker_names.insert(m3FullChar);
    m_source_marker_names.insert(m4FullChar);
    m_source_marker_names.insert(m5FullChar);
    m_source_marker_names.insert(m6FullChar);
    m_source_marker_names.insert(m7FullChar);
    m_source_marker_names.insert(m8FullChar);
    m_source_marker_names.insert(m9FullChar);
    m_source_marker_names.insert(m10FullChar);
    m_source_marker_names.insert(m11FullChar);
    m_source_marker_names.insert(m12FullChar);
    m_source_marker_names.insert(m13FullChar);
    m_source_marker_names.insert(m14FullChar);
    m_source_marker_names.insert(m15FullChar);
    m_source_marker_names.insert(m16FullChar);
    m_source_marker_names.insert(m17FullChar);
    m_source_marker_names.insert(m18FullChar);
    m_source_marker_names.insert(m19FullChar);
    m_source_marker_names.insert(m20FullChar);
    m_source_marker_names.insert(m21FullChar);
    m_source_marker_names.insert(m22FullChar);
    m_source_marker_names.insert(m23FullChar);
    m_source_marker_names.insert(m24FullChar);
    m_source_marker_names.insert(m25FullChar);

    m_source_marker_vertex_mappings[650] = m1Full;
    m_source_marker_vertex_mappings[633] = m2Full;
    m_source_marker_vertex_mappings[616] = m3Full;
    m_source_marker_vertex_mappings[533] = m4Full;
    m_source_marker_vertex_mappings[476] = m5Full;
    m_source_marker_vertex_mappings[291] = m6Full;
    m_source_marker_vertex_mappings[423] = m7Full;
    m_source_marker_vertex_mappings[405] = m8Full;
    m_source_marker_vertex_mappings[399] = m9Full;
    m_source_marker_vertex_mappings[316] = m10Full;
    m_source_marker_vertex_mappings[86] = m11Full;
    m_source_marker_vertex_mappings[274] = m12Full;
    m_source_marker_vertex_mappings[731] = m13Full;
    m_source_marker_vertex_mappings[250] = m14Full;
    m_source_marker_vertex_mappings[89] = m15Full;
    m_source_marker_vertex_mappings[181] = m16Full;
    m_source_marker_vertex_mappings[158] = m17Full;
    m_source_marker_vertex_mappings[79] = m18Full;
    m_source_marker_vertex_mappings[92] = m19Full;
    m_source_marker_vertex_mappings[671] = m20Full;
    m_source_marker_vertex_mappings[554] = m21Full;
    m_source_marker_vertex_mappings[443] = m22Full;
    m_source_marker_vertex_mappings[320] = m23Full;
    m_source_marker_vertex_mappings[744] = m24Full;
    m_source_marker_vertex_mappings[24] = m25Full;

    m_source_marker_partial_lookup_table[m1PartialChar] = m1Full;
    m_source_marker_partial_lookup_table[m2PartialChar] = m2Full;
    m_source_marker_partial_lookup_table[m3PartialChar] = m3Full;
    m_source_marker_partial_lookup_table[m4PartialChar] = m4Full;
    m_source_marker_partial_lookup_table[m5PartialChar] = m5Full;
    m_source_marker_partial_lookup_table[m6PartialChar] = m6Full;
    m_source_marker_partial_lookup_table[m7PartialChar] = m7Full;
    m_source_marker_partial_lookup_table[m8PartialChar] = m8Full;
    m_source_marker_partial_lookup_table[m9PartialChar] = m9Full;
    m_source_marker_partial_lookup_table[m10PartialChar] = m10Full;
    m_source_marker_partial_lookup_table[m11PartialChar] = m11Full;
    m_source_marker_partial_lookup_table[m12PartialChar] = m12Full;
    m_source_marker_partial_lookup_table[m13PartialChar] = m13Full;
    m_source_marker_partial_lookup_table[m14PartialChar] = m14Full;
    m_source_marker_partial_lookup_table[m15PartialChar] = m15Full;
    m_source_marker_partial_lookup_table[m16PartialChar] = m16Full;
    m_source_marker_partial_lookup_table[m17PartialChar] = m17Full;
    m_source_marker_partial_lookup_table[m18PartialChar] = m18Full;
    m_source_marker_partial_lookup_table[m19PartialChar] = m19Full;
    m_source_marker_partial_lookup_table[m20PartialChar] = m20Full;
    m_source_marker_partial_lookup_table[m21PartialChar] = m21Full;
    m_source_marker_partial_lookup_table[m22PartialChar] = m22Full;
    m_source_marker_partial_lookup_table[m23PartialChar] = m23Full;
    m_source_marker_partial_lookup_table[m24PartialChar] = m24Full;
    m_source_marker_partial_lookup_table[m25PartialChar] = m25Full;

    MColor tmc0 = MColor(0.922, 0.318, 0.318);
    MColor tmc1 = MColor(0.318, 0.561, 0.922);
    MColor tmc2 = MColor(0.318, 0.827, 0.922);
    MColor tmc3 = MColor(0.318, 0.922, 0.388);
    MColor tmc4 = MColor(0.737, 0.980, 0.314);
    MColor tmc5 = MColor(0.549, 0.314, 0.980);
    MColor tmc6 = MColor(0.980, 0.314, 0.8);
    MColor tmc7 = MColor(0.784, 0.314, 0.980);
    MColor tmc8 = MColor(0.980, 0.973, 0.314);
    MColor tmc9 = MColor(1.0, 0.761, 0.345);

    m_target_marker_color_pallette.push_back(tmc0);
    m_target_marker_color_pallette.push_back(tmc1);
    m_target_marker_color_pallette.push_back(tmc2);
    m_target_marker_color_pallette.push_back(tmc3);
    m_target_marker_color_pallette.push_back(tmc4);
    m_target_marker_color_pallette.push_back(tmc5);
    m_target_marker_color_pallette.push_back(tmc6);
    m_target_marker_color_pallette.push_back(tmc7);
    m_target_marker_color_pallette.push_back(tmc8);
    m_target_marker_color_pallette.push_back(tmc9);
}

MarkerCalibrationContext::~MarkerCalibrationContext() {}

void *MarkerCalibrationContext::creator()
{
    return new MarkerCalibrationContext();
}

void MarkerCalibrationContext::toolOnSetup(MEvent &event)
{
    MStatus status;

    MGlobal::displayInfo("Setting up marker calibration context...");

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(SOURCE_HAND_NAME, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("Source hand MUST have the name " +
                             SOURCE_HAND_NAME);
        return;
    }

    m_view = M3dView::active3dView();

    m_target_marker_color_table.clear();
    m_color_pallette_index = 0;

    status = loadAllGeometries();
    CHECK_MSTATUS(status);

    status = pluckTargetMesh();
    CHECK_MSTATUS(status);

    status = loadTargetVirtualMarkers();
    CHECK_MSTATUS(status);

    status = visualizeSourceMarkers();
    CHECK_MSTATUS(status);

    status = visualizeAllTargetMarkers();
    CHECK_MSTATUS(status);

    status = pairAllMarkers();
    CHECK_MSTATUS(status);

    status = redrawPairingLines();
    CHECK_MSTATUS(status);

    MGlobal::clearSelectionList();

    MGlobal::displayInfo("Done");
}

void MarkerCalibrationContext::toolOffCleanup()
{
    MStatus status;

    MGlobal::displayInfo("Tearing down marker calibration context...");

    status = clearTargetMarkerVisualizations();
    CHECK_MSTATUS(status);

    status = wipePairingLines();
    CHECK_MSTATUS(status);

    m_paired_markers.clear();
    m_target_marker_names.clear();

    MGlobal::displayInfo("Done");
}

// Core Context Setup

MStatus MarkerCalibrationContext::loadAllGeometries()
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

MStatus MarkerCalibrationContext::loadTargetVirtualMarkers()
{
    MStatus status;

    MString markerPatchRegex = TARGET_MARKER_BASE_PREFIX + MString("*");

    MSelectionList markerPatchSelectionList;
    status = MGlobal::getSelectionListByName(markerPatchRegex,
                                             markerPatchSelectionList);

    string targetMeshNameChar = m_target_mesh_name.asChar();

    if (status == MStatus::kSuccess)
    {
        MItSelectionList markerPatchItList(markerPatchSelectionList);

        for (; !markerPatchItList.isDone(); markerPatchItList.next())
        {
            MObject markerPatchSet;
            status = markerPatchItList.getDependNode(markerPatchSet);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            if (markerPatchSet.hasFn(MFn::kSet))
            {
                MFnSet fnSet(markerPatchSet, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MString targetMarkerName = fnSet.name(&status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = validateTargetMarkerNamingConvention(targetMarkerName);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                cout << "Found target marker " << targetMarkerName << endl;

                MGlobal::displayInfo("Found target marker " + targetMarkerName);

                string targetMarkerNameChar = targetMarkerName.asChar();

                // Ignore shader sets
                string patchShaderSuffixChar =
                    TARGET_MARKER_PATCH_SHADER_SUFFIX.asChar();

                if (targetMarkerNameChar.find(patchShaderSuffixChar) !=
                    string::npos)
                {
                    MGlobal::displayInfo("Skipping shader set " +
                                         targetMarkerName);
                    continue;
                }

                // Assign color
                m_target_marker_color_table[targetMarkerNameChar] =
                    m_target_marker_color_pallette[m_color_pallette_index];
                m_color_pallette_index = (m_color_pallette_index + 1) %
                                         m_target_marker_color_pallette.size();

                MStringArray serializedMarkerPoints;

                status = getTargetMarkerPointsAttribute(targetMarkerName,
                                                        serializedMarkerPoints);

                if (status != MS::kSuccess)
                {
                    status = digestNewTargetMarker(targetMarkerName);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                }

                m_target_marker_names.insert(targetMarkerNameChar);
            }
        }
    }
    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::pairAllMarkers()
{
    MStatus status;

    for (const auto &markerNameChar : m_target_marker_names)
    {
        MString markerName = markerNameChar.c_str();

        MString partialMarkerName;
        status = getTargetMarkerPartialName(markerName, partialMarkerName);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        string partialMarkerNameChar = partialMarkerName.asChar();

        if (m_source_marker_partial_lookup_table.contains(
                partialMarkerNameChar))
        {
            m_paired_markers[markerNameChar] =
                m_source_marker_partial_lookup_table[partialMarkerNameChar];
        }
    }

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::pluckTargetMesh()
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

MStatus MarkerCalibrationContext::createVisualizationSphere(
    MFnMesh &fnMesh, MString &serializedPoint, MString name, float radius,
    MString shadingNodeName, MFnTransform &fnTransform,
    MSelectionList &selectionList)
{
    MStatus status;

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

MStatus MarkerCalibrationContext::visualizeAllTargetMarkers()
{
    MStatus status;

    for (const auto &targetMarkerNameChar : m_target_marker_names)
    {
        MString targetMarkerName = targetMarkerNameChar.c_str();

        status = visualizeTargetMarker(targetMarkerName);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::visualizeSourceMarkers()
{
    MStatus status;

    MSelectionList selectionList;

    string sourceHandShapeNameChar = SOURCE_HAND_NAME.asChar();

    MDagPath sourceGeometry = m_global_geometry_map[sourceHandShapeNameChar];

    MFnMesh fnSourceHandMesh(sourceGeometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    selectionList.clear();

    for (const auto &markerNameChar : m_source_marker_names)
    {
        MString markerName = markerNameChar.c_str();

        selectionList.clear();

        status = MGlobal::getSelectionListByName(markerName, selectionList);

        if (status != MS::kSuccess)
        {
            selectionList.clear();

            MString virtualMocapMarkerCreateCommand =
                "spaceLocator -n " + markerName + " -p 0 0 0";
            status = MGlobal::executeCommand(virtualMocapMarkerCreateCommand);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = MGlobal::getSelectionListByName(markerName, selectionList);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        MDagPath mmdp;
        status = selectionList.getDagPath(0, mmdp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_source_markers[markerNameChar] = mmdp;
    }

    int numSourceHandVertices = fnSourceHandMesh.numVertices(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPoint vPos;

    for (int vIndex = 0; vIndex < numSourceHandVertices; vIndex++)
    {
        if (m_source_marker_vertex_mappings.contains(vIndex))
        {
            status = fnSourceHandMesh.getPoint(vIndex, vPos, MSpace::kWorld);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MString mmName = m_source_marker_vertex_mappings[vIndex];

            string mmNameChar = mmName.asChar();

            MDagPath locatorDag = m_source_markers[mmNameChar];

            MVector tfTrans(vPos[0], vPos[1], vPos[2]);

            MFnTransform fnTransform(locatorDag, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = fnTransform.setTranslation(tfTrans, MSpace::kWorld);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

MStatus
MarkerCalibrationContext::visualizeTargetMarker(MString &targetMarkerName)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

    string targetMarkerNameChar = targetMarkerName.asChar();
    string targetMeshNameChar = m_target_mesh_name.asChar();

    MStringArray serializedMarkerPoints;
    status = getTargetMarkerPointsAttribute(targetMarkerName,
                                            serializedMarkerPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath meshDag = m_global_geometry_map[targetMeshNameChar];

    MFnMesh fnMesh(meshDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject targetMarkerGroup = fnTransform.create(MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString targetMarkerPatchName;
    status = getTargetMarkerPatchName(targetMarkerName, targetMarkerPatchName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string targetMarkerPatchNameChar = targetMarkerPatchName.asChar();

    fnTransform.setName(targetMarkerPatchName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Get a colored Lambertian shading node for the patch
    MString shadingNodeName =
        targetMarkerPatchName + TARGET_MARKER_PATCH_SHADER_SUFFIX;
    MColor targetMarkerColor =
        m_target_marker_color_table[targetMarkerNameChar];

    snprintf(command, COMMAND_BUFFER_SIZE,
             "shadingNode -asShader lambert -name %s",
             shadingNodeName.asChar());
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    snprintf(command, COMMAND_BUFFER_SIZE,
             "setAttr %s.color -type double3 %f %f %f",
             shadingNodeName.asChar(), targetMarkerColor[0],
             targetMarkerColor[1], targetMarkerColor[2]);
    status = MGlobal::executeCommand(command);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numMarkerPoints = serializedMarkerPoints.length();
    for (int i = 1; i < numMarkerPoints; i++)
    {
        MString serializedMarkerPoint = serializedMarkerPoints[i];

        status = createVisualizationSphere(
            fnMesh, serializedMarkerPoint, TARGET_MARKER_SPHERE_NAME,
            DEFAULT_SPHERE_SIZE, shadingNodeName, fnTransform, selectionList);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

// Core Utils

MStatus
MarkerCalibrationContext::digestNewTargetMarker(MString &targetMarkerName)
{
    MStatus status;

    char vBuff[COMMAND_BUFFER_SIZE];

    // Find the marker
    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(targetMarkerName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject markerPointSet;
    status = selectionList.getDependNode(0, markerPointSet);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnSet(markerPointSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnSet.getMembers(selectionList, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath dp;
    MObject components;
    status = selectionList.getDagPath(0, dp, components);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MItMeshVertex vertexItList(dp, components, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedMarkerPoints;

    for (; !vertexItList.isDone(); vertexItList.next())
    {
        int index = vertexItList.index(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        memset(vBuff, 0, sizeof(vBuff));
        snprintf(vBuff, sizeof(vBuff), "v %d", index);

        serializedMarkerPoints.append(vBuff);
    }

    status = setTargetMarkerPointsAttribute(targetMarkerName,
                                            serializedMarkerPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnSet.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::getSetSerializedPointsAttribute(
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

        status = serializedPoints.append(item);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::getTargetMarkerPointsAttribute(
    MString &targetMarkerName, MStringArray &serializedMarkerPoints)
{
    MStatus status;

    status = getSetSerializedPointsAttribute(
        targetMarkerName, MARKER_POINTS_ATTRIBUTE, serializedMarkerPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::getTargetMarkerPartialName(
    MString &fullTargetMarkerName, MString &partialName)
{
    MStatus status;

    string fullTargetMarkerNameChar = fullTargetMarkerName.asChar();

    MString basePrefix =
        TARGET_MARKER_BASE_PREFIX + m_target_mesh_name + "Shape";

    string basePrefixChar = basePrefix.asChar();
    int basePrefixPos = fullTargetMarkerNameChar.find(basePrefixChar);

    fullTargetMarkerNameChar.erase(basePrefixPos, basePrefixChar.length());

    partialName = MString(fullTargetMarkerNameChar.c_str());

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::getTargetMarkerPatchName(
    MString &targetMarkerName, MString &targetMarkerPatchName)
{
    MStatus status;

    MString partialName;
    status = getTargetMarkerPartialName(targetMarkerName, partialName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    targetMarkerPatchName = TARGET_MARKER_PATCH_BASE_PREFIX + partialName;

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::interpolateSerializedPoint(
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

MStatus MarkerCalibrationContext::parseSerializedPoint(MFnMesh &fnMesh,
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

MStatus MarkerCalibrationContext::redrawPairingLines()
{
    MStatus status;

    status = wipePairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string targetMeshNameChar = m_target_mesh_name.asChar();

    MDagPath targetMeshDag = m_global_geometry_map[targetMeshNameChar];

    MFnMesh fnTargetMesh(targetMeshDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTransform;
    MObject lineGroup = fnTransform.create();

    MFnTransform fnLineTransform(lineGroup, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnLineTransform.setName(MARKER_PAIRING_LINES_PREFIX);

    vector<int> vertexIndices;
    vector<double> coords;

    MFloatPoint targetPointPosition;
    MFloatVector targetPointNormal;

    for (const auto &entry : m_paired_markers)
    {
        string targetMarkerNameChar = entry.first;

        MString targetMarkerName = targetMarkerNameChar.c_str();
        MString sourceMarkerName = entry.second;

        string sourceMarkerNameChar = sourceMarkerName.asChar();

        MDagPath sourceMarkerDag = m_source_markers[sourceMarkerNameChar];

        MFnTransform sourceMarkerTransform(sourceMarkerDag, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector sourceMarkerPos =
            sourceMarkerTransform.getTranslation(MSpace::kWorld, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MPoint sourcePointPosition(sourceMarkerPos);

        MStringArray serializedTargetMarkerPoints;
        status = getTargetMarkerPointsAttribute(targetMarkerName,
                                                serializedTargetMarkerPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numPoints = serializedTargetMarkerPoints.length();

        for (int i = 0; i < numPoints; i++)
        {
            vertexIndices.clear();
            coords.clear();

            MString serializedTargetMarkerPoint =
                serializedTargetMarkerPoints[i];

            status =
                parseSerializedPoint(fnTargetMesh, serializedTargetMarkerPoint,
                                     vertexIndices, coords);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = interpolateSerializedPoint(fnTargetMesh, vertexIndices,
                                                coords, targetPointPosition,
                                                targetPointNormal);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MPointArray pairedPoints;
            pairedPoints.append(sourcePointPosition);
            pairedPoints.append(targetPointPosition);

            MFnNurbsCurve fnCurve;
            MObject curve = fnCurve.createWithEditPoints(
                pairedPoints, 1, MFnNurbsCurve::kOpen, false, true, true,
                lineGroup, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::setSetSerializedPointsAttribute(
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

MStatus MarkerCalibrationContext::setTargetMarkerPointsAttribute(
    MString &targetMarkerName, MStringArray &serializedPoints)
{
    MStatus status;

    status = setSetSerializedPointsAttribute(
        targetMarkerName, MARKER_POINTS_ATTRIBUTE, serializedPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::validateTargetMarkerNamingConvention(
    MString &targetMarkerName)
{
    MStatus status;

    string targetMarkerNameChar = targetMarkerName.asChar();

    MString basePrefix =
        TARGET_MARKER_BASE_PREFIX + m_target_mesh_name + "Shape";

    string basePrefixChar = basePrefix.asChar();
    int basePrefixPos = targetMarkerNameChar.find(basePrefixChar);

    if (basePrefixPos == string::npos)
    {
        MGlobal::displayInfo("Invalid marker naming convention " +
                             targetMarkerName);
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus MarkerCalibrationContext::wipePairingLines()
{
    MStatus status;

    MSelectionList selectionList;
    MString linesRegex = MARKER_PAIRING_LINES_PREFIX + MString("*");
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

MStatus MarkerCalibrationContext::clearTargetMarkerVisualizations()
{
    MStatus status;

    MString targerMarkerPatchRegex =
        TARGET_MARKER_PATCH_BASE_PREFIX + MString("*");

    MSelectionList selectionList;

    status =
        MGlobal::getSelectionListByName(targerMarkerPatchRegex, selectionList);

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
