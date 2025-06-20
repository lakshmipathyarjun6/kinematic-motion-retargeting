#include <maya/MFnPlugin.h>

#include "virtualMarkerIO.hpp"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Arjun Lakshmipathy - Meta Platforms Inc.", "1.0",
                       "Meta Platforms Inc.");

    status = fnPlugin.registerFileTranslator("virtualmarkers", "none",
                                             VirtualMarkerIO::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterFileTranslator("virtualmarkers");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
