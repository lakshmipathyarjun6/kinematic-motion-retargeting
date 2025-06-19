#ifndef SMOOTHMOTIONEDITCONTEXTCOMMAND_H
#define SMOOTHMOTIONEDITCONTEXTCOMMAND_H

#include <maya/MPxContextCommand.h>

#include "smoothMotionEditContext.hpp"

// Spline Creation flags

#define FIT_SPLINES_FLAG "-fs"
#define FIT_SPLINES_FLAG_LONG "-fitsplines"

// Animation flags

#define JUMP_FLAG "-j"
#define JUMP_FLAG_LONG "-jump"

#define LOAD_SPLINES_FLAG "-ls"
#define LOAD_SPLINES_FLAG_LONG "-loadsplines"

class SmoothMotionEditContextCommand : public MPxContextCommand
{
public:
    SmoothMotionEditContextCommand();
    virtual ~SmoothMotionEditContextCommand();
    static void *creator();

    virtual MStatus appendSyntax();
    virtual MStatus doEditFlags();
    virtual MPxContext *makeObj();

protected:
    SmoothMotionEditContext *m_pContext;
};

#endif
