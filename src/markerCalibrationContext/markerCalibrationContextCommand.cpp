#include "markerCalibrationContextCommand.hpp"

MarkerCalibrationContextCommand::MarkerCalibrationContextCommand()
{
    m_pContext = NULL;
}

MarkerCalibrationContextCommand::~MarkerCalibrationContextCommand() {}

void *MarkerCalibrationContextCommand::creator()
{
    return new MarkerCalibrationContextCommand();
}

MPxContext *MarkerCalibrationContextCommand::makeObj()
{
    m_pContext = new MarkerCalibrationContext();
    return m_pContext;
}
