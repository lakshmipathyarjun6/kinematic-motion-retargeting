#ifndef KEYFRAMEMOTIONSEQUENCEIO_H
#define KEYFRAMEMOTIONSEQUENCEIO_H

#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>

#include "JSONUtils.hpp"

#include <stack>
#include <vector>

#define OBJECT_NAME MString("object")
#define TABLE_NAME MString("table")

#define USE_QUATS

using namespace std;

class KeyframeMotionSequenceIO : public MPxFileTranslator
{
public:
    KeyframeMotionSequenceIO();

    virtual ~KeyframeMotionSequenceIO();

    static void *creator();

    virtual bool canBeOpened() const;

    virtual MString defaultExtension() const;

    MStatus exportAll(string &filename);

    virtual bool haveReadMethod() const;

    virtual bool haveWriteMethod() const;

    MPxFileTranslator::MFileKind identifyFile(const MFileObject &fileName,
                                              const char *buffer,
                                              short size) const;

    virtual MStatus writer(const MFileObject &file, const MString &optionString,
                           FileAccessMode mode);

protected:
    MStatus loadDofSolutionFull();

    MStatus loadDofSolutionSingle(int index);

    MStatus parseKinematicTree();

    // Hand kinematic vars

    int m_rig_n_dofs;
    MDagPath m_rig_base;
    MDagPathArray m_rig_joints;
    MDoubleArray m_dof_vector;
    MStringArray m_joint_names;
    MIntArray m_joint_num_dofs;
    vector<pair<int, int>> m_dof_vec_mappings;
};

#endif // KEYFRAMEMOTIONSEQUENCEIO_H
