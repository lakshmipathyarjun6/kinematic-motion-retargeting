#include "GRABMotionSequenceIO.hpp"

GRABMotionSequenceIO::GRABMotionSequenceIO()
{
    mJointNames[0] = "wrist";
    mJointNames[1] = "index1";
    mJointNames[2] = "index2";
    mJointNames[3] = "index3";
    mJointNames[4] = "middle1";
    mJointNames[5] = "middle2";
    mJointNames[6] = "middle3";
    mJointNames[7] = "pinky1";
    mJointNames[8] = "pinky2";
    mJointNames[9] = "pinky3";
    mJointNames[10] = "ring1";
    mJointNames[11] = "ring2";
    mJointNames[12] = "ring3";
    mJointNames[13] = "thumb1";
    mJointNames[14] = "thumb2";
    mJointNames[15] = "thumb3";

    mVirtualMocapMarkerNames[0] = "mmhandShapePinkyTip";
    mVirtualMocapMarkerNames[1] = "mmhandShapePinkyMiddle";
    mVirtualMocapMarkerNames[2] = "mmhandShapePinkyBase";
    mVirtualMocapMarkerNames[3] = "mmhandShapeRingTip";
    mVirtualMocapMarkerNames[4] = "mmhandShapeRingMiddle";
    mVirtualMocapMarkerNames[5] = "mmhandShapeRingBase";
    mVirtualMocapMarkerNames[6] = "mmhandShapeMiddleTip";
    mVirtualMocapMarkerNames[7] = "mmhandShapeMiddleMiddle";
    mVirtualMocapMarkerNames[8] = "mmhandShapeMiddleBase";
    mVirtualMocapMarkerNames[9] = "mmhandShapeIndexTip";
    mVirtualMocapMarkerNames[10] = "mmhandShapeIndexMiddle";
    mVirtualMocapMarkerNames[11] = "mmhandShapeIndexBase";
    mVirtualMocapMarkerNames[12] = "mmhandShapeThumbTip";
    mVirtualMocapMarkerNames[13] = "mmhandShapeThumbMiddle";
    mVirtualMocapMarkerNames[14] = "mmhandShapeThumbBase";
    mVirtualMocapMarkerNames[15] = "mmhandShapeBackRing";
    mVirtualMocapMarkerNames[16] = "mmhandShapeBackIndex";
    mVirtualMocapMarkerNames[17] = "mmhandShapeWristRight";
    mVirtualMocapMarkerNames[18] = "mmhandShapeWristLeft";
    mVirtualMocapMarkerNames[19] = "mmhandShapePinkyFingertip";
    mVirtualMocapMarkerNames[20] = "mmhandShapeRingFingertip";
    mVirtualMocapMarkerNames[21] = "mmhandShapeMiddleFingertip";
    mVirtualMocapMarkerNames[22] = "mmhandShapeIndexFingertip";
    mVirtualMocapMarkerNames[23] = "mmhandShapeThumbFingertip";
    mVirtualMocapMarkerNames[24] = "mmhandShapePalmRoot";

    mVirtualMarkerVertexMappings[650] = "mmhandShapePinkyTip";
    mVirtualMarkerVertexMappings[633] = "mmhandShapePinkyMiddle";
    mVirtualMarkerVertexMappings[616] = "mmhandShapePinkyBase";
    mVirtualMarkerVertexMappings[533] = "mmhandShapeRingTip";
    mVirtualMarkerVertexMappings[476] = "mmhandShapeRingMiddle";
    mVirtualMarkerVertexMappings[291] = "mmhandShapeRingBase";
    mVirtualMarkerVertexMappings[423] = "mmhandShapeMiddleTip";
    mVirtualMarkerVertexMappings[405] = "mmhandShapeMiddleMiddle";
    mVirtualMarkerVertexMappings[399] = "mmhandShapeMiddleBase";
    mVirtualMarkerVertexMappings[316] = "mmhandShapeIndexTip";
    mVirtualMarkerVertexMappings[86] = "mmhandShapeIndexMiddle";
    mVirtualMarkerVertexMappings[274] = "mmhandShapeIndexBase";
    mVirtualMarkerVertexMappings[731] = "mmhandShapeThumbTip";
    mVirtualMarkerVertexMappings[250] = "mmhandShapeThumbMiddle";
    mVirtualMarkerVertexMappings[89] = "mmhandShapeThumbBase";
    mVirtualMarkerVertexMappings[181] = "mmhandShapeBackRing";
    mVirtualMarkerVertexMappings[158] = "mmhandShapeBackIndex";
    mVirtualMarkerVertexMappings[79] = "mmhandShapeWristRight";
    mVirtualMarkerVertexMappings[92] = "mmhandShapeWristLeft";
    mVirtualMarkerVertexMappings[671] = "mmhandShapePinkyFingertip";
    mVirtualMarkerVertexMappings[554] = "mmhandShapeRingFingertip";
    mVirtualMarkerVertexMappings[443] = "mmhandShapeMiddleFingertip";
    mVirtualMarkerVertexMappings[320] = "mmhandShapeIndexFingertip";
    mVirtualMarkerVertexMappings[744] = "mmhandShapeThumbFingertip";
    mVirtualMarkerVertexMappings[24] = "mmhandShapePalmRoot";
}

GRABMotionSequenceIO::~GRABMotionSequenceIO() {}

void *GRABMotionSequenceIO::creator() { return new GRABMotionSequenceIO(); }

bool GRABMotionSequenceIO::canBeOpened() const { return true; }

MString GRABMotionSequenceIO::defaultExtension() const { return "npz"; }

MPxFileTranslator::MFileKind
GRABMotionSequenceIO::identifyFile(const MFileObject &fileName,
                                   const char *buffer, short size) const
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

bool GRABMotionSequenceIO::haveReadMethod() const { return true; }

bool GRABMotionSequenceIO::haveWriteMethod() const { return false; }

MStatus GRABMotionSequenceIO::reader(const MFileObject &file,
                                     const MString &optionString,
                                     FileAccessMode mode)
{
    MStatus status;

    MString resolvedPath = file.resolvedPath();
    MString resolvedName = file.resolvedName();

    MString resolvedFullName = resolvedPath + resolvedName;

    string fileNameChar = resolvedFullName.asChar();

    status = readImport(fileNameChar);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus GRABMotionSequenceIO::readImport(string &fileName)
{
    MStatus status;

    // Create the IK joints
    MFnIkJoint fnJointGenerator;

    // Import GRABMotion
    MSelectionList selectionList;

    MString handShapeName = HAND_NAME + "Shape";
    MString objectShapeName = OBJECT_NAME + "Shape";
    MString tableShapeName = TABLE_NAME + "Shape";

    status = MGlobal::getSelectionListByName(handShapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath handMesh;
    status = selectionList.getDagPath(0, handMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnHandMesh(handMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    selectionList.clear();

    status = MGlobal::getSelectionListByName(objectShapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath objectMesh;
    status = selectionList.getDagPath(0, objectMesh);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnObjectMesh(objectMesh, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    selectionList.clear();

    status = MGlobal::getSelectionListByName(OBJECT_NAME, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath objectMeshTransform;
    status = selectionList.getDagPath(0, objectMeshTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnObjectTransform(objectMeshTransform, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    selectionList.clear();

    status = MGlobal::getSelectionListByName(TABLE_NAME, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath tableMeshTransform;
    status = selectionList.getDagPath(0, tableMeshTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTableTransform(tableMeshTransform, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    cnpy::npz_t dataNpz = cnpy::npz_load(fileName);

    cnpy::NpyArray numFramesNpy = dataNpz["numFrames"];
    int *numFramesData = numFramesNpy.data<int>();
    int numFrames = numFramesData[0];

    cnpy::NpyArray handJointHierarchyNpy = dataNpz["handJointHierarchy"];
    uint8_t *handJointHierarchyData = handJointHierarchyNpy.data<uint8_t>();

    double objectScale[3] = {SCALE_MULT, SCALE_MULT, SCALE_MULT};
    double tableScale[3] = {SCALE_MULT, SCALE_MULT, SCALE_MULT};

    for (int i = 0; i < NUM_HAND_JOINTS; i++)
    {
        selectionList.clear();

        MString jointName = mJointNames[i].c_str();

        status = MGlobal::getSelectionListByName(jointName, selectionList);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath jdp;
        status = selectionList.getDagPath(0, jdp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        mJoints[jointName.asChar()] = jdp;
    }

    for (int i = 0; i < NUM_MOCAP_MARKERS; i++)
    {
        string vmName = mVirtualMocapMarkerNames[i];

        MString markerName = vmName.c_str();

        MString mocapMarkerCreate =
            "spaceLocator -n " + markerName + " -p 0 0 0";
        status = MGlobal::executeCommand(mocapMarkerCreate);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        selectionList.clear();

        status = MGlobal::getSelectionListByName(markerName, selectionList);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath mmdp;
        status = selectionList.getDagPath(0, mmdp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        mVirtualMocapMarkers[vmName] = mmdp;
    }

    selectionList.clear();

    int numHandVertices = fnHandMesh.numVertices(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numObjectVertices = fnObjectMesh.numVertices(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    cnpy::NpyArray handVerticesNpy = dataNpz["handVertices"];
    float *handVerticesData = handVerticesNpy.data<float>();

    cnpy::NpyArray handRootOrientationsNpy = dataNpz["handRootOrientations"];
    float *handRootOrientationsData = handRootOrientationsNpy.data<float>();

    cnpy::NpyArray handRootTranslationsNpy = dataNpz["handRootTranslations"];
    float *handRootTranslationsData = handRootTranslationsNpy.data<float>();

    cnpy::NpyArray handJointOrientationsNpy = dataNpz["handJointOrientations"];
    float *handJointOrientationsData = handJointOrientationsNpy.data<float>();

    cnpy::NpyArray handJointsRestConfigurationNpy =
        dataNpz["handJointsRestConfiguration"];
    float *handJointsRestConfigurationData =
        handJointsRestConfigurationNpy.data<float>();

    cnpy::NpyArray objectRotationsNpy = dataNpz["objectRotations"];
    float *objectRotationsData = objectRotationsNpy.data<float>();

    cnpy::NpyArray objectTranslationsNpy = dataNpz["objectTranslations"];
    float *objectTranslationsData = objectTranslationsNpy.data<float>();

    cnpy::NpyArray tableRotationsNpy = dataNpz["tableRotations"];
    float *tableRotationsData = tableRotationsNpy.data<float>();

    cnpy::NpyArray tableTranslationsNpy = dataNpz["tableTranslations"];
    float *tableTranslationsData = tableTranslationsNpy.data<float>();

    cnpy::NpyArray contactFramesNpy = dataNpz["contactFrames"];
    uint16_t *contactFramesData = contactFramesNpy.data<uint16_t>();

    cnpy::NpyArray objectContactFrameCountsNpy =
        dataNpz["objectContactFrameCounts"];
    uint16_t *objectContactFrameCountsData =
        objectContactFrameCountsNpy.data<uint16_t>();

    cnpy::NpyArray objectContactLocationsNpy =
        dataNpz["objectContactLocations"];
    uint32_t *objectContactLocationsData =
        objectContactLocationsNpy.data<uint32_t>();

    MAnimControl animCtrl;

    MPointArray handVertexPoints;

    vector<float> handVertices;
    vector<float> handJointsAtRest;
    vector<float> handRootFrameOrientations;
    vector<float> handRootFrameTranslations;
    vector<float> handJointFrameOrientations;

    vector<float> objectFrameRotations;
    vector<float> objectFrameTranslations;

    vector<float> tableFrameRotations;
    vector<float> tableFrameTranslations;

    vector<uint32_t> objectContactFrameLocations;

    int objectContactFrameIndex = 0;
    int objectContactDataIndex = 0;

    // Hand joint rest configuration data for seeding rotations
    extractElements(handJointsRestConfigurationData, handJointsAtRest,
                    NUM_HAND_JOINTS * XYZ_BLOCK_SIZE);

    MVectorArray handJointFixedTranslations;

    for (int i = 0; i < NUM_HAND_JOINTS; i++)
    {
        string jointName = mJointNames[i];
        MDagPath joint = mJoints[jointName];

        MFnIkJoint fnJoint(joint, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int offset = i * XYZ_BLOCK_SIZE;

        float transX = handJointsAtRest[offset];
        float transY = handJointsAtRest[offset + 1];
        float transZ = handJointsAtRest[offset + 2];

        MVector jointTranslation(transX, transY, transZ);

        jointTranslation *= SCALE_MULT;

        status = handJointFixedTranslations.append(jointTranslation);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    for (int frame = 0; frame < numFrames; frame++)
    {
        // GRAB uses 120 FPS, source:
        // https://github.com/otaheri/GRAB/blob/284cba757bd10364fd38eb883c33d4490c4d98f5/README.md
        MTime newFrame((double)frame, MTime::k120FPS);

        status = animCtrl.setCurrentTime(newFrame);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        handVertexPoints.clear();
        handVertices.clear();

        handRootFrameOrientations.clear();
        handRootFrameTranslations.clear();
        handJointFrameOrientations.clear();

        objectFrameRotations.clear();
        objectFrameTranslations.clear();

        extractElements(handVerticesData +
                            (frame * numHandVertices * XYZ_BLOCK_SIZE),
                        handVertices, numHandVertices * XYZ_BLOCK_SIZE);

        extractElements(handJointOrientationsData +
                            (frame * (NUM_HAND_JOINTS - 1) * XYZ_BLOCK_SIZE),
                        handJointFrameOrientations,
                        (NUM_HAND_JOINTS - 1) * XYZ_BLOCK_SIZE);

        for (int vIndex = 0; vIndex < numHandVertices; vIndex++)
        {
            int startIndex = vIndex * XYZ_BLOCK_SIZE;

            float fhvx = handVertices[startIndex];
            float fhvy = handVertices[startIndex + 1];
            float fhvz = handVertices[startIndex + 2];

            MPoint fhvp(fhvx, fhvy, fhvz);

            fhvp *= SCALE_MULT;

            handVertexPoints.append(fhvp);

            if (mVirtualMarkerVertexMappings.contains(vIndex))
            {
                string mmName = mVirtualMarkerVertexMappings[vIndex];
                MDagPath locatorDag = mVirtualMocapMarkers[mmName];

                MVector tfTrans(fhvx, fhvy, fhvz);
                tfTrans *= SCALE_MULT;

                MFnTransform fnTransform(locatorDag, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = fnTransform.setTranslation(tfTrans, MSpace::kWorld);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                saveGenericKeyframe(mmName.c_str());
            }
        }

        // Hand root transform data
        extractElements(handRootOrientationsData + (frame * XYZ_BLOCK_SIZE),
                        handRootFrameOrientations, XYZ_BLOCK_SIZE);

        extractElements(handRootTranslationsData + (frame * XYZ_BLOCK_SIZE),
                        handRootFrameTranslations, XYZ_BLOCK_SIZE);

        MVector handRootRotation(handRootFrameOrientations[0],
                                 handRootFrameOrientations[1],
                                 handRootFrameOrientations[2]);

        MVector handRootTranslation(handRootFrameTranslations[0],
                                    handRootFrameTranslations[1],
                                    handRootFrameTranslations[2]);

        handRootTranslation *= SCALE_MULT;

        // Object transform data
        extractElements(objectRotationsData + (frame * XYZ_BLOCK_SIZE),
                        objectFrameRotations, XYZ_BLOCK_SIZE);

        extractElements(objectTranslationsData + (frame * XYZ_BLOCK_SIZE),
                        objectFrameTranslations, XYZ_BLOCK_SIZE);

        MVector objectRotation(objectFrameRotations[0], objectFrameRotations[1],
                               objectFrameRotations[2]);

        MVector objectTranslation(objectFrameTranslations[0],
                                  objectFrameTranslations[1],
                                  objectFrameTranslations[2]);

        objectTranslation *= SCALE_MULT;

        // Table transform data
        extractElements(tableRotationsData + (frame * XYZ_BLOCK_SIZE),
                        tableFrameRotations, XYZ_BLOCK_SIZE);

        extractElements(tableTranslationsData + (frame * XYZ_BLOCK_SIZE),
                        tableFrameTranslations, XYZ_BLOCK_SIZE);

        MVector tableRotation(tableFrameRotations[0], tableFrameRotations[1],
                              tableFrameRotations[2]);

        MVector tableTranslation(tableFrameTranslations[0],
                                 tableFrameTranslations[1],
                                 tableFrameTranslations[2]);

        tableTranslation *= SCALE_MULT;

        // Hand vertex data
        status = fnHandMesh.setPoints(handVertexPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Hand joint rotation data
        for (int i = 1; i < NUM_HAND_JOINTS; i++)
        {
            string jointName = mJointNames[i];
            MDagPath joint = mJoints[jointName];

            MFnIkJoint fnJoint(joint, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int offset = (i - 1) * XYZ_BLOCK_SIZE;

            float rotX = handJointFrameOrientations[offset];
            float rotY = handJointFrameOrientations[offset + 1];
            float rotZ = handJointFrameOrientations[offset + 2];

            MVector jointRotation(rotX, rotY, rotZ);

            MTransformationMatrix jtf = fnJoint.transformation();
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MVector jointTranslation = handJointFixedTranslations[i];

            double theta = jointRotation.length();
            MVector axis = jointRotation.normal();

            status = jtf.setToRotationAxis(axis, theta);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = jtf.setTranslation(jointTranslation, MSpace::kTransform);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = fnJoint.set(jtf);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            saveGenericKeyframe(jointName.c_str());
        }

        // Hand root data
        MString handRootJointName = mJointNames[0].c_str();
        MDagPath handRootJoint = mJoints[handRootJointName.asChar()];

        MFnIkJoint fnHandRootJoint(handRootJoint, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MTransformationMatrix hrtf = fnHandRootJoint.transformation();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        double theta = handRootRotation.length();
        MVector axis = handRootRotation.normal();

        status = hrtf.setToRotationAxis(axis, theta);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = hrtf.setTranslation(handRootTranslation, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnHandRootJoint.set(hrtf);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Object data
        MTransformationMatrix otf = fnObjectTransform.transformation();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        theta = objectRotation.length();
        axis = objectRotation.normal();

        // TODO: Why inverted theta?
        status = otf.setToRotationAxis(axis, -theta);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = otf.setTranslation(objectTranslation, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = otf.setScale(objectScale, MSpace::kObject);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnObjectTransform.set(otf);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Table data
        MTransformationMatrix ttf = fnTableTransform.transformation();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        theta = tableRotation.length();
        axis = tableRotation.normal();

        // TODO: Why inverted theta?
        status = ttf.setToRotationAxis(axis, -theta);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = ttf.setTranslation(tableTranslation, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = ttf.setScale(tableScale, MSpace::kObject);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnTableTransform.set(ttf);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        saveHandVertexKeyframe(numHandVertices);

        saveGenericKeyframe(handRootJointName);
        saveGenericKeyframe(OBJECT_NAME);
        saveGenericKeyframe(TABLE_NAME);

#ifndef USE_BASELINE_POINTS
        if ((int)contactFramesData[objectContactFrameIndex] == frame)
        {
            int numObjectContacts =
                (int)objectContactFrameCountsData[objectContactFrameIndex];

            objectContactFrameLocations.clear();

            extractElements(objectContactLocationsData + objectContactDataIndex,
                            objectContactFrameLocations, numObjectContacts);

            status = generateFrameContactSet(frame, objectShapeName,
                                             objectContactFrameLocations,
                                             numObjectContacts);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            objectContactFrameIndex++;
            objectContactDataIndex += objectContactFrameLocations.size();

            MString allFrameContactGroupNamesRegex =
                CONTACT_GROUP_PREFIX + MString("*_") +
                MString(to_string(frame).c_str());

            MString allFrameContactGroupName =
                CONTACT_GROUP_PREFIX + MString(to_string(frame).c_str());

            selectionList.clear();

            status = MGlobal::getSelectionListByName(
                allFrameContactGroupNamesRegex, selectionList);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnSet fnGroupSetGenerator;
            MSelectionList dummyList;

            MObject setObj = fnGroupSetGenerator.create(
                dummyList, MFnSet::Restriction::kNone, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MFnSet fnGroupSet(setObj, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            fnGroupSet.setName(allFrameContactGroupName, false, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MItSelectionList contactGroupItList(selectionList);

            for (; !contactGroupItList.isDone(); contactGroupItList.next())
            {
                MObject contactGroup;
                status = contactGroupItList.getDependNode(contactGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = fnGroupSet.addMember(contactGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }
#endif
    }

    return MS::kSuccess;
}

template <typename T>
void GRABMotionSequenceIO::extractElements(T srcArray[], vector<T> &result,
                                           int chunkSize)
{
    for (int i = 0; i < chunkSize; i++)
    {
        result.push_back(srcArray[i]);
    }
}

MStatus GRABMotionSequenceIO::generateFrameContactSet(
    int frame, MString shapeName, vector<uint32_t> &contactFrameLocations,
    int numContacts)
{
    MStatus status;

    MIntArray contactLocations;

    for (int cIndex = 0; cIndex < numContacts; cIndex++)
    {
        int contactVertexLocation = (int)contactFrameLocations[cIndex];

        status = contactLocations.append(contactVertexLocation);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(shapeName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath dp;
    status = selectionList.getDagPath(0, dp);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnGroupSetGenerator;
    MSelectionList dummyList;

    MObject groupSetObj = fnGroupSetGenerator.create(
        dummyList, MFnSet::Restriction::kNone, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnSet fnGroupSet(groupSetObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString fullContactGroupName = CONTACT_GROUP_PREFIX + shapeName + "_" +
                                   MString(to_string(frame).c_str());

    fnGroupSet.setName(fullContactGroupName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MStringArray serializedContactPoints;

    for (int i = 0; i < contactLocations.length(); i++)
    {
        MString serializedContactPoint =
            "v " + MString(to_string(contactLocations[i]).c_str());
        serializedContactPoints.append(serializedContactPoint);
    }

    status = setContactAttribute(groupSetObj, serializedContactPoints);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus GRABMotionSequenceIO::saveGenericKeyframe(MString objectName)
{
    MStatus status;

    // Because API is broken...

    MString keyTranslate = "setKeyframe { \"" + objectName + ".translate\" }";
    status = MGlobal::executeCommand(keyTranslate);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString keyRotate = "setKeyframe { \"" + objectName + ".rotate\" }";
    status = MGlobal::executeCommand(keyRotate);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus GRABMotionSequenceIO::saveHandVertexKeyframe(int numVertices)
{
    MStatus status;

    string allVerticesSegmentChar = "[0:" + to_string(numVertices - 1) + "]";
    MString allVerticesSegment = allVerticesSegmentChar.c_str();

    MString saveCommand =
        "setKeyframe { \"hand.vtx" + allVerticesSegment + "\" }";
    status = MGlobal::executeCommand(saveCommand);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus
GRABMotionSequenceIO::setContactAttribute(MObject &contactSet,
                                          MStringArray &serializedContactPoints)
{
    MStatus status;

    MFnDependencyNode fnDepNode(contactSet, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTypedAttribute fnTypedAttr;

    MObject attrObj =
        fnTypedAttr.create(CONTACT_POINTS_ATTRIBUTE, CONTACT_POINTS_ATTRIBUTE,
                           MFnData::kStringArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnDepNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(contactSet, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedContactPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
