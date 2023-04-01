#include "CavernGenerator.h"
#include <iostream>
#include <stdlib.h>
#include <random>

// TO DO
// CHECK FOR OUT OF BOUNDS

void ACavernGenerator::generateStag(TArray<TArray<TArray<float>>>& matrix)
{
	// Set random seed
	srand(CASeed);

	// Calculating maximum number of points that can be occupied
	
	// CA method
	int stagMaxPt = (x_size * y_size * (100 - EmptySpace)) / 100;
	int stagPtCount = 0;
	int stagCount = 0;
	int gridmax = x_size * y_size;


	// Making copy of ground matrix points to check against; prevent duplicate points from being chosen
	TArray<TArray<float>> matrixCheck;
	for (int y = 0; y < matrix[0].Num(); y++) {
		TArray<float> matrixChecky;
		for (int x = 0; x < matrix[0][0].Num(); x++) {
			matrixChecky.Add(0);
		}
		matrixCheck.Add(matrixChecky);
	}

	// Array for indexes to march on
	TArray<TArray<int>> marchIndex;

	// Generating stag points on ground
	while (stagPtCount < stagMaxPt) {
		UE_LOG(LogTemp, Warning, TEXT("Looping..."));

		// Check if point has already been chosen
		int randXind = rand() % x_size;
		int randYind = rand() % y_size;
		if (matrixCheck[randYind][randXind] == 1) {

			continue;
		}

		// Generating initial points at z = 0
		else {
			matrixCheck[randYind][randXind] = 1;
			int stagVar = maxStagSize - minStagSize;
			int stagDia = rand() % minStagSize + stagVar;
			int stagRad = stagDia / 2;

			// Add in random variation for 
			for (int z = 0; z < z_size/stagHeightScale; z++) {
				if (z < z_size) {
					//for (int y = -stagRad + z + (rand() % 3 + 1); y < stagRad - z - (rand() % 3 + 1); y++) {
					for (int y = -stagRad + z; y < stagRad - z; y++) {
						//for (int x = -stagRad + z + (rand() % 3 + 1); x < stagRad - z - (rand() % 3 + 1); x++) {
						for (int x = -stagRad + z ; x < stagRad - z ; x++) {
							if ((y * y) + (x * x) < (stagRad * stagRad)) {
								if (randYind + y < y_size && randXind + x < x_size && randYind + y >= 0 && randXind + x >= 0) {
									if (z == 0) {
										matrix[z][randYind + y][randXind + x] = SurfaceLevel - 1;
										marchIndex.Add(TArray<int>{randXind + x, randYind + y, z});
										stagPtCount += 1;
									}

									else {
										if (matrix[z - 1][randYind + y][randXind + x] == SurfaceLevel - 1) {
											matrix[z][randYind + y][randXind + x] = SurfaceLevel - 1;
											marchIndex.Add(TArray<int>{randXind + x, randYind + y, z});
										}
									}

								}

								else {

								}
							}
						}
					}
				}

				// Once height has reached limit of stag size - 2
				else {
					for (int y = -stagRad + z; y < stagRad - z; y++) {
						for (int x = -stagRad + z; x < stagRad - z; x++) {
							if ((y * y) + (x * x) < (stagRad * stagRad)) {
								if (randYind + y < y_size && randXind + x < x_size && randYind + y >= 0 && randXind + x >= 0) {
									if (matrix[z - 1][randYind + y][randXind + x] == SurfaceLevel - 1) {
										matrix[z][randYind + y][randXind + x] = SurfaceLevel - 1;
										marchIndex.Add(TArray<int>{randXind + x, randYind + y, z});
									}
								}

								else {

								}
							}
						}
					}
				}

			}
		}

		stagCount++;
	}

	FString print = FString::SanitizeFloat(stagCount);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *print);

	// Get Perlin Noise for 3D matrix
	TArray<TArray<TArray<float>>> matrixPerlin = PerlinNoise3D(perlinSeed, x_size, y_size, z_size);


	// March
	GenerateMeshNoised(matrix, matrixPerlin, marchIndex);
	//GenerateMesh(matrix, marchIndex);
	ApplyMesh(0);

	//ShowDebugGeometry(matrix);
	//PrintMatrix(matrixCheck);
	//PrintMatrix(matrix);

}