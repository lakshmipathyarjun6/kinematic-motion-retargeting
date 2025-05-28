#ifndef MARKERCALIBRATIONCONTEXTCOMMAND_H
#define MARKERCALIBRATIONCONTEXTCOMMAND_H

#include <maya/MPxContextCommand.h>

#include "markerCalibrationContext.hpp"

class MarkerCalibrationContextCommand : public MPxContextCommand
{
public:
    MarkerCalibrationContextCommand();
    virtual ~MarkerCalibrationContextCommand();
    static void *creator();

    virtual MPxContext *makeObj();

protected:
    MarkerCalibrationContext *m_pContext;
};

#endif
