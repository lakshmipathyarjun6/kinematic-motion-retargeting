#include "contactTransferEditContextCommand.hpp"

ContactTransferEditContextCommand::ContactTransferEditContextCommand()
{
    m_pContext = NULL;
}

ContactTransferEditContextCommand::~ContactTransferEditContextCommand() {}

void *ContactTransferEditContextCommand::creator()
{
    return new ContactTransferEditContextCommand();
}

MPxContext *ContactTransferEditContextCommand::makeObj()
{
    m_pContext = new ContactTransferEditContext();
    return m_pContext;
}

MStatus ContactTransferEditContextCommand::appendSyntax()
{
    MStatus status;
    MSyntax mSyntax = syntax();

    status = mSyntax.addFlag(JUMP_FLAG, JUMP_FLAG_LONG, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(TRANSFER_CONTACTS_FLAG,
                             TRANSFER_CONTACTS_FLAG_LONG, MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(TRANSFER_CONTACTS_BULK_FLAG,
                             TRANSFER_CONTACTS_BULK_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(COMMIT_PARAMETERIZED_DISTANCES_FLAG,
                             COMMIT_PARAMETERIZED_DISTANCES_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(DUMP_PARAMETERIZED_DISTANCES_FLAG,
                             DUMP_PARAMETERIZED_DISTANCES_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(PARAMETERIZED_DISTANCE_THRESHOLD_FLAG,
                             PARAMETERIZED_DISTANCE_THRESHOLD_FLAG_LONG,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactTransferEditContextCommand::doEditFlags()
{
    MStatus status;
    MArgParser argData = parser();

    if (argData.isFlagSet(JUMP_FLAG))
    {
        int frame = argData.flagArgumentInt(JUMP_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->jumpToFrame(frame);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(TRANSFER_CONTACTS_FLAG))
    {
        status = m_pContext->transferContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(TRANSFER_CONTACTS_BULK_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(TRANSFER_CONTACTS_BULK_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(TRANSFER_CONTACTS_BULK_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->transferContactsBulk(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(COMMIT_PARAMETERIZED_DISTANCES_FLAG))
    {
        int frameStart = argData.flagArgumentInt(
            COMMIT_PARAMETERIZED_DISTANCES_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(
            COMMIT_PARAMETERIZED_DISTANCES_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        double parameterizedFilterDistanceThreshold =
            argData.flagArgumentDouble(COMMIT_PARAMETERIZED_DISTANCES_FLAG, 2,
                                       &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->commitParameterizedDistancesFilter(
            frameStart, frameEnd, parameterizedFilterDistanceThreshold);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(DUMP_PARAMETERIZED_DISTANCES_FLAG))
    {
        int frameStart = argData.flagArgumentInt(
            DUMP_PARAMETERIZED_DISTANCES_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(
            DUMP_PARAMETERIZED_DISTANCES_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->dumpParameterizedDistances(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(PARAMETERIZED_DISTANCE_THRESHOLD_FLAG))
    {
        double parameterizedFilterDistanceThreshold =
            argData.flagArgumentDouble(PARAMETERIZED_DISTANCE_THRESHOLD_FLAG, 0,
                                       &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->setContactParameterizedFilterDistanceThreshold(
            parameterizedFilterDistanceThreshold);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}
