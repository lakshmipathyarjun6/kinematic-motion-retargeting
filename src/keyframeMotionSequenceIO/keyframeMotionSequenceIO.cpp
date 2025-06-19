#include "keyframeMotionSequenceIO.hpp"

KeyframeMotionSequenceIO::KeyframeMotionSequenceIO() {}

KeyframeMotionSequenceIO::~KeyframeMotionSequenceIO() {}

void *KeyframeMotionSequenceIO::creator()
{
    return new KeyframeMotionSequenceIO();
}

bool KeyframeMotionSequenceIO::canBeOpened() const { return true; }

MString KeyframeMotionSequenceIO::defaultExtension() const { return "kmexp"; }

MStatus KeyframeMotionSequenceIO::exportAll(string &filename)
{
    MStatus status;
    MGlobal::displayInfo("Writing file...");

    status = parseKinematicTree();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MSelectionList selectionList;

    MAnimControl animCtrl;

    Document d;
    d.SetObject();

    Document::AllocatorType &allocator = d.GetAllocator();

    MTime time = animCtrl.currentTime();
    MTime::Unit framerate = time.unit();

    Value frameRateVal(kNumberType);
    int frameRateNum;

    switch (framerate)
    {
    case MTime::k24FPS:
        frameRateNum = 24;
        break;
    case MTime::k30FPS:
        frameRateNum = 30;
        break;
    case MTime::k60FPS:
        frameRateNum = 60;
        break;
    case MTime::k120FPS:
        frameRateNum = 120;
        break;
    default:
        cout << "Unknown framerate" << endl;
        break;
    }

    frameRateVal.SetInt(frameRateNum);
    d.AddMember("framerate", frameRateVal, allocator);

    Value jointNames(kArrayType);

    int numJoints = m_joint_names.length();

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++)
    {
        MString jointName = m_joint_names[jointIndex];

        Value jName(kStringType);
        jName.SetString(jointName.asChar(), jointName.length());
        jointNames.PushBack(jName, allocator);
    }

    d.AddMember("jointNames", jointNames, allocator);

    Value jointDofs(kArrayType);

    int numJointDofs = m_joint_num_dofs.length();

    for (int jointDofIndex = 0; jointDofIndex < numJointDofs; jointDofIndex++)
    {
        int numJointDofs = m_joint_num_dofs[jointDofIndex];

        Value jointDofNumber(kNumberType);
        jointDofNumber.SetInt(numJointDofs);

        jointDofs.PushBack(jointDofNumber, allocator);
    }

    d.AddMember("jointDofs", jointDofs, allocator);

    status = MGlobal::getSelectionListByName(TABLE_NAME, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath tableDag;
    status = selectionList.getDagPath(0, tableDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTableTransform(tableDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MQuaternion tableRotation;
    double tableScale[3];

    status = fnTableTransform.getScale(tableScale);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnTableTransform.getRotation(tableRotation, MSpace::kTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MVector tableTranslation =
        fnTableTransform.getTranslation(MSpace::kWorld, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    Value tableState(kArrayType);

    Value tableScaleX(kNumberType);
    Value tableScaleY(kNumberType);
    Value tableScaleZ(kNumberType);
    Value tableStateRotW(kNumberType);
    Value tableStateRotX(kNumberType);
    Value tableStateRotY(kNumberType);
    Value tableStateRotZ(kNumberType);
    Value tableStateTransX(kNumberType);
    Value tableStateTransY(kNumberType);
    Value tableStateTransZ(kNumberType);

    tableScaleX.SetDouble(tableScale[0]);
    tableScaleY.SetDouble(tableScale[1]);
    tableScaleZ.SetDouble(tableScale[2]);
    tableStateRotW.SetDouble(tableRotation.w);
    tableStateRotX.SetDouble(tableRotation.x);
    tableStateRotY.SetDouble(tableRotation.y);
    tableStateRotZ.SetDouble(tableRotation.z);
    tableStateTransX.SetDouble(tableTranslation.x);
    tableStateTransY.SetDouble(tableTranslation.y);
    tableStateTransZ.SetDouble(tableTranslation.z);

    tableState.PushBack(tableScaleX, allocator);
    tableState.PushBack(tableScaleY, allocator);
    tableState.PushBack(tableScaleZ, allocator);
    tableState.PushBack(tableStateRotW, allocator);
    tableState.PushBack(tableStateRotX, allocator);
    tableState.PushBack(tableStateRotY, allocator);
    tableState.PushBack(tableStateRotZ, allocator);
    tableState.PushBack(tableStateTransX, allocator);
    tableState.PushBack(tableStateTransY, allocator);
    tableState.PushBack(tableStateTransZ, allocator);

    d.AddMember("table", tableState, allocator);

    status = selectionList.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(OBJECT_NAME, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath objectMeshTransform;
    status = selectionList.getDagPath(0, objectMeshTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnObjectTransform(objectMeshTransform, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int startFrame = (int)animCtrl.minTime().value();
    int endFrame = (int)animCtrl.maxTime().value();

    Value dataFrames(kArrayType);

    for (int frame = startFrame; frame <= endFrame; frame++)
    {
        selectionList.clear();

        MTime newFrame((double)frame, framerate);

        status = animCtrl.setCurrentTime(newFrame);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        Value frameData(kObjectType);

        Value frameNumber(kNumberType);
        frameNumber.SetInt(frame);
        frameData.AddMember("frame", frameNumber, allocator);

        Value objectState(kArrayType);
        Value handState(kArrayType);

        // Object data
        MQuaternion objectRotation;

        Value objectStateRotW(kNumberType);
        Value objectStateRotX(kNumberType);
        Value objectStateRotY(kNumberType);
        Value objectStateRotZ(kNumberType);
        Value objectStateTransX(kNumberType);
        Value objectStateTransY(kNumberType);
        Value objectStateTransZ(kNumberType);

        status =
            fnObjectTransform.getRotation(objectRotation, MSpace::kTransform);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MVector objectTranslation =
            fnObjectTransform.getTranslation(MSpace::kWorld, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        objectStateRotW.SetDouble(objectRotation.w);
        objectStateRotX.SetDouble(objectRotation.x);
        objectStateRotY.SetDouble(objectRotation.y);
        objectStateRotZ.SetDouble(objectRotation.z);
        objectStateTransX.SetDouble(objectTranslation.x);
        objectStateTransY.SetDouble(objectTranslation.y);
        objectStateTransZ.SetDouble(objectTranslation.z);

        objectState.PushBack(objectStateRotW, allocator);
        objectState.PushBack(objectStateRotX, allocator);
        objectState.PushBack(objectStateRotY, allocator);
        objectState.PushBack(objectStateRotZ, allocator);
        objectState.PushBack(objectStateTransX, allocator);
        objectState.PushBack(objectStateTransY, allocator);
        objectState.PushBack(objectStateTransZ, allocator);

        // Hand Data
        status = loadDofSolutionFull();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (int i = 0; i < m_rig_n_dofs; i++)
        {
            Value handJointDofVal(kNumberType);
            handJointDofVal.SetDouble(m_dof_vector[i]);
            handState.PushBack(handJointDofVal, allocator);
        }

        frameData.AddMember("object", objectState, allocator);
        frameData.AddMember("hand", handState, allocator);

        dataFrames.PushBack(frameData, allocator);
    }

    d.AddMember("data", dataFrames, allocator);

    writeJSON(d, filename);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

bool KeyframeMotionSequenceIO::haveReadMethod() const { return false; }

bool KeyframeMotionSequenceIO::haveWriteMethod() const { return true; }

MPxFileTranslator::MFileKind
KeyframeMotionSequenceIO::identifyFile(const MFileObject &fileName,
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

MStatus KeyframeMotionSequenceIO::writer(const MFileObject &file,
                                         const MString &optionString,
                                         FileAccessMode mode)
{
    MStatus status;

    MString resolvedPath = file.resolvedPath();
    MString resolvedName = file.resolvedName();

    MString resolvedFullName = resolvedPath + resolvedName;

    string fileNameChar = resolvedFullName.asChar();

    if (mode == MPxFileTranslator::kExportAccessMode)
    {
        status = exportAll(fileNameChar);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

// Begin Protected Utils

MStatus KeyframeMotionSequenceIO::loadDofSolutionFull()
{
    MStatus status;

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        status = loadDofSolutionSingle(i);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus KeyframeMotionSequenceIO::loadDofSolutionSingle(int index)
{
    MStatus status;

    pair<int, int> indices = m_dof_vec_mappings.at(index);

    int jointIndex = indices.first;
    int dofIndex = indices.second;

    int numJointDofs = m_joint_num_dofs[jointIndex];

    MDagPath joint = m_rig_joints[jointIndex];

    MFnIkJoint fnJoint(joint, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

#ifdef USE_QUATS
    if (dofIndex > 3) // indicates translation dof
    {
        MVector transVec = fnJoint.getTranslation(MSpace::kWorld, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_dof_vector[index] = transVec[dofIndex - 4];
    }
    else if (jointIndex == 0 || numJointDofs == 4)
    {
        MQuaternion qRot;

        status = fnJoint.getRotation(qRot, MSpace::kTransform);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        switch (dofIndex)
        {
        case 0:
            m_dof_vector[index] = qRot.w;
            break;
        case 1:
            m_dof_vector[index] = qRot.x;
            break;
        case 2:
            m_dof_vector[index] = qRot.y;
            break;
        case 3:
            m_dof_vector[index] = qRot.z;
            break;
        default:
            cout << "ERROR: Unknown quaternion dof index" << endl;
            break;
        }
    }
    else
    {
        double rotation[3];
        MTransformationMatrix::RotationOrder order;

        status = fnJoint.getRotation(rotation, order);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_dof_vector[index] = rotation[dofIndex];
    }
#else
    if (dofIndex > 2) // indicates translation dof
    {
        MVector transVec = fnJoint.getTranslation(MSpace::kWorld, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_dof_vector[index] = transVec[dofIndex - 3];
    }
    else
    {
        double rotation[3];
        MTransformationMatrix::RotationOrder order;

        status = fnJoint.getRotation(rotation, order);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_dof_vector[index] = rotation[dofIndex];
    }
#endif

    return MS::kSuccess;
}

MStatus KeyframeMotionSequenceIO::parseKinematicTree()
{
    MStatus status;

    status = m_joint_num_dofs.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

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

        int numJointDofs = 0;

#ifdef USE_QUATS
        // Treat root joint as special free joint
        if (numJoints == 0)
        {
            pair<int, int> wRotDof = make_pair(numJoints, 0);
            pair<int, int> xRotDof = make_pair(numJoints, 1);
            pair<int, int> yRotDof = make_pair(numJoints, 2);
            pair<int, int> zRotDof = make_pair(numJoints, 3);

            pair<int, int> xTransDof = make_pair(numJoints, 4);
            pair<int, int> yTransDof = make_pair(numJoints, 5);
            pair<int, int> zTransDof = make_pair(numJoints, 6);

            m_dof_vec_mappings.push_back(wRotDof);
            m_dof_vec_mappings.push_back(xRotDof);
            m_dof_vec_mappings.push_back(yRotDof);
            m_dof_vec_mappings.push_back(zRotDof);

            m_dof_vec_mappings.push_back(xTransDof);
            m_dof_vec_mappings.push_back(yTransDof);
            m_dof_vec_mappings.push_back(zTransDof);

            MQuaternion qRot;

            status = fnJoint.getRotation(qRot, MSpace::kTransform);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MVector vTrans = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_dof_vector.append(qRot.w);
            m_dof_vector.append(qRot.x);
            m_dof_vector.append(qRot.y);
            m_dof_vector.append(qRot.z);

            m_dof_vector.append(vTrans[0]);
            m_dof_vector.append(vTrans[1]);
            m_dof_vector.append(vTrans[2]);

            numJointDofs = 7;
        }
        else
        {
            bool freeX = false;
            bool freeY = false;
            bool freeZ = false;

            status = fnJoint.getDegreesOfFreedom(freeX, freeY, freeZ);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            // Complete ball joint
            if (freeX && freeY && freeZ)
            {
                pair<int, int> wRotDof = make_pair(numJoints, 0);
                pair<int, int> xRotDof = make_pair(numJoints, 1);
                pair<int, int> yRotDof = make_pair(numJoints, 2);
                pair<int, int> zRotDof = make_pair(numJoints, 3);

                m_dof_vec_mappings.push_back(wRotDof);
                m_dof_vec_mappings.push_back(xRotDof);
                m_dof_vec_mappings.push_back(yRotDof);
                m_dof_vec_mappings.push_back(zRotDof);

                MQuaternion qRot;

                status = fnJoint.getRotation(qRot, MSpace::kTransform);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                m_dof_vector.append(qRot.w);
                m_dof_vector.append(qRot.x);
                m_dof_vector.append(qRot.y);
                m_dof_vector.append(qRot.z);

                numJointDofs = 4;
            }
            else
            {
                double jointRotations[3];
                MTransformationMatrix::RotationOrder rOrder;

                status = fnJoint.getRotation(jointRotations, rOrder);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                if (freeX)
                {
                    pair<int, int> xRotDof = make_pair(numJoints, 0);
                    m_dof_vec_mappings.push_back(xRotDof);
                    m_dof_vector.append(jointRotations[0]);
                    numJointDofs++;
                }
                if (freeY)
                {
                    pair<int, int> yRotDof = make_pair(numJoints, 1);
                    m_dof_vec_mappings.push_back(yRotDof);
                    m_dof_vector.append(jointRotations[1]);
                    numJointDofs++;
                }
                if (freeZ)
                {
                    pair<int, int> zRotDof = make_pair(numJoints, 2);
                    m_dof_vec_mappings.push_back(zRotDof);
                    m_dof_vector.append(jointRotations[2]);
                    numJointDofs++;
                }
            }
        }
#else
        bool freeX = false;
        bool freeY = false;
        bool freeZ = false;

        status = fnJoint.getDegreesOfFreedom(freeX, freeY, freeZ);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        double jointRotations[3];
        MTransformationMatrix::RotationOrder rOrder;

        status = fnJoint.getRotation(jointRotations, rOrder);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (freeX)
        {
            pair<int, int> xRotDof = make_pair(numJoints, 0);
            m_dof_vec_mappings.push_back(xRotDof);
            m_dof_vector.append(jointRotations[0]);
            numJointDofs++;
        }
        if (freeY)
        {
            pair<int, int> yRotDof = make_pair(numJoints, 1);
            m_dof_vec_mappings.push_back(yRotDof);
            m_dof_vector.append(jointRotations[1]);
            numJointDofs++;
        }
        if (freeZ)
        {
            pair<int, int> zRotDof = make_pair(numJoints, 2);
            m_dof_vec_mappings.push_back(zRotDof);
            m_dof_vector.append(jointRotations[2]);
            numJointDofs++;
        }

        if (numJoints == 0)
        {
            pair<int, int> xTransDof = make_pair(numJoints, 3);
            pair<int, int> yTransDof = make_pair(numJoints, 4);
            pair<int, int> zTransDof = make_pair(numJoints, 5);

            m_dof_vec_mappings.push_back(xTransDof);
            m_dof_vec_mappings.push_back(yTransDof);
            m_dof_vec_mappings.push_back(zTransDof);

            MVector vTrans = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_dof_vector.append(vTrans[0]);
            m_dof_vector.append(vTrans[1]);
            m_dof_vector.append(vTrans[2]);

            numJointDofs += 3;
        }
#endif

        // Ignore all welded joints
        if (numJointDofs > 0)
        {
            m_rig_joints.append(dp);
            numJoints++;

            status = m_joint_num_dofs.append(numJointDofs);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MString jointName = dp.partialPathName();
            m_joint_names.append(jointName);

            MGlobal::displayInfo(jointName + " " +
                                 MString(to_string(numJoints).c_str()) + " " +
                                 MString(to_string(numJointDofs).c_str()));
        }

        int numChildren = dp.childCount();

        for (int i = 0; i < numChildren; i++)
        {
            status = dp.push(dp.child(i));
            CHECK_MSTATUS_AND_RETURN_IT(status);

            stack.push(dp);
            dp.pop();
        }

        nDofs += numJointDofs;
    }

    m_rig_n_dofs = nDofs;

    MGlobal::displayInfo(to_string(m_rig_n_dofs).c_str());
    MGlobal::displayInfo(to_string(m_dof_vec_mappings.size()).c_str());

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        pair<int, int> jm = m_dof_vec_mappings.at(i);
        float value = m_dof_vector[i];

        MGlobal::displayInfo(MString(to_string(jm.first).c_str()) + " " +
                             MString(to_string(jm.second).c_str()) + " " +
                             MString(to_string(value).c_str()));
    }

    return MS::kSuccess;
}
