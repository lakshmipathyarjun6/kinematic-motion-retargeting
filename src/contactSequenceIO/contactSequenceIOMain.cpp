#include <maya/MFnPlugin.h>

#include "contactSequenceIO.hpp"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Arjun Lakshmipathy - Meta Platforms Inc.", "1.0",
                       "Meta Platforms Inc.");

    status = fnPlugin.registerFileTranslator("json", "none",
                                             ContactSequenceIO::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterFileTranslator("json");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
