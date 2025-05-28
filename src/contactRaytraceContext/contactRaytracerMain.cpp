#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "contactRaytraceContextCommand.hpp"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Arjun Lakshmipathy - Meta Platforms Inc.", "1.0",
                       "Meta Platforms Inc.");

    status = fnPlugin.registerContextCommand(
        "contactRaytraceContext", ContactRaytraceContextCommand::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::executeCommand("contactRaytraceContext crc;");

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterContextCommand("contactRaytraceContext");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
