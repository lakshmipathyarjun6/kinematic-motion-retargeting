#ifndef GEOMETRYPROCESSINGCONTEXT_H
#define GEOMETRYPROCESSINGCONTEXT_H

#include <maya/MDagPath.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>

#include "geometrycentral/surface/exact_geodesics.h"
#include "geometrycentral/surface/manifold_surface_mesh.h"
#include "geometrycentral/surface/meshio.h"
#include "geometrycentral/surface/surface_curve.h"
#include "geometrycentral/surface/surface_patch.h"
#include "geometrycentral/surface/surface_point.h"
#include "geometrycentral/surface/vector_heat_method.h"

#include <sstream>
#include <vector>

using namespace geometrycentral;
using namespace geometrycentral::surface;

using namespace std;

// Not a "true" Maya context - subcontext used to integrate geometry processing
// algorithms and representations
class GeometryProcessingContext
{
public:
    GeometryProcessingContext();
    virtual ~GeometryProcessingContext();

    MStatus createContact(MString &axisName, MString &contactName,
                          MStringArray &serializedContactPoints);
    MStatus getAxis(MString &axisName, MStringArray &serializedAxisPoints);
    MStatus getReassembledContactsFromAxisGroups(
        MStringArray &serializedContactPoints,
        MDoubleArray &contactPointParameterizedDistances);
    MStatus initializeLandmarkParameterization();
    MStatus registerAxis(MString &axisName, MStringArray &serializedAxisPoints);
    MStatus registerGeometry(MDagPath &geometry, MDagPath &transform);
    MStatus
    parameterizeAllContactsFromLandmarks(MStringArray &serializedContactPoints);
    MStatus parseSerializedPoint(MString &serializedSurfacePoint,
                                 vector<int> &vertexIndices,
                                 vector<double> &coords);
    MStatus
    transferAllContacts(GeometryProcessingContext *targetContext,
                        map<string, MString> &sourceTargetAxisNameMappings,
                        map<string, double> &targetSpreadCoefficients);

private:
    MStatus createContactFromExplicitClosestPointBindings(
        MString &axisName, MStringArray &serializedContactPoints,
        map<size_t, size_t> &closestPointBindings);
    SurfacePoint deserializeSurfacePoint(MString &serializedSurfacePoint);
    int findClosestLandmarkIndexToPoint(SurfacePoint &surfacePoint);
    MString serializeSurfacePoint(SurfacePoint &surfacePoint);
    MStatus verifyAxisExistence(MString &axisName);
    MStatus verifyContactExistence(MString &contactName);

    unique_ptr<ManifoldSurfaceMesh> m_mesh;
    unique_ptr<VertexPositionGeometry> m_geometry;

    unique_ptr<GeodesicAlgorithmExact> m_mmp_solver;
    unique_ptr<VectorHeatMethodSolver> m_vector_heat_solver;

    map<string, SurfaceCurve *> m_axis_curves;
    map<string, SurfacePatch *> m_contact_patches;

    vector<tuple<SurfacePoint, double>> m_landmark_source_data;
    MStringArray m_landmark_axis_names;
    MIntArray m_axis_point_indices;

    VertexData<double> m_landmark_scalar_extensions;

    vector<pair<string, int>> m_contact_point_axis_group_mapping;
};

#endif // GEOMETRYPROCESSINGCONTEXT_H
