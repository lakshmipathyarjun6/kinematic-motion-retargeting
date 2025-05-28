#include "geometryProcessingContext.hpp"

GeometryProcessingContext::GeometryProcessingContext() {}

GeometryProcessingContext::~GeometryProcessingContext() {}

MStatus
GeometryProcessingContext::createContact(MString &axisName,
                                         MString &contactName,
                                         MStringArray &serializedContactPoints)
{
    MStatus status;

    status = verifyAxisExistence(axisName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string axisNameChar = axisName.asChar();
    string contactNameChar = contactName.asChar();

    if (!m_contact_patches.contains(contactNameChar))
    {
        m_contact_patches[contactNameChar] =
            new SurfacePatch(m_mesh.get(), m_geometry.get(), m_mmp_solver.get(),
                             m_vector_heat_solver.get());

        // Axis does not change, so generate only once
        vector<SurfacePoint> axisPoints;
        vector<CurvePointParams> parameterizedAxisPoints;

        m_axis_curves[axisNameChar]->getPoints(axisPoints);
        m_axis_curves[axisNameChar]->getParameterized(parameterizedAxisPoints);

        int numAxisPoints = axisPoints.size();

        vector<CurvePointParams> convertedParameterizedAxisPoints(
            numAxisPoints);

        CurvePointParams cp;

        for (int i = 0; i < numAxisPoints; i++)
        {
            CurvePointParams parameterizedAxisPoint =
                parameterizedAxisPoints[i];

            cp.nextPointDirection = parameterizedAxisPoint.nextPointDirection;
            cp.nextPointDistance = parameterizedAxisPoint.nextPointDistance;

            convertedParameterizedAxisPoints[i] = cp;
        }

        m_contact_patches[contactNameChar]->setAxisPoints(axisPoints);
        m_contact_patches[contactNameChar]->setParameterizedAxis(
            convertedParameterizedAxisPoints);
    }

    int numContactPoints = serializedContactPoints.length();
    vector<SurfacePoint> patchPoints(numContactPoints);

    for (int i = 0; i < numContactPoints; i++)
    {
        MString serializedPoint = serializedContactPoints[i];
        patchPoints[i] = deserializeSurfacePoint(serializedPoint);
    }

    m_contact_patches[contactNameChar]->setPatchPoints(patchPoints);
    m_contact_patches[contactNameChar]->parameterizePatch(true);

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::getAxis(MString &axisName,
                                           MStringArray &serializedAxisPoints)
{
    MStatus status;

    serializedAxisPoints.clear();

    status = verifyAxisExistence(axisName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string axisNameChar = axisName.asChar();

    vector<SurfacePoint> axisPoints;

    m_axis_curves[axisNameChar]->getPoints(axisPoints);

    for (int i = 0; i < axisPoints.size(); i++)
    {
        MString serializedPoint = serializeSurfacePoint(axisPoints[i]);
        serializedAxisPoints.append(serializedPoint);
    }

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::getReassembledContactsFromAxisGroups(
    MStringArray &serializedContactPoints,
    MDoubleArray &contactPointParameterizedDistances)
{
    MStatus status;

    status = serializedContactPoints.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = contactPointParameterizedDistances.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int numTotalContacts = m_contact_point_axis_group_mapping.size();

    for (int i = 0; i < numTotalContacts; i++)
    {
        pair<string, int> mappingEntry = m_contact_point_axis_group_mapping[i];

        string axisNameChar = mappingEntry.first;
        int axisIndex = mappingEntry.second;

        vector<SurfacePoint> axisGroupedContactPoints;
        vector<PatchPointParams> parameterizedAxisGroupedContactPoints;

        m_contact_patches[axisNameChar]->getPoints(axisGroupedContactPoints);
        m_contact_patches[axisNameChar]->getParameterizedPoints(
            parameterizedAxisGroupedContactPoints);

        SurfacePoint desiredContactPoint = axisGroupedContactPoints[axisIndex];
        PatchPointParams desiredParameterizedContactPoint =
            parameterizedAxisGroupedContactPoints[axisIndex];

        double parameterizedDistance =
            desiredParameterizedContactPoint.axisPointDistance;

        MString serializedContactPoint =
            serializeSurfacePoint(desiredContactPoint);

        status = serializedContactPoints.append(serializedContactPoint);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status =
            contactPointParameterizedDistances.append(parameterizedDistance);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::initializeLandmarkParameterization()
{
    MStatus status;

    m_landmark_source_data.clear();

    vector<SurfacePoint> allAxisPointLandmarks;

    status = m_landmark_axis_names.clear();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (auto const &curveEntry : m_axis_curves)
    {
        string axisNameChar = curveEntry.first;

        vector<SurfacePoint> axisPointLandmarks;

        m_axis_curves[axisNameChar]->getPoints(axisPointLandmarks);

        for (int i = 0; i < axisPointLandmarks.size(); i++)
        {
            allAxisPointLandmarks.push_back(axisPointLandmarks[i]);

            status = m_landmark_axis_names.append(axisNameChar.c_str());
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = m_axis_point_indices.append(i);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
    }

    for (int i = 0; i < allAxisPointLandmarks.size(); i++)
    {
        SurfacePoint sp = allAxisPointLandmarks[i];

        m_landmark_source_data.push_back(make_tuple(sp, i));
    }

    m_landmark_scalar_extensions =
        m_vector_heat_solver.get()->extendScalar(m_landmark_source_data);

    // TODO: Kludge fix for edge case - sometimes the diffused value is negative
    // Not sure how this is possible - possibly bug in VHM?
    // Temporary fix: if vertex weight is negative, replace it with average of
    // all neighbors
    for (Vertex v : m_mesh->vertices())
    {
        double value = m_landmark_scalar_extensions[v];

        if (value < 0)
        {
            int numNeighbors = 0;
            double combinedWeight = 0.0;

            for (Vertex vn : v.adjacentVertices())
            {
                combinedWeight += m_landmark_scalar_extensions[vn];
                numNeighbors++;
            }

            double newWeight = combinedWeight / numNeighbors;

            cout << "Replacing " << v << " original weight "
                 << m_landmark_scalar_extensions[v] << " with " << newWeight
                 << endl;

            m_landmark_scalar_extensions[v] = newWeight;
        }
    }

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::parameterizeAllContactsFromLandmarks(
    MStringArray &serializedContactPoints)
{
    MStatus status;

    m_contact_patches.clear();
    m_contact_point_axis_group_mapping.clear();

    map<string, pair<MStringArray, MIntArray>> axisGroupedContacts;

    for (int i = 0; i < serializedContactPoints.length(); i++)
    {
        MString serializedContactPoint = serializedContactPoints[i];

        SurfacePoint cp = deserializeSurfacePoint(serializedContactPoint);

        int closestLandmarkIndex = findClosestLandmarkIndexToPoint(cp);

        if (closestLandmarkIndex == -1)
        {
            cout << "Invalid closest landmark index for point " << cp << endl;
            return MS::kFailure;
        }

        MString closestLandmarkAxis =
            m_landmark_axis_names[closestLandmarkIndex];
        int closestLandmarkAxisPointIndex =
            m_axis_point_indices[closestLandmarkIndex];

        string closestLandmarkAxisChar = closestLandmarkAxis.asChar();

        if (axisGroupedContacts.contains(closestLandmarkAxisChar))
        {
            status = axisGroupedContacts[closestLandmarkAxisChar].first.append(
                serializedContactPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = axisGroupedContacts[closestLandmarkAxisChar].second.append(
                closestLandmarkAxisPointIndex);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
        else
        {
            MStringArray axisSerializedContactGroup;
            MIntArray axisContactGroupPairingIndices;

            status = axisSerializedContactGroup.append(serializedContactPoint);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            status = axisContactGroupPairingIndices.append(
                closestLandmarkAxisPointIndex);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            axisGroupedContacts[closestLandmarkAxisChar] = make_pair(
                axisSerializedContactGroup, axisContactGroupPairingIndices);
        }

        pair<string, int> contactPointAxisGroupMapping = make_pair(
            closestLandmarkAxisChar,
            axisGroupedContacts[closestLandmarkAxisChar].first.length() - 1);

        m_contact_point_axis_group_mapping.push_back(
            contactPointAxisGroupMapping);
    }

    for (auto const &axisContactGroupEntry : axisGroupedContacts)
    {
        string axisNameChar = axisContactGroupEntry.first;

        MStringArray axisGroupedSerializedContactPoints =
            axisContactGroupEntry.second.first;
        MIntArray contactPointAxisIndices = axisContactGroupEntry.second.second;

        map<size_t, size_t> closestPointBindings;

        for (int i = 0; i < axisGroupedSerializedContactPoints.length(); i++)
        {
            closestPointBindings[i] = contactPointAxisIndices[i];
        }

        MString axisName = axisNameChar.c_str();

        status = createContactFromExplicitClosestPointBindings(
            axisName, axisGroupedSerializedContactPoints, closestPointBindings);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MS::kSuccess;
}

MStatus
GeometryProcessingContext::parseSerializedPoint(MString &serializedSurfacePoint,
                                                vector<int> &vertexIndices,
                                                vector<double> &coords)
{
    MStatus status;

    vertexIndices.clear();
    coords.clear();

    SurfacePoint surfacePoint = deserializeSurfacePoint(serializedSurfacePoint);
    SurfacePointType t = surfacePoint.type;

    if (t == SurfacePointType::Edge)
    {
        Edge e = surfacePoint.edge;
        double tEdge = surfacePoint.tEdge;

        int v1idx = e.firstVertex().getIndex();
        int v2idx = e.secondVertex().getIndex();

        vertexIndices.push_back(v1idx);
        vertexIndices.push_back(v2idx);

        coords.push_back(tEdge);
    }
    else if (t == SurfacePointType::Face)
    {
        Face f = surfacePoint.face;
        Vector3 faceCoords = surfacePoint.faceCoords;

        for (Vertex v : f.adjacentVertices())
        {
            int idx = v.getIndex();
            vertexIndices.push_back(idx);
        }

        coords.push_back(faceCoords.x);
        coords.push_back(faceCoords.y);
        coords.push_back(faceCoords.z);
    }
    else if (t == SurfacePointType::Vertex)
    {
        Vertex v = surfacePoint.vertex;
        int idx = v.getIndex();

        vertexIndices.push_back(idx);
    }
    else
    {
        MGlobal::displayInfo(
            "Unknown surface type found. Unable to parse point");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus
GeometryProcessingContext::registerAxis(MString &axisName,
                                        MStringArray &serializedAxisPoints)
{
    MStatus status;

    SurfaceCurve *sc =
        new SurfaceCurve(m_mesh.get(), m_geometry.get(), m_mmp_solver.get());

    int numPoints = serializedAxisPoints.length();
    vector<SurfacePoint> axisPoints(numPoints);

    for (int i = 0; i < numPoints; i++)
    {
        MString serializedAxisPoint = serializedAxisPoints[i];
        axisPoints[i] = deserializeSurfacePoint(serializedAxisPoint);
    }

    sc->setPoints(axisPoints);

    string axisNameChar = axisName.asChar();
    m_axis_curves[axisNameChar] = sc;

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::registerGeometry(MDagPath &geometry,
                                                    MDagPath &transform)
{
    MStatus status;

    MGlobal::displayInfo("Registering geometry...");

    MFnTransform fnTransform(transform, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MTransformationMatrix tfm = fnTransform.transformation(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnMesh fnMesh(geometry, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int polygonCount = fnMesh.numPolygons(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    int vertexCount = fnMesh.numVertices(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vector<vector<size_t>> polygons;
    vector<Vector3> vertexCoordinates;

    for (int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
    {
        MIntArray vertex_list;
        status = fnMesh.getPolygonVertices(polygonIndex, vertex_list);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        int size = vertex_list.length();

        vector<size_t> polygon;
        for (int faceIndex = 0; faceIndex < 3; faceIndex++)
        {
            polygon.push_back(vertex_list[faceIndex]);
        }

        polygons.push_back(polygon);
    }

    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {
        MPoint point;
        status = fnMesh.getPoint(vertexIndex, point);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        point *= tfm.asMatrix();

        Vector3 vertexCoordinate{point[0], point[1], point[2]};
        vertexCoordinates.push_back(vertexCoordinate);
    }

    tie(m_mesh, m_geometry) =
        loadMeshFromExplicitGeometry(polygons, vertexCoordinates);

    m_geometry->requireFaceTangentBasis();
    m_geometry->requireVertexTangentBasis();
    m_geometry->requireVertexIndices();

    if (m_mmp_solver == nullptr)
    {
        m_mmp_solver.reset(new GeodesicAlgorithmExact(*m_mesh, *m_geometry));
    }
    if (m_vector_heat_solver == nullptr)
    {
        m_vector_heat_solver.reset(
            new VectorHeatMethodSolver(*m_geometry, 0.01));
    }

    MGlobal::displayInfo("Successfully registered geometry");

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::transferAllContacts(
    GeometryProcessingContext *targetContext,
    map<string, MString> &sourceTargetAxisNameMappings,
    map<string, double> &targetSpreadCoefficients)
{
    MStatus status;

    int numContactGroups = m_contact_patches.size();

    targetContext->m_contact_patches.clear();
    targetContext->m_contact_point_axis_group_mapping.clear();

    for (auto const &axisContactGroupEntry : m_contact_patches)
    {
        string sourceAxisNameChar = axisContactGroupEntry.first;
        MString targetAxisName =
            sourceTargetAxisNameMappings[sourceAxisNameChar];

        string targetAxisNameChar = targetAxisName.asChar();
        double targetSpreadCoefficient =
            targetSpreadCoefficients[targetAxisNameChar];

        targetContext->m_contact_patches[targetAxisNameChar] = new SurfacePatch(
            targetContext->m_mesh.get(), targetContext->m_geometry.get(),
            targetContext->m_mmp_solver.get(),
            targetContext->m_vector_heat_solver.get());

        vector<SurfacePoint> targetAxisPoints;
        vector<CurvePointParams> sourceParameterizedAxisPoints;
        vector<PatchPointParams> sourceParameterizedContactPoints;

        targetContext->m_axis_curves[targetAxisNameChar]->getPoints(
            targetAxisPoints);

        m_contact_patches[sourceAxisNameChar]->getParameterizedAxis(
            sourceParameterizedAxisPoints);
        m_contact_patches[sourceAxisNameChar]->getParameterizedPoints(
            sourceParameterizedContactPoints);

        targetContext->m_contact_patches[targetAxisNameChar]->setAxisPoints(
            targetAxisPoints);
        targetContext->m_contact_patches[targetAxisNameChar]
            ->setParameterizedAxis(sourceParameterizedAxisPoints);
        targetContext->m_contact_patches[targetAxisNameChar]
            ->setParameterizedPatch(sourceParameterizedContactPoints);

        targetContext->m_contact_patches[targetAxisNameChar]
            ->setPatchSpreadCoefficient(targetSpreadCoefficient);

        targetContext->m_contact_patches[targetAxisNameChar]
            ->reconstructPatch();
    }

    int numTotalContacts = m_contact_point_axis_group_mapping.size();

    for (int i = 0; i < numTotalContacts; i++)
    {
        pair<string, int> mappingEntry = m_contact_point_axis_group_mapping[i];

        string sourceAxisNameChar = mappingEntry.first;
        MString targetAxisName =
            sourceTargetAxisNameMappings[sourceAxisNameChar];

        string targetAxisNameChar = targetAxisName.asChar();

        int axisIndex = mappingEntry.second;

        pair<string, int> contactTargetAxisGroupMapping =
            make_pair(targetAxisNameChar, axisIndex);

        targetContext->m_contact_point_axis_group_mapping.push_back(
            contactTargetAxisGroupMapping);
    }

    return MS::kSuccess;
}

// Private utils

MStatus
GeometryProcessingContext::createContactFromExplicitClosestPointBindings(
    MString &axisName, MStringArray &serializedContactPoints,
    map<size_t, size_t> &closestPointBindings)
{
    MStatus status;

    status = verifyAxisExistence(axisName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    string axisNameChar = axisName.asChar();

    m_contact_patches[axisNameChar] =
        new SurfacePatch(m_mesh.get(), m_geometry.get(), m_mmp_solver.get(),
                         m_vector_heat_solver.get());

    // Axis does not change, so generate only once
    vector<SurfacePoint> axisPoints;
    vector<CurvePointParams> parameterizedAxisPoints;

    m_axis_curves[axisNameChar]->getPoints(axisPoints);
    m_axis_curves[axisNameChar]->getParameterized(parameterizedAxisPoints);

    int numAxisPoints = axisPoints.size();

    vector<CurvePointParams> convertedParameterizedAxisPoints(numAxisPoints);

    CurvePointParams cp;

    for (int i = 0; i < numAxisPoints; i++)
    {
        CurvePointParams parameterizedAxisPoint = parameterizedAxisPoints[i];

        cp.nextPointDirection = parameterizedAxisPoint.nextPointDirection;
        cp.nextPointDistance = parameterizedAxisPoint.nextPointDistance;

        convertedParameterizedAxisPoints[i] = cp;
    }

    m_contact_patches[axisNameChar]->setAxisPoints(axisPoints);
    m_contact_patches[axisNameChar]->setParameterizedAxis(
        convertedParameterizedAxisPoints);

    int numContactPoints = serializedContactPoints.length();
    vector<SurfacePoint> patchPoints(numContactPoints);

    for (int i = 0; i < numContactPoints; i++)
    {
        MString serializedPoint = serializedContactPoints[i];
        patchPoints[i] = deserializeSurfacePoint(serializedPoint);
    }

    m_contact_patches[axisNameChar]->setPatchPoints(patchPoints);
    m_contact_patches[axisNameChar]->parameterizePatch(false,
                                                       closestPointBindings);

    return MS::kSuccess;
}

SurfacePoint GeometryProcessingContext::deserializeSurfacePoint(
    MString &serializedSurfacePoint)
{
    SurfacePoint pt;

    string elementType;
    size_t index;

    double edgeInterpWeight;
    double vertexCoordX, vertexCoordY, vertexCoordZ;

    istringstream iss(serializedSurfacePoint.asChar());
    iss >> elementType;

    if (elementType == "v")
    {
        iss >> index;
        pt = SurfacePoint(m_mesh->vertex(index));
    }
    else if (elementType == "e")
    {
        iss >> index >> edgeInterpWeight;
        pt = SurfacePoint(m_mesh->edge(index), edgeInterpWeight);
    }
    else if (elementType == "f")
    {
        iss >> index >> vertexCoordX >> vertexCoordY >> vertexCoordZ;
        Vector3 faceCoords = Vector3{vertexCoordX, vertexCoordY, vertexCoordZ};
        pt = SurfacePoint(m_mesh->face(index), faceCoords);
    }

    return pt;
}

int GeometryProcessingContext::findClosestLandmarkIndexToPoint(
    SurfacePoint &surfacePoint)
{
    double heatDiffusedVal =
        surfacePoint.interpolate(m_landmark_scalar_extensions);

    for (size_t i = 0; i < m_landmark_source_data.size() - 1; i++)
    {
        double lowerBound = get<1>(m_landmark_source_data[i]);
        double upperBound = get<1>(m_landmark_source_data[i + 1]);

        if (heatDiffusedVal >= lowerBound && heatDiffusedVal <= upperBound)
        {
            if ((heatDiffusedVal - lowerBound) > (upperBound - heatDiffusedVal))
            {
                return i;
            }
            return i + 1;
        }
    }

    // Edge case for contacts that will typically be filtered out anyway
    if (heatDiffusedVal >
        get<1>(m_landmark_source_data[m_landmark_source_data.size() - 1]))
    {
        return m_landmark_source_data.size() - 1;
    }

    // Should never reach here
    return -1;
}

MString
GeometryProcessingContext::serializeSurfacePoint(SurfacePoint &surfacePoint)
{
    MString result;

    SurfacePointType t = surfacePoint.type;

    if (t == SurfacePointType::Edge)
    {
        Edge e = surfacePoint.edge;
        int idx = e.getIndex();
        double tEdge = surfacePoint.tEdge;

        result = "e " + MString(to_string(idx).c_str()) + " " +
                 MString(to_string(tEdge).c_str());
    }
    else if (t == SurfacePointType::Face)
    {
        Face f = surfacePoint.face;
        int idx = f.getIndex();
        Vector3 faceCoords = surfacePoint.faceCoords;

        result = "f " + MString(to_string(idx).c_str()) + " " +
                 MString(to_string(faceCoords.x).c_str()) + " " +
                 MString(to_string(faceCoords.y).c_str()) + " " +
                 MString(to_string(faceCoords.z).c_str());
    }
    else if (t == SurfacePointType::Vertex)
    {
        Vertex v = surfacePoint.vertex;
        int idx = v.getIndex();

        result = "v " + MString(to_string(idx).c_str());
    }
    else
    {
        cout << "Unknown surface type found. Killing" << endl;
        result = "NULL";
    }

    return result;
}

MStatus GeometryProcessingContext::verifyAxisExistence(MString &axisName)
{
    MStatus status;

    string axisNameChar = axisName.asChar();

    if (!m_axis_curves.contains(axisNameChar))
    {
        MGlobal::displayInfo("ERROR: Axis " + axisName + " not found");
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MStatus GeometryProcessingContext::verifyContactExistence(MString &contactName)
{
    MStatus status;

    string contactNameChar = contactName.asChar();

    if (!m_contact_patches.contains(contactNameChar))
    {
        MGlobal::displayInfo("ERROR: Patch " + contactName + " not found");
        return MS::kFailure;
    }

    return MS::kSuccess;
}
