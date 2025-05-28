#include "fusedMotionEditContextCommand.hpp"

FusedMotionEditContextCommand::FusedMotionEditContextCommand()
{
    m_pContext = NULL;
}

FusedMotionEditContextCommand::~FusedMotionEditContextCommand() {}

void *FusedMotionEditContextCommand::creator()
{
    return new FusedMotionEditContextCommand();
}

MPxContext *FusedMotionEditContextCommand::makeObj()
{
    m_pContext = new FusedMotionEditContext();
    return m_pContext;
}

MStatus FusedMotionEditContextCommand::appendSyntax()
{
    MStatus status;
    MSyntax mSyntax = syntax();

    status = mSyntax.addFlag(CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG,
                             CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG_LONG,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG,
                             CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG_LONG,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        mSyntax.addFlag(MARKER_PENALTY_COEFFICIENT_FLAG,
                        MARKER_PENALTY_COEFFICIENT_FLAG_LONG, MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(CONTACT_PENALTY_COEFFICIENT_FLAG,
                             CONTACT_PENALTY_COEFFICIENT_FLAG_LONG,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(INTERSECTION_PENALTY_COEFFICIENT_FLAG,
                             INTERSECTION_PENALTY_COEFFICIENT_FLAG_LONG,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        mSyntax.addFlag(PRIOR_PENALTY_COEFFICIENT_FLAG,
                        PRIOR_PENALTY_COEFFICIENT_FLAG_LONG, MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(NUM_OPT_ITERATIONS_FLAG,
                             NUM_OPT_ITERATIONS_FLAG_LONG, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        mSyntax.addFlag(OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG,
                        OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG_LONG,
                        MSyntax::kBoolean);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(RESET_JOINTS_FLAG, RESET_JOINTS_FLAG_LONG,
                             MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(UNDO_OPTIMIZATION_FLAG,
                             UNDO_OPTIMIZATION_FLAG_LONG, MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(JUMP_FLAG, JUMP_FLAG_LONG, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(KEYFRAME_RIG_FLAG, KEYFRAME_RIG_FLAG_LONG,
                             MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(KEYFRAME_RIG_BULK_FLAG,
                             KEYFRAME_RIG_BULK_FLAG_LONG, MSyntax::kUnsigned,
                             MSyntax::kUnsigned, MSyntax::kBoolean);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        mSyntax.addFlag(WIPE_RIG_KEYFRAME_FLAG, WIPE_RIG_KEYFRAME_FLAG_LONG,
                        MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(FINALIZE_OMISSION_INDICES_FLAG,
                             FINALIZE_OMISSION_INDICES_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(COMPUTE_ACCELERATION_ERRORS_FLAG,
                             COMPUTE_ACCELERATION_ERRORS_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(ACCELERATION_EPSILON_FLAG,
                             ACCELERATION_EPSILON_FLAG_LONG, MSyntax::kUnsigned,
                             MSyntax::kUnsigned, MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(
        RESOLVE_ACCELERATION_ERRORS_FLAG, RESOLVE_ACCELERATION_ERRORS_FLAG_LONG,
        MSyntax::kUnsigned, MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status =
        mSyntax.addFlag(STORE_ACCELERATION_ERRORS_FLAG,
                        STORE_ACCELERATION_ERRORS_FLAG_LONG, MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus FusedMotionEditContextCommand::doEditFlags()
{
    MStatus status;
    MArgParser argData = parser();

    if (argData.isFlagSet(CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG))
    {
        double distancePenaltyCoeff = argData.flagArgumentDouble(
            CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setCoefficientContactDistanceError(
            distancePenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG))
    {
        double normalPenaltyCoeff = argData.flagArgumentDouble(
            CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status =
            m_pContext->setCoefficientContactNormalError(normalPenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(MARKER_PENALTY_COEFFICIENT_FLAG))
    {
        double markerPenaltyCoeff = argData.flagArgumentDouble(
            MARKER_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setCoefficientMarkerError(markerPenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(CONTACT_PENALTY_COEFFICIENT_FLAG))
    {
        double contactPenaltyCoeff = argData.flagArgumentDouble(
            CONTACT_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setCoefficientContactError(contactPenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(INTERSECTION_PENALTY_COEFFICIENT_FLAG))
    {
        double intersectionPenaltyCoeff = argData.flagArgumentDouble(
            INTERSECTION_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setCoefficientIntersectionError(
            intersectionPenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(PRIOR_PENALTY_COEFFICIENT_FLAG))
    {
        double priorPenaltyCoeff = argData.flagArgumentDouble(
            PRIOR_PENALTY_COEFFICIENT_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setCoefficientPriorError(priorPenaltyCoeff);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(NUM_OPT_ITERATIONS_FLAG))
    {
        int numOptIterations =
            argData.flagArgumentInt(NUM_OPT_ITERATIONS_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setNumOptIterations(numOptIterations);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG))
    {
        bool enable = argData.flagArgumentBool(
            OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->enableOptimizationProgressVisualization(enable);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(RESET_JOINTS_FLAG))
    {
        status = m_pContext->resetNonRootJoints();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(UNDO_OPTIMIZATION_FLAG))
    {
        status = m_pContext->undoOptimization();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(JUMP_FLAG))
    {
        int frame = argData.flagArgumentInt(JUMP_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->jumpToFrame(frame);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(KEYFRAME_RIG_FLAG))
    {
        status = m_pContext->keyframeRig();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(KEYFRAME_RIG_BULK_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(KEYFRAME_RIG_BULK_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(KEYFRAME_RIG_BULK_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        bool saveKeysOnly =
            argData.flagArgumentBool(KEYFRAME_RIG_BULK_FLAG, 2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status =
            m_pContext->keyframeRigBulk(frameStart, frameEnd, saveKeysOnly);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(WIPE_RIG_KEYFRAME_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(WIPE_RIG_KEYFRAME_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(WIPE_RIG_KEYFRAME_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->wipeRigKeyframe(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(FINALIZE_OMISSION_INDICES_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(FINALIZE_OMISSION_INDICES_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(FINALIZE_OMISSION_INDICES_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->finalizeOmissionIndices(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(COMPUTE_ACCELERATION_ERRORS_FLAG))
    {
        int frameStart = argData.flagArgumentInt(
            COMPUTE_ACCELERATION_ERRORS_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(COMPUTE_ACCELERATION_ERRORS_FLAG,
                                               1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->computeAccelerationErrors(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(ACCELERATION_EPSILON_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(ACCELERATION_EPSILON_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(ACCELERATION_EPSILON_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        double epsilon =
            argData.flagArgumentDouble(ACCELERATION_EPSILON_FLAG, 2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setAccelerationViolationEpsilon(frameStart,
                                                             frameEnd, epsilon);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(RESOLVE_ACCELERATION_ERRORS_FLAG))
    {
        int frameStart = argData.flagArgumentInt(
            RESOLVE_ACCELERATION_ERRORS_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(RESOLVE_ACCELERATION_ERRORS_FLAG,
                                               1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int maxIterations = argData.flagArgumentInt(
            RESOLVE_ACCELERATION_ERRORS_FLAG, 2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->resolveAccelerationErrors(frameStart, frameEnd,
                                                       maxIterations);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(STORE_ACCELERATION_ERRORS_FLAG))
    {
        status = m_pContext->storeAccelerationErrors();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}
