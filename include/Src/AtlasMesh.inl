#pragma once

int InitializeAtlasMesh(const TexturedMesh & inputMesh, AtlasMesh & outputMesh, const int width, const int height) {

	if (!InitializeTriangleChartIndexing(inputMesh, outputMesh.triangleChartIndex, outputMesh.numCharts)) {
		printf("Failed triangle chart indexing! \n");
		return 0;
	}
	printf("Num Charts %d \n", outputMesh.numCharts);

	int lastVertexIndex = 0;
	std::set<IndexedVector2D, IndexedVector2DComparison> IndexedPointSet;
	std::set<IndexedVector2D, IndexedVector2DComparison>::iterator it;

	for (int t = 0; t < inputMesh.triangles.size(); t++) {
		int cornerIndices[3];
		for (int k = 0; k < 3; k++) {
			int currentCorner = -1;
			IndexedVector2D idxP(inputMesh.textureCoordinates[3 * t + k], lastVertexIndex, inputMesh.triangles[t][k]);
			it = IndexedPointSet.find(idxP);
			if (it == IndexedPointSet.end()) {
				IndexedPointSet.insert(idxP);
				outputMesh.vertexMap.push_back(inputMesh.triangles[t][k]);
				currentCorner = lastVertexIndex;
				outputMesh.vertices.push_back(inputMesh.textureCoordinates[3 * t + k]);
				lastVertexIndex++;
			}
			else {
				IndexedVector2D indexPoint = *it;
				currentCorner = indexPoint.index;
			}
			cornerIndices[k] = currentCorner;
		}
		outputMesh.triangles.push_back(TriangleIndex(cornerIndices[0], cornerIndices[1], cornerIndices[2]));
	}
	if (1){//Jitter vertices lying on the grid to avoid degeneracies
		double precision = 1e-6;
		for (int i = 0; i < outputMesh.vertices.size(); i++) {
			for (int c = 0;c < 2; c++){
				double dimSize = c == 0 ? double(width) : double(height);
				double scaled = outputMesh.vertices[i][c] * dimSize - 0.5;
				double offset = scaled - double(round(scaled));
				if (fabs(offset) < precision) {
					if(0) printf("WARNING: Vertex lying over the grid. Automatic jittering applied. \n");
					if (offset > 0) {
						scaled = round(scaled) + precision*(1.0 + double(rand()) / double(RAND_MAX));
					}
					else {
						scaled = round(scaled) - precision*(1.0 + double(rand()) / double(RAND_MAX));
					}
					outputMesh.vertices[i][c] = (scaled + 0.5) / dimSize;
				}
			}
		}
	}

	std::unordered_map<unsigned long long, int> halfEdgeIndex;
	for (int i = 0; i < outputMesh.triangles.size(); i++) {
		for (int k = 0; k < 3; k++) {
			unsigned long long  edgeKey = SetMeshEdgeKey(outputMesh.triangles[i][k], outputMesh.triangles[i][(k + 1) % 3]);
			if (halfEdgeIndex.find(edgeKey) == halfEdgeIndex.end()) {
				halfEdgeIndex[edgeKey] = 3 * i + k;
			}
			else {
				printf("Non oriented manifold mesh!! \n");
				return 0;
			}
		}
	}

	int fineGridResolution = std::max<int>(width, height);

	int lastEdgeIndex = 2 * fineGridResolution*fineGridResolution*outputMesh.numCharts;
	std::vector<int> halfEdgeToEdgeIndex(3 * outputMesh.triangles.size(), -1);
	for (int i = 0; i < outputMesh.triangles.size(); i++) {
		for (int k = 0; k < 3; k++) {
			int currentEdgeIndex = 3 * i + k;
			unsigned long long edgeKey = SetMeshEdgeKey(outputMesh.triangles[i][(k + 1) % 3], outputMesh.triangles[i][k]);
			if (halfEdgeIndex.find(edgeKey) != halfEdgeIndex.end()) {
				int oppositeEdgeIndex = halfEdgeIndex[edgeKey];

				if (currentEdgeIndex < oppositeEdgeIndex) {
					halfEdgeToEdgeIndex[currentEdgeIndex] = halfEdgeToEdgeIndex[oppositeEdgeIndex] = lastEdgeIndex;
					lastEdgeIndex++;
				}
			}
			else {
				halfEdgeToEdgeIndex[currentEdgeIndex] = lastEdgeIndex;
				lastEdgeIndex++;
			}
		}
	}
	for (int i = 0; i < outputMesh.triangles.size(); i++) for (int k = 0; k < 3; k++) if (halfEdgeToEdgeIndex[3 * i + k] == -1) {
		printf("Non indexed half edge! \n");
		return 0;
	}

	outputMesh.halfEdgeToEdgeIndex = halfEdgeToEdgeIndex;

	return 1;
}

int InitializeBoundaryHalfEdges(const TexturedMesh & mesh, std::vector<int> & boundaryHalfEdges, std::vector<int> & oppositeHalfEdge, std::vector<bool> & isBoundaryHalfEdge, bool & isClosedMesh) {

	isClosedMesh = true;

	std::unordered_map<unsigned long long, int> edgeIndex;
	for (int i = 0; i < mesh.triangles.size(); i++) {
		for (int k = 0; k < 3; k++) {
			unsigned long long  edgeKey = SetMeshEdgeKey(mesh.triangles[i][k], mesh.triangles[i][(k + 1) % 3]);
			if (edgeIndex.find(edgeKey) == edgeIndex.end()) {
				edgeIndex[edgeKey] = 3 * i + k;
			}
			else {
				printf("Non manifold mesh!! \n");
				return 0;
			}
		}
	}

	oppositeHalfEdge.resize(3 * mesh.triangles.size());
	int lastBoundaryIndex = 0;
	isBoundaryHalfEdge.resize(3 * mesh.triangles.size(), false);

	for (int i = 0; i < mesh.triangles.size(); i++) {
		for (int k = 0; k < 3; k++) {
			int currentEdgeIndex = 3 * i + k;
			unsigned long long edgeKey = SetMeshEdgeKey(mesh.triangles[i][(k + 1) % 3], mesh.triangles[i][k]);
			if (edgeIndex.find(edgeKey) != edgeIndex.end()) {
				int oppositeEdgeIndex = edgeIndex[edgeKey];

				if (currentEdgeIndex < oppositeEdgeIndex) {
					oppositeHalfEdge[currentEdgeIndex] = oppositeEdgeIndex;
					oppositeHalfEdge[oppositeEdgeIndex] = currentEdgeIndex;
				}

				int tIndex = oppositeEdgeIndex / 3;
				int kIndex = oppositeEdgeIndex % 3;
				if (mesh.textureCoordinates[3 * i + ((k + 1) % 3)][0] == mesh.textureCoordinates[3 * tIndex + kIndex][0] &&
					mesh.textureCoordinates[3 * i + ((k + 1) % 3)][1] == mesh.textureCoordinates[3 * tIndex + kIndex][1] &&
					mesh.textureCoordinates[3 * i + k][0] == mesh.textureCoordinates[3 * tIndex + ((kIndex + 1) % 3)][0] &&
					mesh.textureCoordinates[3 * i + k][1] == mesh.textureCoordinates[3 * tIndex + ((kIndex + 1) % 3)][1]
					) {
					//Do nothing
				}
				else {
					if (currentEdgeIndex < oppositeEdgeIndex) {
						boundaryHalfEdges.push_back(currentEdgeIndex);
						boundaryHalfEdges.push_back(oppositeEdgeIndex);
						isBoundaryHalfEdge[currentEdgeIndex] = isBoundaryHalfEdge[oppositeEdgeIndex] = true;
					}
				}
			}
			else {
				isClosedMesh = false;
				oppositeHalfEdge[currentEdgeIndex] = -1;
				boundaryHalfEdges.push_back(currentEdgeIndex);
				isBoundaryHalfEdge[currentEdgeIndex] = true;
			}
		}
	}
	return 1;
}

int InitiallizeBoundaryVertices(const TexturedMesh & mesh, const std::vector<int> & boundaryHalfEdges, std::unordered_map<int, int> & boundaryVerticesIndices, int & lastBoundaryIndex) {

	lastBoundaryIndex = 0;

	for (int b = 0; b < boundaryHalfEdges.size(); b++) {
		int halfEdgeIndex = boundaryHalfEdges[b];
		int i = halfEdgeIndex / 3;
		int k = halfEdgeIndex % 3;
		if (boundaryVerticesIndices.find(mesh.triangles[i][k]) == boundaryVerticesIndices.end()) {
			boundaryVerticesIndices[mesh.triangles[i][k]] = lastBoundaryIndex;
			lastBoundaryIndex++;
		}
		if (boundaryVerticesIndices.find(mesh.triangles[i][(k + 1) % 3]) == boundaryVerticesIndices.end()) {
			boundaryVerticesIndices[mesh.triangles[i][(k + 1) % 3]] = lastBoundaryIndex;
			lastBoundaryIndex++;
		}
	}

	return 1;
}
