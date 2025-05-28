#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "fusedMotionEditContextCommand.hpp"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Arjun Lakshmipathy - Meta Platforms Inc.", "1.0",
                       "Meta Platforms Inc.");

    status = fnPlugin.registerContextCommand(
        "fusedMotionEditContext", FusedMotionEditContextCommand::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::executeCommand("fusedMotionEditContext fmec;");

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterContextCommand("fusedMotionEditContext");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
