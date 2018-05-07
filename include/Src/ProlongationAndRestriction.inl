#pragma once

int InitializeProlongation(const int & numInteriorTexels, const int & numFineNodes, const int & numCoarseNodes, const std::vector<GridChart> & gridCharts, const std::vector<GridNodeInfo> & nodeInfo, Eigen::SparseMatrix<double> & prolongation) {

	std::vector<Eigen::Triplet<double>> prolongationTriplets;
	std::unordered_set<int> coveredNodes;

	std::vector<int> interiorTexelIndices(numCoarseNodes, -1);
	for (int i = 0; i < gridCharts.size(); i++) {
		const GridChart & gridChart = gridCharts[i];
		for (int j = 0; j < gridChart.globalTexelIndex.size(); j++) {
			if (gridChart.globalTexelIndex[j] != -1 && gridChart.globalInteriorTexelIndex[j] != -1) {
				interiorTexelIndices[gridChart.globalTexelIndex[j]] = gridChart.globalInteriorTexelIndex[j];
			}
		}
	}

	for (int i = 0; i < interiorTexelIndices.size(); i++) {
		if (interiorTexelIndices[i] != -1) {
			if (interiorTexelIndices[i] < 0 || interiorTexelIndices[i]>numFineNodes) {
				printf("Out of bound index! \n");
				return 0;
			}
			prolongationTriplets.push_back(Eigen::Triplet<double>(interiorTexelIndices[i], i, 1.0));
			coveredNodes.insert(i);
		}
	}

	const int numAuxiliarNodes = numFineNodes - numInteriorTexels;
	std::vector<int> auxiliarNodesDegree(numAuxiliarNodes, 0);

	for (int i = 0; i < gridCharts.size(); i++) {
		const GridChart & gridChart = gridCharts[i];
		for (int j = 0; j < gridChart.auxiliarNodes.size(); j++) {
			int auxiliarId = gridChart.auxiliarNodes[j].index - numInteriorTexels;
			auxiliarNodesDegree[auxiliarId]++;
		}
	}

	double precision_error = 1e-10;

	std::vector<double> auxiliarNodesCumWeight(numAuxiliarNodes, 0);

	for (int i = 0; i < gridCharts.size(); i++) {
		const GridChart & gridChart = gridCharts[i];
		for (int j = 0; j < gridChart.auxiliarNodes.size(); j++) {
			int auxiliarId = gridChart.auxiliarNodes[j].index - numInteriorTexels;
			int nodeDegree = auxiliarNodesDegree[auxiliarId];
			Point2D<double>nodePosition = gridChart.auxiliarNodes[j].position;
			int corner[2] = { floor(nodePosition[0] / gridChart.cellSizeW), floor(nodePosition[1] / gridChart.cellSizeH) };
			int cellId = gridChart.localCellIndex(corner[0], corner[1]);

			nodePosition[0] /= gridChart.cellSizeW;
			nodePosition[1] /= gridChart.cellSizeH;
			nodePosition[0] -= double(corner[0]);
			nodePosition[1] -= double(corner[1]);
			if (nodePosition[0] < 0 - precision_error || nodePosition[0] > 1 + precision_error || nodePosition[1] < 0 - precision_error || nodePosition[1] > 1 + precision_error) {
				printf("Sample out of unit box! (%f %f)\n", nodePosition[0], nodePosition[1]);
				return 0;
			}
			for (int k = 0; k < 4; k++) {
				double texelWeight = BilinearElementValue(k, nodePosition) / double(nodeDegree);
				if (fabs(texelWeight) > 1e-11) {
					auxiliarNodesCumWeight[auxiliarId] += texelWeight;
					int texelIndex = gridChart.cellIndices[cellId][k];
					if (nodeInfo[texelIndex].nodeType == 2) {
						printf("ERROR: Deep texel cannot be in the support of an auxiliar node. Weight %g (B) \n", texelWeight);
						for (int _k = 0; _k < 4; _k++) printf("Neighbours weight %g \n", BilinearElementValue(_k, nodePosition) / double(nodeDegree));
						return 0;
					}
					coveredNodes.insert(texelIndex);
					if (gridChart.auxiliarNodes[j].index < numInteriorTexels || gridChart.auxiliarNodes[j].index > numFineNodes || texelIndex < 0 || texelIndex> numCoarseNodes) {
						printf("Out of bound index! \n");
						return 0;
					}
					prolongationTriplets.push_back(Eigen::Triplet<double>(gridChart.auxiliarNodes[j].index, texelIndex, texelWeight));
				}
			}
		}
	}

	for (int i = 0; i < numAuxiliarNodes; i++) if (fabs(auxiliarNodesCumWeight[i] - 1.0) > precision_error) {
		printf("Cum weight out of precision %g \n", auxiliarNodesCumWeight[i]);
		return 0;
	}

	if (coveredNodes.size() != numCoarseNodes) {
		printf("ERROR: Total active texels does not match total texels!\n");
		return 0;
	}

	printf("Prolongation operator dimensions %d x %d \n", numFineNodes, numCoarseNodes);
	prolongation.resize(numFineNodes, numCoarseNodes);
	prolongation.setFromTriplets(prolongationTriplets.begin(), prolongationTriplets.end());

	return 1;
}

int InitializeAtlasHierachicalProlongation(GridAtlas & fineAtlas, const GridAtlas & coarseAtlas) {

	std::vector<ProlongationLine> & prolongationLines = fineAtlas.prolongationLines;

	for (int k = 0; k < fineAtlas.gridCharts.size(); k++) {
		const GridChart & fineChart = fineAtlas.gridCharts[k];
		const GridChart & coarseChart = coarseAtlas.gridCharts[k];
		int width = fineChart.globalTexelIndex.width();
		for (int j = 0; j < fineChart.globalTexelIndex.height(); j++) {
			int offset = 0;
			bool previousIsValid = false;
			int rasterStart = -1;
			while (offset < width) {
				bool currentIsValid = (fineChart.globalTexelIndex(offset, j) != -1);
				if (currentIsValid && !previousIsValid) rasterStart = offset; //Start raster line
				if ((!currentIsValid && previousIsValid) || (currentIsValid && offset == (width - 1))) { //Terminate raster line
					if (currentIsValid && offset == (width - 1)) offset++;
					ProlongationLine newLine;
					newLine.startIndex = fineChart.globalTexelIndex(rasterStart, j);
					newLine.length = offset - rasterStart;

					int fi = rasterStart;
					int fj = j;

					int _fi = fi - fineChart.centerOffset[0];
					int _di = ((_fi % 2) == 0) ? 0 : 1;
					int _fj = fj - fineChart.centerOffset[1];
					int _dj = ((_fj % 2) == 0) ? 0 : 1;
					int ci = (_fi - _di) / 2 + coarseChart.centerOffset[0];
					int cj = (_fj - _dj) / 2 + coarseChart.centerOffset[1];
					//int sign_fj = _fj < 0 ? -1 : (_fj > 0 ? 1 : 0);

					newLine.alignedStart = ((_fi % 2) == 0);

					if (_fj % 2 == 0) {
						newLine.prevLineIndex = -1;
						newLine.nextLineIndex = -1;
						int globalIndex = coarseChart.globalTexelIndex(ci, cj);
						if (globalIndex == -1) {
							printf("ERROR: coarse texel is unactive!(A) \n");
							return 0;
						}
						else {
							newLine.centerLineIndex = globalIndex;
						}
					}
					else {
						newLine.centerLineIndex = -1;

						int globalIndex = coarseChart.globalTexelIndex(ci, cj);
						if (globalIndex == -1) {
							printf("ERROR: coarse texel is unactive!(B) \n");
							return 0;
						}
						else {
							newLine.prevLineIndex = globalIndex;
						}

						globalIndex = coarseChart.globalTexelIndex(ci, cj + 1);

						if (globalIndex == -1) {
							printf("ERROR: coarse texel is unactive!(C) \n");
							return 0;
						}
						else {
							newLine.nextLineIndex = globalIndex;
						}
					}
					prolongationLines.push_back(newLine);
				}
				previousIsValid = currentIsValid;
				offset++;
			}
		}
	}

	if (1) {
		std::vector<Eigen::Triplet<double>> prolongationTriplets;

		for (int r = 0; r < prolongationLines.size(); r++) {
			int startIndex = prolongationLines[r].startIndex;
			int lineLenght = prolongationLines[r].length;
			int centerLineStart = prolongationLines[r].centerLineIndex;
			int previousLineStart = prolongationLines[r].prevLineIndex;
			int nextLineStart = prolongationLines[r].nextLineIndex;
			int offset = prolongationLines[r].alignedStart ? 0 : 1;
			if (centerLineStart != -1) {
				for (int i = 0; i < lineLenght; i++) {
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, centerLineStart + (i + offset) / 2, 0.5));
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, centerLineStart + (i + offset + 1) / 2, 0.5));
				}
			}
			else {
				for (int i = 0; i < lineLenght; i++) {
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, previousLineStart + (i + offset) / 2, 0.25));
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, previousLineStart + (i + offset + 1) / 2, 0.25));
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, nextLineStart + (i + offset) / 2, 0.25));
					prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, nextLineStart + (i + offset + 1) / 2, 0.25));
				}
			}
		}

		Eigen::SparseMatrix<double> prolongation;
		prolongation.resize(fineAtlas.numTexels, coarseAtlas.numTexels);
		prolongation.setFromTriplets(prolongationTriplets.begin(), prolongationTriplets.end());
		Eigen::VectorXd ones = Eigen::VectorXd::Ones(coarseAtlas.numTexels);
		Eigen::VectorXd prolongedOnes = prolongation*ones;
		for (int i = 0; i < fineAtlas.numTexels; i++) {
			if (fabs(prolongedOnes[i] - 1.0) > 1e-10) {
				printf("ERROR: Prolongation does not add up to one! %d -> %g \n", i, prolongedOnes[i]);
				printf("Node info : chart %d  pos (%d %d) \n", fineAtlas.nodeInfo[i].chartId, fineAtlas.nodeInfo[i].ci, fineAtlas.nodeInfo[i].cj);
				return 0;
			}
		}
	}

	return 1;
}

int InitializeProlongationMatrix(const GridAtlas & fineAtlas, GridAtlas & coarseAtlas, SparseMatrix<double, int> & __prolongation) {

	const std::vector<ProlongationLine> & prolongationLines = fineAtlas.prolongationLines;

	std::vector<Eigen::Triplet<double>> prolongationTriplets;

	for (int r = 0; r < prolongationLines.size(); r++) {
		int startIndex = prolongationLines[r].startIndex;
		int lineLenght = prolongationLines[r].length;
		int centerLineStart = prolongationLines[r].centerLineIndex;
		int previousLineStart = prolongationLines[r].prevLineIndex;
		int nextLineStart = prolongationLines[r].nextLineIndex;
		int offset = prolongationLines[r].alignedStart ? 0 : 1;
		if (centerLineStart != -1) {
			for (int i = 0; i < lineLenght; i++) {
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, centerLineStart + (i + offset) / 2, 0.5));
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, centerLineStart + (i + offset + 1) / 2, 0.5));
			}
		}
		else {
			for (int i = 0; i < lineLenght; i++) {
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, previousLineStart + (i + offset) / 2, 0.25));
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, previousLineStart + (i + offset + 1) / 2, 0.25));
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, nextLineStart + (i + offset) / 2, 0.25));
				prolongationTriplets.push_back(Eigen::Triplet<double>(startIndex + i, nextLineStart + (i + offset + 1) / 2, 0.25));
			}
		}
	}

	Eigen::SparseMatrix<double> prolongation;
	prolongation.resize(fineAtlas.numTexels, coarseAtlas.numTexels);
	prolongation.setFromTriplets(prolongationTriplets.begin(), prolongationTriplets.end());
	Eigen::VectorXd ones = Eigen::VectorXd::Ones(coarseAtlas.numTexels);
	Eigen::VectorXd prolongedOnes = prolongation*ones;
	for (int i = 0; i < fineAtlas.numTexels; i++) {
		if (fabs(prolongedOnes[i] - 1.0) > 1e-10) {
			printf("ERROR: Prolongation does not add up to one! %d -> %g \n", i, prolongedOnes[i]);
			printf("Node info : chart %d  pos (%d %d) \n", fineAtlas.nodeInfo[i].chartId, fineAtlas.nodeInfo[i].ci, fineAtlas.nodeInfo[i].cj);
			return 0;
		}
	}

	SparseMatrixParser(prolongation, __prolongation, false);

	return 1;
}

//Coarse restriction
int InitializeAtlasHierachicalRestriction(const GridAtlas & fineAtlas, GridAtlas & coarseAtlas, SparseMatrix<double, int> & boundaryRestriction) {

	std::vector<Eigen::Triplet<double>> boundaryRestrictionTriplets;

	const std::vector<GridNodeInfo> & coarseNodeInfo = coarseAtlas.nodeInfo;

	const std::vector<int> & coarseBoundaryDeepIndexing = coarseAtlas.boundaryAndDeepIndex;

	const std::vector<RasterLine> & coarseRasterLines = coarseAtlas.rasterLines;
	std::vector<RasterLine> & restrictionLines = coarseAtlas.restrictionLines;
	std::vector<DeepLine> & deepLines = coarseAtlas.deepLines;
	//Initialize restrictionLines

	//std::vector<Eigen::Triplet<double>> fullRestrictionTriplets;

	restrictionLines.resize(coarseRasterLines.size());
	deepLines.resize(coarseRasterLines.size());

#pragma omp parallel for
	for (int i = 0; i < coarseRasterLines.size(); i++) {
		int lineCoarseStartIndex = coarseRasterLines[i].lineStartIndex;
		int lineCoarseLength = coarseRasterLines[i].lineEndIndex - lineCoarseStartIndex + 1;

		restrictionLines[i].coeffStartIndex = lineCoarseStartIndex; //global (NOT DEEP) variable index in the current level

		deepLines[i].coarseLineStartIndex = coarseRasterLines[i].coeffStartIndex;
		deepLines[i].coarseLineEndIndex = coarseRasterLines[i].coeffStartIndex + lineCoarseLength - 1;

		GridNodeInfo startNodeInfo = coarseNodeInfo[lineCoarseStartIndex];
		if (startNodeInfo.nodeType != 2) {
			printf("Error: not a deep node!");
			//return 0;
		}

		int ci = startNodeInfo.ci;
		int cj = startNodeInfo.cj;
		int chartId = startNodeInfo.chartId;

		const GridChart & fineChart = fineAtlas.gridCharts[chartId];
		const GridChart & coarseChart = coarseAtlas.gridCharts[chartId];

		int fi = fineChart.centerOffset[0] + 2.0 *(ci - coarseChart.centerOffset[0]);
		int fj = fineChart.centerOffset[1] + 2.0 *(cj - coarseChart.centerOffset[1]);
		if (fi - 1 < 0 || fi + 1> fineChart.width - 1 || fj - 1 < 0 || fj + 1> fineChart.height - 1) {
			printf("ERROR: Out of bounds node position! \n");
			//return 0;
		}
		int fineCurrentLineStart = fineChart.globalTexelIndex(fi, fj);
		if (fineCurrentLineStart != -1) {
			restrictionLines[i].lineStartIndex = fineCurrentLineStart;
			restrictionLines[i].lineEndIndex = fineCurrentLineStart + 2 * (coarseRasterLines[i].lineEndIndex - lineCoarseStartIndex);

			int fineCurrentDeep = fineChart.globalTexelDeepIndex(fi, fj);
			if (fineCurrentDeep != -1) {
				deepLines[i].fineCurrentLineIndex = fineCurrentDeep;
			}
			else {
				printf("ERROR: Invalid fine line start index. \n");
				//return 0;
			}
		}
		else {
			printf("ERROR: Invalid fine line start index. \n");
			printf("Fine resolution %d x %d , center (%d %d) \n", fineChart.globalTexelIndex.width(), fineChart.globalTexelIndex.height(), fineChart.centerOffset[0], fineChart.centerOffset[1]);
			printf("Coarse resolution %d x %d , center (%d %d) \n", coarseChart.globalTexelIndex.width(), coarseChart.globalTexelIndex.height(), coarseChart.centerOffset[0], coarseChart.centerOffset[1]);
			printf("Fine index (%d %d), Coarse index (%d %d) \n", fi, fj, ci, cj);
			//return 0;
		}

		int finePreviousLineStart = fineChart.globalTexelIndex(fi, fj - 1);
		if (finePreviousLineStart != -1) {
			restrictionLines[i].prevLineIndex = finePreviousLineStart;

			int finePreviousDeep = fineChart.globalTexelDeepIndex(fi, fj - 1);
			if (finePreviousDeep != -1) {
				deepLines[i].finePrevLineIndex = finePreviousDeep;
			}
			else {
				printf("ERROR: Invalid fine line start index. \n");
				//return 0;
			}
		}
		else {
			printf("ERROR: Invalid fine previous line start index \n");
			//return 0;
		}


		int fineNextLineStart = fineChart.globalTexelIndex(fi, fj + 1);
		if (fineNextLineStart != -1) {
			restrictionLines[i].nextLineIndex = fineNextLineStart;

			int fineNextDeep = fineChart.globalTexelDeepIndex(fi, fj + 1);
			if (fineNextDeep != -1) {
				deepLines[i].fineNextLineIndex = fineNextDeep;
			}
			else {
				printf("ERROR: Invalid fine line start index. \n");
				//return 0;
			}

		}
		else {
			printf("ERROR: Invalid fine next line start index \n");
			//return 0;
		}
	}

	//Initialize boundary nodes prolongation

	double expectedWeight = 0;

	for (int k = 0; k < fineAtlas.gridCharts.size(); k++) {

		const GridChart & fineChart = fineAtlas.gridCharts[k];
		const GridChart & coarseChart = coarseAtlas.gridCharts[k];

		for (int fj = 0; fj < fineChart.height; fj++) for (int fi = 0; fi < fineChart.width; fi++) {
			if (fineChart.globalTexelIndex(fi, fj) != -1) {

				int _fi = fi - fineChart.centerOffset[0];
				int sign_fi = _fi < 0 ? -1 : (_fi > 0 ? 1 : 0);
				int _fj = fj - fineChart.centerOffset[1];
				int sign_fj = _fj < 0 ? -1 : (_fj > 0 ? 1 : 0);

				int ci[2];
				double ci_weights[2];
				if (_fi % 2 == 0) {
					ci[0] = _fi / 2 + coarseChart.centerOffset[0];
					ci[1] = -1;
					ci_weights[0] = 1.0;
					ci_weights[1] = 0.0;
				}
				else {
					ci[0] = _fi / 2 + coarseChart.centerOffset[0];
					ci[1] = (_fi + sign_fi) / 2 + coarseChart.centerOffset[0];
					ci_weights[0] = 0.5;
					ci_weights[1] = 0.5;
				}

				int cj[2];
				double cj_weights[2];
				if (_fj % 2 == 0) {
					cj[0] = _fj / 2 + coarseChart.centerOffset[1];
					cj[1] = -1;
					cj_weights[0] = 1.0;
					cj_weights[1] = 0.0;
				}
				else {
					cj[0] = _fj / 2 + coarseChart.centerOffset[1];
					cj[1] = (_fj + sign_fj) / 2 + coarseChart.centerOffset[1];
					cj_weights[0] = 0.5;
					cj_weights[1] = 0.5;
				}
				for (int di = 0; di < 2; di++)for (int dj = 0; dj < 2; dj++) {
					if (ci[di] != -1 && cj[dj] != -1) {
						int coarseNodeGlobalIndex = coarseChart.globalTexelIndex(ci[di], cj[dj]);
						if (coarseNodeGlobalIndex == -1) {
							printf("ERROR: coarse texel is unactive! (D) \n");
							printf("Fine chart texel %d %d \n", fi, fj);
							printf("Fine chart center %d %d \n", fineChart.centerOffset[0], fineChart.centerOffset[1]);
							printf("Fine chart centered texel %d %d \n", _fi, _fj);
							printf("coarse chart center %d %d \n", coarseChart.centerOffset[0], coarseChart.centerOffset[1]);
							printf("coarse chart texel %d %d \n", ci[di], cj[dj]);

							return 0;
						}
						else {
							int coarseNodeBoundaryIndex = coarseBoundaryDeepIndexing[coarseNodeGlobalIndex] - 1;
							if (coarseNodeBoundaryIndex >= 0) {
								boundaryRestrictionTriplets.push_back(Eigen::Triplet<double>(coarseNodeBoundaryIndex, fineChart.globalTexelIndex(fi, fj), ci_weights[di] * cj_weights[dj]));
							}
						}
					}
				}
			}
		}
	}

	Eigen::SparseMatrix<double> __boundaryRestriction;
	__boundaryRestriction.resize(coarseAtlas.numBoundaryTexels, fineAtlas.numTexels);
	__boundaryRestriction.setFromTriplets(boundaryRestrictionTriplets.begin(), boundaryRestrictionTriplets.end());

	SparseMatrixParser(__boundaryRestriction, boundaryRestriction, false);

	return 1;
}
