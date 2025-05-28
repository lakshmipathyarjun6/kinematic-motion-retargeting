#include "fusedMotionEditContext.hpp"

// Setup and Teardown

FusedMotionEditContext::FusedMotionEditContext()
    : m_num_opt_iterations(100), m_contact_distance_penalty_coefficient(1.0),
      m_contact_normal_penalty_coefficient(1.0),
      m_marker_penalty_coefficient(1.0), m_contact_penalty_coefficient(1.0),
      m_intersection_penalty_coefficient(1.0),
      m_prior_penalty_coefficient(50.0),
      m_optimization_visualization_enabled(false), m_acceleration_epsilon(500.0)
{
    m_rng = default_random_engine{};

    m_fpsRealtimeConversionTable[MTime::Unit::k24FPS] = 1.0 / 24.0;
    m_fpsRealtimeConversionTable[MTime::Unit::k40FPS] = 1.0 / 40.0;
    m_fpsRealtimeConversionTable[MTime::Unit::k59_94FPS] = 1.0 / 59.94;
    m_fpsRealtimeConversionTable[MTime::Unit::k120FPS] = 1.0 / 120.0;
}

FusedMotionEditContext::~FusedMotionEditContext() {}

void *FusedMotionEditContext::creator() { return new FusedMotionEditContext(); }

void FusedMotionEditContext::toolOnSetup(MEvent &event)
{
    MStatus status;

    MGlobal::displayInfo("Setting up fused motion edit context...");

    m_view = M3dView::active3dView();

    m_rig_base = MDagPath();
    m_rig_n_dofs = 0;

    m_rig_joints.clear();
    m_dof_vector.clear();
    m_dof_vec_mappings.clear();
    m_joint_names.clear();

    MAnimControl animCtrl;
    MTime time = animCtrl.currentTime();
    m_frame = (int)time.value();
    m_framerate = time.unit();
    m_realtime_delta = m_fpsRealtimeConversionTable[m_framerate];

    status = loadAllGeometries();
    CHECK_MSTATUS(status);

    status = pluckObjectAndHandMesh();
    CHECK_MSTATUS(status);

    status = loadTable();
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

void FusedMotionEditContext::toolOffCleanup()
{
    MStatus status;

    MGlobal::displayInfo("Tearing down fused motion edit context...");

    status = wipeContactPairingLines();
    CHECK_MSTATUS(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS(status);

    status = clearVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

// Tool Sheet Reference Properties

void FusedMotionEditContext::getClassName(MString &name) const
{
    name.set("fusedMotionEditContext");
}

MStatus FusedMotionEditContext::computeAccelerationErrors(int frameStart,
                                                          int frameEnd)
{
    MStatus status;

    MSelectionList selectionList;

    MGlobal::displayInfo("Computing...");

    // Load all solutions and acceleration errors

    status = storeExistingFrameSolutions(frameStart, frameEnd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vector<MPointArray> accelerationErrors;

    m_acceleration_violations.clear();

    for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
    {
        accelerationErrors.push_back(MPointArray());
    }

    for (int frame = frameStart; frame <= frameEnd; frame++)
    {
        MDoubleArray currentFrameSolutions = m_existing_frame_solutions[frame];
        MDoubleArray priorFrameSolutions =
            m_existing_frame_solutions[frame - 1];
        MDoubleArray nextFrameSolutions = m_existing_frame_solutions[frame + 1];

        for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
        {
            pair<int, int> indices = m_dof_vec_mappings.at(rigDofIndex);
            int jointDofIndex = indices.second;

            double currentDofValue = currentFrameSolutions[rigDofIndex];
            double priorDofValue = priorFrameSolutions[rigDofIndex];
            double nextDofValue = nextFrameSolutions[rigDofIndex];

            // Convert rotations to degrees for better scale consistency
            if (jointDofIndex < 3)
            {
                currentDofValue *= 180.0 / M_PI;
                priorDofValue *= 180.0 / M_PI;
                nextDofValue *= 180.0 / M_PI;
            }

            double incomingVelocity =
                (currentDofValue - priorDofValue) / m_realtime_delta;
            double outgoingVelocity =
                (nextDofValue - currentDofValue) / m_realtime_delta;

            // Via finite acceleration metric
            double accelerationError = abs(outgoingVelocity - incomingVelocity);

            status = accelerationErrors[rigDofIndex].append(
                MPoint(frame, accelerationError));
            CHECK_MSTATUS_AND_RETURN_IT(status);

            if (accelerationError > m_acceleration_epsilon)
            {
                if (m_acceleration_violations.contains(frame))
                {
                    status =
                        m_acceleration_violations[frame].append(rigDofIndex);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                }
                else
                {
                    m_acceleration_violations[frame] =
                        MIntArray(1, rigDofIndex);
                }
            }
        }
    }

    status = jumpToFrame(frameStart, true);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Plot

    status = MGlobal::getSelectionListByName(ACCELERATION_ERROR_LINES_GROUP,
                                             selectionList);

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

    fnSplineGroupTransform.setName(ACCELERATION_ERROR_LINES_GROUP, false,
                                   &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int rigDofIndex = 0; rigDofIndex < m_rig_n_dofs; rigDofIndex++)
    {
        MFnNurbsCurve fnSplineGenerator;

        MPointArray controlPoints = accelerationErrors[rigDofIndex];

        MObject splineDofCurve = fnSplineGenerator.createWithEditPoints(
            controlPoints, 1, MFnNurbsCurve::kOpen, false, true, true,
            splineGroup, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString splineName = ACCELERATION_ERROR_SPLINE_DOF_PREFIX + rigDofIndex;

        fnSplineGenerator.setName(splineName, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    status = redrawAccelerationViolationCutoff(frameStart, frameEnd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus
FusedMotionEditContext::enableOptimizationProgressVisualization(bool enable)
{
    m_optimization_visualization_enabled = enable;

    if (m_optimization_visualization_enabled)
    {
        MGlobal::displayInfo("Optimization visualization progress enabled");
    }
    else
    {
        MGlobal::displayInfo("Optimization visualization progress disabled");
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::finalizeOmissionIndices(int frameStart,
                                                        int frameEnd)
{
    MStatus status;

    MGlobal::displayInfo("Finalizing omission indices...");

    MSelectionList selectionList;

    status = wipeContactPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = clearVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString objectShapeName = OBJECT_NAME + "Shape_";
    MString handShapeName = m_hand_name + "Shape_";

    for (int frame = frameStart; frame <= frameEnd; frame++)
    {
        status = jumpToFrame(frame, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString frameStringSuffix = MString(to_string(frame).c_str());

        MString objectContactGroupName =
            CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;

        MString handContactGroupName =
            CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

        MIntArray omissionIndices;
        status =
            getOmissionIndicesAttribute(handContactGroupName, omissionIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (omissionIndices.length() > 0)
        {
            set<int> omissionIndicesSet;

            for (int index = 0; index < omissionIndices.length(); index++)
            {
                int omissionIndex = omissionIndices[index];
                omissionIndicesSet.insert(omissionIndex);
            }

            MStringArray serializedObjectContactPoints;
            MStringArray serializedHandContactPoints;

            status = getSetSerializedPointsAttribute(
                objectContactGroupName, CONTACT_POINTS_ATTRIBUTE,
                serializedObjectContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = getSetSerializedPointsAttribute(
                handContactGroupName, CONTACT_POINTS_ATTRIBUTE,
                serializedHandContactPoints);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            int numOriginalPoints = serializedObjectContactPoints.length();

            MStringArray cleanedSerializedObjectContactPoints;
            MStringArray cleanedSerializedHandContactPoints;

            for (int i = 0; i < numOriginalPoints; i++)
            {
                MString serializedObjectContactPoint =
                    serializedObjectContactPoints[i];
                MString serializedHandContactPoint =
                    serializedHandContactPoints[i];

                if (!omissionIndicesSet.contains(i))
                {
                    status = cleanedSerializedObjectContactPoints.append(
                        serializedObjectContactPoint);
                    CHECK_MSTATUS_AND_RETURN_IT(status);

                    status = cleanedSerializedHandContactPoints.append(
                        serializedHandContactPoint);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                }
            }

            if (cleanedSerializedHandContactPoints.length() == 0 ||
                cleanedSerializedObjectContactPoints.length() == 0)
            {
                MString contactGroupName =
                    CONTACT_GROUP_PREFIX + frameStringSuffix;

                cout << "Removing group " << contactGroupName << endl;

                status = selectionList.clear();
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = MGlobal::getSelectionListByName(contactGroupName,
                                                         selectionList);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MObject frameContactGroup;
                status = selectionList.getDependNode(0, frameContactGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = MGlobal::deleteNode(frameContactGroup);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
            else
            {
                status =
                    setContactAttribute(objectContactGroupName,
                                        cleanedSerializedObjectContactPoints);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = setContactAttribute(
                    handContactGroupName, cleanedSerializedHandContactPoints);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = selectionList.clear();
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status = MGlobal::getSelectionListByName(handContactGroupName,
                                                         selectionList);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MObject handContactSet;
                status = selectionList.getDependNode(0, handContactSet);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MFnSet fnHandContactSet(handContactSet, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                MObject omissionIndicesAttrObj = fnHandContactSet.attribute(
                    CONTACT_OMISSION_INDICES_ATTRIBUTE, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                status =
                    fnHandContactSet.removeAttribute(omissionIndicesAttrObj);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::jumpToFrame(int frame,
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
        status = clearVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawMarkerVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    m_view.refresh(true, true);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::keyframeRig()
{
    MStatus status;

    MGlobal::displayInfo("Keyframing rig...");

    for (int i = 0; i < m_joint_names.length(); i++)
    {
        MString jointName = m_joint_names[i];

        MString saveCommand = "setKeyframe { \"" + jointName + "\" }";
        status = MGlobal::executeCommand(saveCommand);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::keyframeRigBulk(int frameStart, int frameEnd,
                                                bool saveKeysOnly)
{
    MStatus status;

    MSelectionList selectionList;

    MGlobal::displayInfo("Running...");

    // Hide visualizations for better performance

    status = wipeContactPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = clearVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

#ifdef RUN_OPTIMIZATION_TIMER
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
#endif

    nlopt::opt opt = initializeOptimization();

    for (int frame = frameStart; frame <= frameEnd; frame++)
    {
        status = jumpToFrame(frame, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MGlobal::displayInfo(to_string(m_frame).c_str());

        if (!saveKeysOnly)
        {
            status = initializeDofSolution();
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = runOptimization(opt);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        status = keyframeRig();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

#ifdef RUN_OPTIMIZATION_TIMER
    chrono::steady_clock::time_point end = chrono::steady_clock::now();

    cout << "Time taken to execute = "
         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
         << "[ms]" << endl;
#endif

    status = redrawContactVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = redrawMarkerVisualizations();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::resetNonRootJoints(bool suppressVisualization)
{
    MStatus status;

    MGlobal::displayInfo("Resetting non-root joints...");

    status = initializeDofSolution();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        // Ignore root joint
        if (i > 5)
        {
            m_dof_vector[i] = 0.0;
        }
    }

    status = loadDofSolutionFull();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!suppressVisualization)
    {
        status = redrawContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawMarkerVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

// Use cout here instead of displayInfo since Maya will be frozen
MStatus FusedMotionEditContext::resolveAccelerationErrors(int frameStart,
                                                          int frameEnd,
                                                          int maxIterations)
{
    MStatus status;

    MGlobal::displayInfo("Resolving...");

    nlopt::opt opt = initializeOptimization();

    int iteration = 0;

    for (; iteration < maxIterations; iteration++)
    {
        int numViolations = m_acceleration_violations.size();

        cout << "Iteration " << iteration + 1 << ": "
             << m_acceleration_violations.size() << " violations" << endl;

        if (numViolations <= 0)
        {
            cout << "No violations found - successfull completion" << endl;
            break;
        }

        status = wipeContactPairingLines();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = wipeMarkerPairingLines();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = clearVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (const auto &entry : m_acceleration_violations)
        {
            int violationFrame = entry.first;
            MIntArray violationIndices = entry.second;

            for (int i = 0; i < violationIndices.length(); i++)
            {
                int rigViolationIndex = violationIndices[i];

                pair<int, int> indices =
                    m_dof_vec_mappings.at(rigViolationIndex);

                int jointIndex = indices.first;
                int dofIndex = indices.second;

                MString jointName = m_rig_joints[jointIndex].partialPathName();

                status = wipeJointKeyframe(jointName, violationFrame,
                                           violationFrame);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            }
        }

        status = keyframeRigBulk(frameStart, frameEnd, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = wipeContactPairingLines();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = wipeMarkerPairingLines();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = clearVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        for (const auto &entry : m_acceleration_violations)
        {
            int violationFrame = entry.first;

            status = jumpToFrame(violationFrame, true);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            MGlobal::displayInfo(to_string(m_frame).c_str());

            status = initializeDofSolution();
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = runOptimization(opt);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = keyframeRig();
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        status = computeAccelerationErrors(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = jumpToFrame(frameStart);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (iteration >= maxIterations)
    {
        cout << "Exhausted all iterations" << endl;
    }

    cout << "Remaining violations:" << endl;

    for (const auto &entry : m_acceleration_violations)
    {
        int violationFrame = entry.first;
        MIntArray violationIndices = entry.second;

        cout << violationFrame << " " << violationIndices << " [";

        for (int i = 0; i < violationIndices.length(); i++)
        {
            int violationIndex = violationIndices[i];

            pair<int, int> indices = m_dof_vec_mappings.at(violationIndex);

            int jointIndex = indices.first;
            int dofIndex = indices.second;

            MDagPath joint = m_rig_joints[jointIndex];
            MString jointName = joint.partialPathName();

            cout << jointName << " " << dofIndex << ", ";
        }

        cout << "]" << endl;
    }

    status = jumpToFrame(frameStart);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setAccelerationViolationEpsilon(int frameStart,
                                                                int frameEnd,
                                                                double epsilon)
{
    MStatus status;

    MGlobal::displayInfo("Adjusting acceleration violation epsilon to: " +
                         MString(to_string(epsilon).c_str()));

    m_acceleration_epsilon = epsilon;

    status = redrawAccelerationViolationCutoff(frameStart, frameEnd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setCoefficientContactDistanceError(
    double contactDistanceCoefficient)
{
    MGlobal::displayInfo(
        "Adjusting contact distnace coefficient to: " +
        MString(to_string(contactDistanceCoefficient).c_str()));

    m_contact_distance_penalty_coefficient = contactDistanceCoefficient;

    return MS::kSuccess;
}

MStatus
FusedMotionEditContext::setCoefficientContactError(double contactCoefficient)
{
    MGlobal::displayInfo("Adjusting contact contribution coefficient to: " +
                         MString(to_string(contactCoefficient).c_str()));

    m_contact_penalty_coefficient = contactCoefficient;

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setCoefficientContactNormalError(
    double contactNormalCoefficient)
{
    MGlobal::displayInfo("Adjusting contact normal coefficient to: " +
                         MString(to_string(contactNormalCoefficient).c_str()));

    m_contact_normal_penalty_coefficient = contactNormalCoefficient;

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setCoefficientIntersectionError(
    double intersectionCoefficient)
{
    MGlobal::displayInfo("Adjusting intersection coefficient to: " +
                         MString(to_string(intersectionCoefficient).c_str()));

    m_intersection_penalty_coefficient = intersectionCoefficient;

    return MS::kSuccess;
}

MStatus
FusedMotionEditContext::setCoefficientMarkerError(double markerCoefficient)
{
    MGlobal::displayInfo("Adjusting marker contribution coefficient to: " +
                         MString(to_string(markerCoefficient).c_str()));

    m_marker_penalty_coefficient = markerCoefficient;

    return MS::kSuccess;
}

MStatus
FusedMotionEditContext::setCoefficientPriorError(double priorCoefficient)
{
    MGlobal::displayInfo("Adjusting prior coefficient to: " +
                         MString(to_string(priorCoefficient).c_str()));

    m_prior_penalty_coefficient = priorCoefficient;

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setNumOptIterations(int numIterations)
{
    MGlobal::displayInfo("Adjusting optimization iterations to: " +
                         MString(to_string(numIterations).c_str()));

    m_num_opt_iterations = numIterations;

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::storeAccelerationErrors()
{
    MStatus status;

    MGlobal::displayInfo("Storing...");

    MStringArray allViolationIndices;

    for (const auto &entry : m_acceleration_violations)
    {
        int violationFrame = entry.first;
        MIntArray violationIndices = entry.second;

        MString frameViolationIndices = to_string(violationFrame).c_str();
        frameViolationIndices += " ";

        for (int i = 0; i < violationIndices.length() - 1; i++)
        {
            int violationIndex = violationIndices[i];
            frameViolationIndices += to_string(violationIndex).c_str();
            frameViolationIndices += " ";
        }

        frameViolationIndices +=
            to_string(violationIndices[violationIndices.length() - 1]).c_str();

        status = allViolationIndices.append(frameViolationIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    status = setSerializedViolationsAttribute(allViolationIndices);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::undoOptimization()
{
    MStatus status;

    MGlobal::displayInfo("Undoing last optimization...");

    if (m_last_dof_vectors.size() > 0)
    {
        m_dof_vector = m_last_dof_vectors.top();
        m_last_dof_vectors.pop();

        status = loadDofSolutionFull();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawContactVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = redrawMarkerVisualizations();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        MGlobal::displayInfo("Optimization undo stack is empty - did nothing");
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::wipeRigKeyframe(int frameStart, int frameEnd)
{
    MStatus status;

    MGlobal::displayInfo("Wiping rig keyframe...");

    for (int i = 0; i < m_joint_names.length(); i++)
    {
        MString jointName = m_joint_names[i];

        status = wipeJointKeyframe(jointName, frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MGlobal::displayInfo("Done");

    return MS::kSuccess;
}

// UI Event Handlers

void FusedMotionEditContext::completeAction()
{
    MStatus status;

    status = wipeContactPairingLines();
    CHECK_MSTATUS(status);

    status = wipeMarkerPairingLines();
    CHECK_MSTATUS(status);

    status = clearVisualizations();
    CHECK_MSTATUS(status);

    m_view.refresh(true, true);

    nlopt::opt opt = initializeOptimization();

    MGlobal::displayInfo("Running optimization. Please wait....");

    status = initializeDofSolution();
    CHECK_MSTATUS(status);

    status = runOptimization(opt);
    CHECK_MSTATUS(status);

    status = redrawContactVisualizations();
    CHECK_MSTATUS(status);

    status = redrawMarkerVisualizations();
    CHECK_MSTATUS(status);

    MGlobal::displayInfo("Done");
}

// Core Context Setup

MStatus FusedMotionEditContext::loadAllGeometries()
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
        int tableSubstringIndex = transformName.indexW(TABLE_NAME);

        bool excludeMesh =
            exclusionSubstringIndex != -1 || tableSubstringIndex != -1;

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

MStatus FusedMotionEditContext::loadAllVirtualMarkers()
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

MStatus FusedMotionEditContext::loadMarkerPatchPairing(MString &markerPatchName)
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

MStatus FusedMotionEditContext::loadTable()
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(TABLE_NAME, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("Table not found - skipping transform loading");
        m_table_transform_inv[3][3] = -1;
        return MS::kSuccess;
    }

    MDagPath tableDag;
    status = selectionList.getDagPath(0, tableDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnTransform fnTableTransform(tableDag, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    double scale[3];
    status = fnTableTransform.getScale(scale);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MVector boxDims(scale[0], scale[1], scale[2]);
    boxDims *= 0.5;
    m_table_box_dims = boxDims;

    MTransformationMatrix tfTable = fnTableTransform.transformation(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    double unitScale[3] = {1, 1, 1};
    status = tfTable.setScale(unitScale, MSpace::kTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Maya apparently transposes the conventional matrix
    m_table_transform_inv = tfTable.asMatrix().transpose().inverse();

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::parseKinematicTree()
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
            m_dof_vec_mappings.push_back(xRotDof);
            m_dof_vector.append(jointRotations[0]);
            numJointDofs++;
            nDofs++;
        }
        if (freeY)
        {
            pair<int, int> yRotDof = make_pair(numJoints, 1);
            m_dof_vec_mappings.push_back(yRotDof);
            m_dof_vector.append(jointRotations[1]);
            numJointDofs++;
            nDofs++;
        }
        if (freeZ)
        {
            pair<int, int> zRotDof = make_pair(numJoints, 2);
            m_dof_vec_mappings.push_back(zRotDof);
            m_dof_vector.append(jointRotations[2]);
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

            m_dof_vec_mappings.push_back(xTransDof);
            m_dof_vec_mappings.push_back(yTransDof);
            m_dof_vec_mappings.push_back(zTransDof);

            MVector vTrans = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_dof_vector.append(vTrans[0]);
            m_dof_vector.append(vTrans[1]);
            m_dof_vector.append(vTrans[2]);

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
    MGlobal::displayInfo(to_string(m_dof_vec_mappings.size()).c_str());

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        pair<int, int> jm = m_dof_vec_mappings.at(i);
        float value = m_dof_vector[i];

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

        // Easier to look up in command line
        cout << i << " " << joint.partialPathName() << " " << dofName << " "
             << value << endl;
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::pluckObjectAndHandMesh()
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

MStatus FusedMotionEditContext::createMeshVisualizationSphere(
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

    status = createVisualizationSphere(position, name, radius, shadingNodeName,
                                       fnTransform);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::createVisualizationSphere(
    MFloatPoint &position, MString name, float radius, MString shadingNodeName,
    MFnTransform &fnTransform)
{
    MStatus status;

    MSelectionList selectionList;

    char command[COMMAND_BUFFER_SIZE];
    memset(command, 0, sizeof(command));

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

MStatus
FusedMotionEditContext::redrawAccelerationViolationCutoff(int frameStart,
                                                          int frameEnd)
{
    MStatus status;

    MSelectionList selectionList;

    status = MGlobal::getSelectionListByName(ACCELERATION_VIOLATION_CUTOFF_LINE,
                                             selectionList);

    if (status == MS::kSuccess)
    {
        MObject existingViolationLine;
        status = selectionList.getDependNode(0, existingViolationLine);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = MGlobal::deleteNode(existingViolationLine);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MFnNurbsCurve fnSplineGenerator;

    MPoint startPoint = MPoint(frameStart, m_acceleration_epsilon);
    MPoint endPoint = MPoint(frameEnd, m_acceleration_epsilon);

    MPointArray controlPoints;

    status = controlPoints.append(startPoint);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = controlPoints.append(endPoint);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject splineDofCurve = fnSplineGenerator.createWithEditPoints(
        controlPoints, 1, MFnNurbsCurve::kOpen, false, true, true,
        MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    fnSplineGenerator.setName(ACCELERATION_VIOLATION_CUTOFF_LINE, false,
                              &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnSplineGenerator.setObjectColor(ACCELERATION_VIOLATION_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::redrawContactVisualizations()
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

MStatus FusedMotionEditContext::redrawMarkerVisualizations()
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

MStatus FusedMotionEditContext::visualizeAllContacts()
{
    MStatus status;

    MSelectionList selectionList;

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MString allContactsGroupName = CONTACT_GROUP_PREFIX + frameStringSuffix;

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

    MString objectContactGroupName =
        CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;

    MString handContactGroupName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

    status = visualizePatch(fnObjectMesh, objectContactGroupName,
                            CONTACT_POINTS_ATTRIBUTE, OBJECT_PATCH_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = visualizePatch(fnHandMesh, handContactGroupName,
                            CONTACT_POINTS_ATTRIBUTE, HAND_PATCH_COLOR);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::visualizeAllVirtualMarkers()
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

MStatus FusedMotionEditContext::visualizePatch(MFnMesh &fnMesh,
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

            status = createMeshVisualizationSphere(
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

MStatus FusedMotionEditContext::computePairedMarkerPatchLocations(
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

MStatus FusedMotionEditContext::computeTableSDF(MPoint &queryPoint,
                                                double &signedDistance)
{
    MStatus status;

    if (m_table_transform_inv[3][3] == -1)
    {
        MGlobal::displayError(
            "Error: Table does not exist - cannot compute SDF.");
        return MS::kFailure;
    }

    MPoint relativeQueryPoint = m_table_transform_inv * queryPoint;

    MVector absVec(abs(relativeQueryPoint.x), abs(relativeQueryPoint.y),
                   abs(relativeQueryPoint.z));

    MVector q = absVec - m_table_box_dims;

    MVector maxVec(max(q.x, 0.0), max(q.y, 0.0), max(q.z, 0.0));

    signedDistance = maxVec.length() + min(max(q.x, max(q.y, q.z)), 0.0);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::generateHandTestPoints(
    vector<pair<MFloatPoint, MFloatVector>> &handPoints)
{
    MStatus status;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPoint vPoint;
    MVector vNormal;

    handPoints.clear();

    for (int i = 0; i < fnHandMesh.numVertices(); i++)
    {
        status = fnHandMesh.getPoint(i, vPoint, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnHandMesh.getVertexNormal(i, false, vNormal, MSpace::kWorld);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFloatPoint srcP(vPoint);
        MFloatVector srcV(vNormal);

        pair<MFloatPoint, MFloatVector> rayPoint = make_pair(srcP, srcV);

        handPoints.push_back(rayPoint);
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::getMocapMarker(MString &mocapMarkerName,
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

MStatus
FusedMotionEditContext::getOmissionIndicesAttribute(MString &contactGroupName,
                                                    MIntArray &omissionIndices)
{
    MStatus status;

    MSelectionList selectionList;

    status = omissionIndices.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);

    if (status != MS::kSuccess)
    {
        MGlobal::displayInfo("Contact " + contactGroupName +
                             " not found - no omission indices available");
        return MS::kSuccess;
    }

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

MStatus FusedMotionEditContext::getPairedFrameContactPoints(
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

    MString frameStringSuffix = MString(to_string(m_frame).c_str());

    MString allContactsGroupName = CONTACT_GROUP_PREFIX + frameStringSuffix;

    MString objectContactGroupName =
        CONTACT_GROUP_PREFIX + objectShapeName + frameStringSuffix;

    MString handContactGroupName =
        CONTACT_GROUP_PREFIX + handShapeName + frameStringSuffix;

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

MStatus FusedMotionEditContext::getSetSerializedPointsAttribute(
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

MStatus FusedMotionEditContext::initializeDofSolution()
{
    MStatus status;

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        pair<int, int> indices = m_dof_vec_mappings.at(i);

        int jointIndex = indices.first;
        int dofIndex = indices.second;

        MDagPath joint = m_rig_joints[jointIndex];

        MFnIkJoint fnJoint(joint, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (dofIndex > 2) // indicates translation dof
        {
            MVector transVec = fnJoint.getTranslation(MSpace::kWorld, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_dof_vector[i] = transVec[dofIndex - 3];
        }
        else
        {
            double rotation[3];
            MTransformationMatrix::RotationOrder order;

            status = fnJoint.getRotation(rotation, order);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            m_dof_vector[i] = rotation[dofIndex];
        }

        MGlobal::displayInfo(MString(to_string(jointIndex).c_str()) + " " +
                             MString(to_string(dofIndex).c_str()) + " " +
                             MString(to_string(m_dof_vector[i]).c_str()));
    }

    return MS::kSuccess;
}

nlopt::opt FusedMotionEditContext::initializeOptimization()
{
    MStatus status;

    MGlobal::displayInfo("Initializing optimization....");

    nlopt::opt opt(nlopt::LD_MMA, m_rig_n_dofs);
    opt.set_min_objective(&FusedMotionEditContext::optimizerWrapper,
                          (void *)this);

    opt.set_xtol_rel(1e-4);
    opt.set_maxeval(m_num_opt_iterations);

    return opt;
}

MStatus FusedMotionEditContext::interpolateSerializedPoint(
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

MStatus FusedMotionEditContext::loadDofSolutionFull()
{
    MStatus status;

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        status = loadDofSolutionSingle(i);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::loadDofSolutionSingle(int index)
{
    MStatus status;

    double value = m_dof_vector[index];
    pair<int, int> indices = m_dof_vec_mappings.at(index);

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

MStatus FusedMotionEditContext::parseSerializedPoint(MFnMesh &fnMesh,
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

MStatus FusedMotionEditContext::runOptimization(nlopt::opt &optim)
{
    MStatus status;

    MGlobal::displayInfo("Running optimization, please wait....");

    double minf;
    vector<double> x(m_rig_n_dofs);

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        x[i] = m_dof_vector[i];
    }

    try
    {
        m_last_dof_vectors.push(MDoubleArray(m_dof_vector));

        nlopt::result result = optim.optimize(x, minf);

        for (int i = 0; i < m_rig_n_dofs; i++)
        {
            m_dof_vector[i] = x[i];
        }

        status = loadDofSolutionFull();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        string minfVal = to_string(minf);
        MGlobal::displayInfo(MString("Completed. Found minimum at: ") +
                             minfVal.c_str());
    }
    catch (exception &e)
    {
        MGlobal::displayInfo("NLOPT failed");
        MGlobal::displayInfo(e.what());
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setContactAttribute(
    MString &contactGroupName, MStringArray &serializedContactPoints)
{
    MStatus status;

    MSelectionList selectionList;
    status = MGlobal::getSelectionListByName(contactGroupName, selectionList);
    CHECK_MSTATUS_AND_RETURN_IT(status);

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
        MGlobal::displayInfo(CONTACT_POINTS_ATTRIBUTE +
                             " attribute not found for group " +
                             contactGroupName +
                             " - attribute MUST be present "
                             "for all contact sets");
        return MS::kFailure;
    }

    attrObj = fnContactSet.attribute(CONTACT_POINTS_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug attributePlug(contactSet, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedContactPoints, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::setSerializedViolationsAttribute(
    MStringArray &serializedFrameViolations)
{
    MStatus status;

    MSelectionList selectionList;

    MObject setObject;

    status = MGlobal::getSelectionListByName(ACCELERATION_ERROR_STORAGE_GROUP,
                                             selectionList);

    if (status != MS::kSuccess)
    {
        MFnSet storageGroupGenerator;

        setObject = storageGroupGenerator.create(
            selectionList, MFnSet::Restriction::kNone, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDependencyNode fnDepNode(setObject, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        fnDepNode.setName(ACCELERATION_ERROR_STORAGE_GROUP, false, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        status = selectionList.getDependNode(0, setObject);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MFnDependencyNode fnDepNode(setObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    bool hasAttribute =
        fnDepNode.hasAttribute(ACCELERATION_ERROR_STORAGE_ATTRIBUTE, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject attrObj;

    if (!hasAttribute)
    {
        MFnTypedAttribute fnTypedAttr;

        attrObj = fnTypedAttr.create(ACCELERATION_ERROR_STORAGE_ATTRIBUTE,
                                     ACCELERATION_ERROR_STORAGE_ATTRIBUTE,
                                     MFnData::kStringArray, MObject::kNullObj,
                                     &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = fnDepNode.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    else
    {
        attrObj =
            fnDepNode.attribute(ACCELERATION_ERROR_STORAGE_ATTRIBUTE, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attributePlug(setObject, attrObj);

    MFnStringArrayData fnStringArrayData;

    MObject stringArrayData =
        fnStringArrayData.create(serializedFrameViolations, &status);

    status = attributePlug.setMObject(stringArrayData);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::storeExistingFrameSolutions(int frameStart,
                                                            int frameEnd)
{
    MStatus status;

    MAnimControl animCtrl;

    m_existing_frame_solutions.clear();

    for (int frame = frameStart; frame <= frameEnd; frame++)
    {
        MTime newFrame((double)frame, m_framerate);
        status = animCtrl.setCurrentTime(newFrame);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = initializeDofSolution();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_existing_frame_solutions[frame] = MDoubleArray(m_dof_vector);
    }

    // Add extras in case of using somewhere in middle

    int endFrame = frameEnd;
    int startPriorFrame = frameStart - 1;
    int postEndFrame = frameEnd + 1;

    int extraFrameIndices[3] = {endFrame, startPriorFrame, postEndFrame};

    for (int extraFrameIndex = 0; extraFrameIndex < 3; extraFrameIndex++)
    {
        int extraFrame = extraFrameIndices[extraFrameIndex];

        MTime newFrame((double)extraFrame, m_framerate);
        status = animCtrl.setCurrentTime(newFrame);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = initializeDofSolution();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        m_existing_frame_solutions[extraFrame] = MDoubleArray(m_dof_vector);
    }

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::wipeContactPairingLines()
{
    MStatus status;

    status = wipePairingLines(CONTACT_PAIRING_LINES_GROUP);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::wipeJointKeyframe(MString &jointName,
                                                  int frameStart, int frameEnd)
{
    MStatus status;

    MString animFrameStartStr = to_string(frameStart).c_str();
    MString animFrameEndStr = to_string(frameEnd).c_str();

    MString clearCommand = "cutKey -clear -time \"" + animFrameStartStr + ":" +
                           animFrameEndStr + "\" -option keys {\" " +
                           jointName + " \"}";
    status = MGlobal::executeCommand(clearCommand);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::wipeMarkerPairingLines()
{
    MStatus status;

    status = wipePairingLines(MARKER_PAIRING_LINES_GROUP);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContext::wipePairingLines(MString prefix)
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

MStatus FusedMotionEditContext::clearVisualizations(MString filterString)
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

// NLOPT Utils

double FusedMotionEditContext::computeDofGradient(
    const MDoubleArray &existingDofs, double step, double currentObjectiveValue,
    int dofVectorIndex)
{
    MStatus status;

    double initVal = m_dof_vector[dofVectorIndex];

    m_dof_vector[dofVectorIndex] += step;

    status = loadDofSolutionSingle(dofVectorIndex);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    double forwardObjective = computeObjective(existingDofs, true);
    double JthetaDof = (forwardObjective - currentObjectiveValue) / step;

    m_dof_vector[dofVectorIndex] = initVal;

    status = loadDofSolutionSingle(dofVectorIndex);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return JthetaDof;
}

double
FusedMotionEditContext::computeObjective(const MDoubleArray &existingDofs,
                                         bool suppressVisualization)
{
    MStatus status;

    bool visualizationEnabled =
        !suppressVisualization && m_optimization_visualization_enabled;

    double contactDistanceError = 0.0;
    double contactNormalError = 0.0;

    double contactError = 0.0;
    double markerError = 0.0;
    double intersectionError = 0.0;
    double priorError = 0.0;

    MFnMesh fnHandMesh(m_hand_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnObjectMesh(m_object_geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Step 1: Load new candidate and compute prior error

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        // Do not include base movement in prior error
        if (i > 5)
        {
            priorError += abs(existingDofs[i] - m_dof_vector[i]);
        }
    }

    // To visualize progress
    if (visualizationEnabled)
    {
        m_view.refresh(false, true);
    }

    // Step 2: Compute contact point-to-point distance and normal error

    vector<MPointArray> pairedPointLocations;
    vector<MVectorArray> pairedPointNormals;

    status =
        getPairedFrameContactPoints(pairedPointLocations, pairedPointNormals);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numContactPoints = pairedPointLocations.size();

    for (int i = 0; i < numContactPoints; i++)
    {
        MPointArray locationPair = pairedPointLocations[i];
        MVectorArray normalPair = pairedPointNormals[i];

        MPoint objectPoint = locationPair[0];
        MVector objectNormal = normalPair[0];

        MPoint handPoint = locationPair[1];
        MVector handNormal = normalPair[1];

        // Distance error

        double distance = objectPoint.distanceTo(handPoint);
        contactDistanceError += distance;

        // Normal error

        double normalAntiAlignment = 1 + (objectNormal * handNormal);
        contactNormalError += normalAntiAlignment;
    }

    double weightedContactDistanceError =
        m_contact_distance_penalty_coefficient * contactDistanceError;
    double weightedContactNormalError =
        m_contact_normal_penalty_coefficient * contactNormalError;

    contactError = weightedContactDistanceError + weightedContactNormalError;

    // Step 3: Compute marker point-to-marker distance error

    vector<MPointArray> pointLocations;

    status = computePairedMarkerPatchLocations(pointLocations);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numMarkerPoints = pointLocations.size();

    for (int i = 0; i < numMarkerPoints; i++)
    {
        MPointArray pointLocationPair = pointLocations[i];

        MPoint p1 = pointLocationPair[0];
        MPoint p2 = pointLocationPair[1];

        double distance = p1.distanceTo(p2);
        markerError += distance;
    }

    // Step 4: Compute table intersection error
    if (m_intersection_penalty_coefficient > 0.0)
    {
        MItMeshVertex handVertexItList(m_hand_geometry);

        double signedDistance;

        for (; !handVertexItList.isDone(); handVertexItList.next())
        {
            MPoint queryPoint =
                handVertexItList.position(MSpace::kWorld, &status);
            CHECK_MSTATUS(status);

            status = computeTableSDF(queryPoint, signedDistance);
            CHECK_MSTATUS(status);

            double clampedDistance = min(signedDistance, 0.0);
            intersectionError += -1.0 * clampedDistance;
        }
    }

    // Step 5: Compute total weighted errors

    double weightedMarkerError = m_marker_penalty_coefficient * markerError;
    double weightedContactError = m_contact_penalty_coefficient * contactError;
    double weightedIntersectionError =
        m_intersection_penalty_coefficient * intersectionError;
    double weightedPriorError = m_prior_penalty_coefficient * priorError;

    double objValue = weightedMarkerError + weightedContactError +
                      weightedIntersectionError + weightedPriorError;

    if (visualizationEnabled)
    {
        MGlobal::displayInfo(
            "Marker: " + MString(to_string(weightedMarkerError).c_str()) +
            " Contact: " + MString(to_string(weightedContactError).c_str()) +
            " Intersection: " +
            MString(to_string(weightedIntersectionError).c_str()) +
            " Prior: " + MString(to_string(weightedPriorError).c_str()));
    }

    return objValue;
}

double FusedMotionEditContext::computeOptimization(const vector<double> &x,
                                                   vector<double> &grad)
{
    MStatus status;

    MDoubleArray existingDofs = MDoubleArray(m_dof_vector);

    for (int i = 0; i < m_rig_n_dofs; i++)
    {
        m_dof_vector[i] = x[i];
    }

    status = loadDofSolutionFull();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    double currentObj = computeObjective(existingDofs);

    if (grad.size() > 0)
    {
        double step = 0.001;

        // EXPENSIVE!! Currently uses forward differencing
        for (int i = 0; i < m_rig_n_dofs; i++)
        {
            grad[i] = computeDofGradient(existingDofs, step, currentObj, i);
        }
    }
    else
    {
        cout << "PROBLEM: grad size <= 0" << endl;
    }

    m_dof_vector = existingDofs;

    status = loadDofSolutionFull();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return currentObj;
}

double FusedMotionEditContext::optimizerWrapper(const vector<double> &x,
                                                vector<double> &grad,
                                                void *data)
{
    FusedMotionEditContext *pContext = (FusedMotionEditContext *)data;
    if (!pContext)
    {
        return numeric_limits<double>::quiet_NaN();
    }

    return pContext->computeOptimization(x, grad);
}
