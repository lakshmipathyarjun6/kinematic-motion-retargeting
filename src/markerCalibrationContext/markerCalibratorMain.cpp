#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "markerCalibrationContextCommand.hpp"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Arjun S. Lakshmipathy", "1.0",
                       "Carnegie Mellon University");

    status = fnPlugin.registerContextCommand(
        "markerCalibrationContext", MarkerCalibrationContextCommand::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MGlobal::executeCommand("markerCalibrationContext mcc;");

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterContextCommand("markerCalibrationContext");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}
