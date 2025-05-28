#ifndef FUSEDMOTIONEDITCONTEXTCOMMAND_H
#define FUSEDMOTIONEDITCONTEXTCOMMAND_H

#include <maya/MPxContextCommand.h>

#include "fusedMotionEditContext.hpp"

// Contact Optimization flags

#define CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG "-cdp"
#define CONTACT_DISTANCE_PENALTY_COEFFICIENT_FLAG_LONG "-cdpenalty"

#define CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG "-cnp"
#define CONTACT_NORMAL_PENALTY_COEFFICIENT_FLAG_LONG "-cnpenalty"

// Fused Optimization flags

#define CONTACT_PENALTY_COEFFICIENT_FLAG "-cmp"
#define CONTACT_PENALTY_COEFFICIENT_FLAG_LONG "-cmpenalty"

#define MARKER_PENALTY_COEFFICIENT_FLAG "-mmp"
#define MARKER_PENALTY_COEFFICIENT_FLAG_LONG "-mmpenalty"

#define INTERSECTION_PENALTY_COEFFICIENT_FLAG "-ip"
#define INTERSECTION_PENALTY_COEFFICIENT_FLAG_LONG "-ipenalty"

#define PRIOR_PENALTY_COEFFICIENT_FLAG "-pp"
#define PRIOR_PENALTY_COEFFICIENT_FLAG_LONG "-ppenalty"

#define NUM_OPT_ITERATIONS_FLAG "-ni"
#define NUM_OPT_ITERATIONS_FLAG_LONG "-numiterations"

#define OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG "-pve"
#define OPTIMIZATION_PROGRESS_VISUALIZATION_ENABLED_FLAG_LONG "-progvisenabled"

#define RESET_JOINTS_FLAG "-rj"
#define RESET_JOINTS_FLAG_LONG "-resetjoints"

#define UNDO_OPTIMIZATION_FLAG "-uo"
#define UNDO_OPTIMIZATION_FLAG_LONG "-undoopt"

// Animation flags

#define JUMP_FLAG "-j"
#define JUMP_FLAG_LONG "-jump"

#define KEYFRAME_RIG_FLAG "-kr"
#define KEYFRAME_RIG_FLAG_LONG "-keyframerig"

#define KEYFRAME_RIG_BULK_FLAG "-bkr"
#define KEYFRAME_RIG_BULK_FLAG_LONG "-bulkkeyframerig"

#define WIPE_RIG_KEYFRAME_FLAG "-wrk"
#define WIPE_RIG_KEYFRAME_FLAG_LONG "-wiperigkeyframe"

#define FINALIZE_OMISSION_INDICES_FLAG "-foi"
#define FINALIZE_OMISSION_INDICES_FLAG_LONG "-finalizeomissionindices"

// Refinement flags

#define COMPUTE_ACCELERATION_ERRORS_FLAG "-ca"
#define COMPUTE_ACCELERATION_ERRORS_FLAG_LONG "-computeaccelerationerrors"

#define ACCELERATION_EPSILON_FLAG "-ae"
#define ACCELERATION_EPSILON_FLAG_LONG "-accelerationepsilon"

#define RESOLVE_ACCELERATION_ERRORS_FLAG "-ra"
#define RESOLVE_ACCELERATION_ERRORS_FLAG_LONG "-resolveaccelerationerrors"

#define STORE_ACCELERATION_ERRORS_FLAG "-sa"
#define STORE_ACCELERATION_ERRORS_FLAG_LONG "-storeaccelerationerrors"

class FusedMotionEditContextCommand : public MPxContextCommand
{
public:
    FusedMotionEditContextCommand();
    virtual ~FusedMotionEditContextCommand();
    static void *creator();

    virtual MStatus appendSyntax();
    virtual MStatus doEditFlags();
    virtual MPxContext *makeObj();

protected:
    FusedMotionEditContext *m_pContext;
};

#endif
