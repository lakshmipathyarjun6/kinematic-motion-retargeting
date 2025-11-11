#include "contactRaytraceContextCommand.hpp"

ContactRaytraceContextCommand::ContactRaytraceContextCommand()
{
    m_pContext = NULL;
}

ContactRaytraceContextCommand::~ContactRaytraceContextCommand() {}

void *ContactRaytraceContextCommand::creator()
{
    return new ContactRaytraceContextCommand();
}

MPxContext *ContactRaytraceContextCommand::makeObj()
{
    m_pContext = new ContactRaytraceContext();
    return m_pContext;
}

MStatus ContactRaytraceContextCommand::appendSyntax()
{
    MStatus status;
    MSyntax mSyntax = syntax();

    status = mSyntax.addFlag(DUMP_CONTACT_DISTANCES_FLAG,
                             DUMP_CONTACT_DISTANCES_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(JUMP_FLAG, JUMP_FLAG_LONG, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(PURGE_CONTACTS_FLAG, PURGE_CONTACTS_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned,
                             MSyntax::kDouble);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(RAYTRACE_CONTACTS_FLAG,
                             RAYTRACE_CONTACTS_FLAG_LONG, MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(RAYTRACE_CONTACTS_BULK_FLAG,
                             RAYTRACE_CONTACTS_BULK_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus ContactRaytraceContextCommand::doEditFlags()
{
    MStatus status;
    MArgParser argData = parser();

    if (argData.isFlagSet(DUMP_CONTACT_DISTANCES_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(DUMP_CONTACT_DISTANCES_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(DUMP_CONTACT_DISTANCES_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->dumpContactDistances(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(JUMP_FLAG))
    {
        int frame = argData.flagArgumentInt(JUMP_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->jumpToFrame(frame);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(PURGE_CONTACTS_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(PURGE_CONTACTS_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(PURGE_CONTACTS_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        double cutoffDistance =
            argData.flagArgumentDouble(PURGE_CONTACTS_FLAG, 2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->purgeContactsAboveCutoff(frameStart, frameEnd,
                                                      cutoffDistance);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(RAYTRACE_CONTACTS_FLAG))
    {
        status = m_pContext->raytraceContacts();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(RAYTRACE_CONTACTS_BULK_FLAG))
    {
        int frameStart =
            argData.flagArgumentInt(RAYTRACE_CONTACTS_BULK_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd =
            argData.flagArgumentInt(RAYTRACE_CONTACTS_BULK_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->raytraceContactsBulk(frameStart, frameEnd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}
