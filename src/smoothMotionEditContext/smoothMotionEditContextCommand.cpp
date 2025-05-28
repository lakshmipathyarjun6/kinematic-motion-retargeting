#include "smoothMotionEditContextCommand.hpp"

SmoothMotionEditContextCommand::SmoothMotionEditContextCommand()
{
    m_pContext = NULL;
}

SmoothMotionEditContextCommand::~SmoothMotionEditContextCommand() {}

void *SmoothMotionEditContextCommand::creator()
{
    return new SmoothMotionEditContextCommand();
}

MPxContext *SmoothMotionEditContextCommand::makeObj()
{
    m_pContext = new SmoothMotionEditContext();
    return m_pContext;
}

MStatus SmoothMotionEditContextCommand::appendSyntax()
{
    MStatus status;
    MSyntax mSyntax = syntax();

    status = mSyntax.addFlag(OBJECT_FIT_ENABLED_FLAG,
                             OBJECT_FIT_ENABLED_FLAG_LONG, MSyntax::kBoolean);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(FIT_SPLINES_FLAG, FIT_SPLINES_FLAG_LONG,
                             MSyntax::kUnsigned, MSyntax::kUnsigned,
                             MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(JUMP_FLAG, JUMP_FLAG_LONG, MSyntax::kUnsigned);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = mSyntax.addFlag(LOAD_SPLINES_FLAG, LOAD_SPLINES_FLAG_LONG,
                             MSyntax::kNoArg);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus SmoothMotionEditContextCommand::doEditFlags()
{
    MStatus status;
    MArgParser argData = parser();

    if (argData.isFlagSet(OBJECT_FIT_ENABLED_FLAG))
    {
        bool enable =
            argData.flagArgumentBool(OBJECT_FIT_ENABLED_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->enableObjectFit(enable);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(FIT_SPLINES_FLAG))
    {
        int frameStart = argData.flagArgumentInt(FIT_SPLINES_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int frameEnd = argData.flagArgumentInt(FIT_SPLINES_FLAG, 1, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int numControlPoints =
            argData.flagArgumentInt(FIT_SPLINES_FLAG, 2, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->fitSplines(frameStart, frameEnd, numControlPoints);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(JUMP_FLAG))
    {
        int frame = argData.flagArgumentInt(JUMP_FLAG, 0, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = m_pContext->jumpToFrame(frame);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(LOAD_SPLINES_FLAG))
    {
        status = m_pContext->loadControlSplines();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}
