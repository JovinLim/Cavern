#include "CavernGenerator.h"
#include <iostream>
#include <stdlib.h>

TArray<TArray<TArray<float>>> ACavernGenerator::GenerateMatrix()
{
	TArray<TArray<TArray<float>>> m_id;
	for (int z = 0; z < z_size; z++) {
		TArray<TArray<float>> y_length;
		for (int y = 0; y < y_size; y++) {
			TArray<float> x_breadth;
			for (int x = 0; x < x_size; x++) {
				//if (z == 0 || x == 0 || x == x_size - 1  || z == z_size - 1) {
				//	x_breadth.Add(float(SurfaceLevel - 1));
				//}

				////else if (y == 0 || y == y_size - 1) {
				////	x_breadth.Add(float(SurfaceLevel + 1));
				////}
				//else {
				//	//float isoVal = float(fabs(rand() % z_size - fabs((z_size/2) - z)) / float((z_size/2) - 0.1));
				//	float isoVal = SurfaceLevel + 1;
				//	//FString print = FString::SanitizeFloat(isoVal);
				//	//UE_LOG(LogTemp, Warning, TEXT("Isoval : %s"), *print);
				//	x_breadth.Add(isoVal);
				//}
				x_breadth.Add(1.0f);
			}
			y_length.Add(x_breadth);
		}
		m_id.Add(y_length);
	}

	//wallJitter(m_id);
	//SmoothMatrix(m_id);

	return m_id;
}

TArray<TArray<float>> ACavernGenerator::GenerateMatrix2D()
{
	TArray<TArray<float>> m_id;
	for (int y = 0; y < y_size; y++) {
		TArray<float> m_idy;
		for (int x = 0; x < x_size; x++) {
			m_idy.Add(0);
		}
		m_id.Add(m_idy);
	}
	return m_id;
}

//Function to visualize matrix in output log of UE5
void ACavernGenerator::PrintMatrix(TArray<TArray<float>>& matrix) {
	int rows = matrix.Num();
	int cols = matrix[0].Num();
	//FString print = "Matrix is \n";
	FString print = " \n";

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			print += " ";
			print += FString::SanitizeFloat(matrix[rows - 1 - r][c]);
		}

		print += "\n";
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
}

void ACavernGenerator::ShowDebugGeometry(TArray<TArray<TArray<float>>> matrix)
{
	int xInd = matrix[0][0].Num();
	int yInd = matrix[0].Num();
	int zInd = matrix.Num();

	for (int z = 0; z < zInd; z++) {
		for (int y = 0; y < yInd; y++) {
			for (int x = 0; x < xInd; x++) {
				//if (x == 0 || x == xInd-1 ) {
				//	if (y == 0 || y == yInd - 1){
				//		if (z == 0 || z == zInd - 1) {
				//			DrawDebugSphere(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), 5, 2, FColor::Green, true, -1, 0, 2);
				//			FString print = FString::SanitizeFloat(x);
				//			UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
				//		}
				//	}
				//	//DrawDebugBox(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), FVector(5, 5, 5), FColor::Green, true, -1, 0, 2);
				//	FString print = FString::SanitizeFloat(matrix[z][y][x]);
				//	//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
				//}

				if (matrix[z][y][x] > SurfaceLevel) {
					DrawDebugSphere(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), 5, 2, FColor::Green, true, -1, 0, 2);
					//DrawDebugBox(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), FVector(5, 5, 5), FColor::Green, true, -1, 0, 2);
					FString print = FString::SanitizeFloat(matrix[z][y][x]);
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
				}

				else {
					DrawDebugSphere(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), 5, 2, FColor::Red, true, -1, 0, 2);
					//DrawDebugBox(GetWorld(), FVector(x * gridSize, y * gridSize, z * gridSize), FVector(5, 5, 5), FColor::Red, true, -1, 0, 2);
					FString print = FString::SanitizeFloat(matrix[z][y][x]);
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
				}
			}
		}
	}
}

void ACavernGenerator::SmoothMatrix(TArray<TArray<TArray<float>>>& matrix)
{
	TArray<TArray<TArray<float>>> matrixCopy = matrix;
	for (int z = 0; z < z_size; z++) {
		for (int y = 0; y < y_size; y++) {
			for (int x = 0; x < x_size; x++) {
				TArray<int> target = { x,y,z };
				int adj = adjCount(matrixCopy, target);
				if (adj < 15) {
					matrix[z][y][x] = float(SurfaceLevel + 1);
				}
				else if (adj == 99) {
					
				}
				else {
					//matrix[z][y][x] = float(SurfaceLevel - 1);
				}
			}
		}
	}

}

int ACavernGenerator::adjCount(TArray<TArray<TArray<float>>>& matrix, TArray<int> target) {
	int count = 0;
	int x = target[0];
	int y = target[1];
	int z = target[2];

	if (x < 1 || y < 1 || z < 1 || x == x_size - 1 || y == y_size - 1 || z == z_size - 1) {
		return 99;
	}
	else {
		for (int z_ind = -1; z_ind < 2; z_ind++){
			for (int y_ind = -1; y_ind < 2; y_ind++) {
				for (int x_ind = -1; x_ind < 2; x_ind++) {
					if (z_ind == z && y_ind == y && x_ind == x) {
						continue;
					}
					else if (matrix[z + z_ind][y + y_ind][x + x_ind] < SurfaceLevel) {
						count++;
					}
				}
			}
		}
	}

	return count;
}


void ACavernGenerator::wallJitter(TArray<TArray<TArray<float>>>& matrix)
{
	//for (int z = 1; z < z_size - 1; z++) {
	//	for (int y = 0; y < y_size - 1; y++) {
	//		for (int x = 1; x < x_jitter; x++) {
	//			float isoVal = (rand() % 80) + (x_jitter * 1.5);
	//			float isoVal2 = (rand() % 80) + (x_jitter * 1.5);
	//			matrix[z][y][x] = isoVal;
	//			matrix[z][y][x_size - 1 - x] = isoVal2;
	//		}
	//	}
	//}


	TArray<TArray<float>> matrixPerlin = PerlinNoise2D(perlinSeed, y_size, z_size - 1, WallNoiseOffset, 0);

	//FString print = FString::SanitizeFloat(matrixPerlin.Num());
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
	//FString print2 = FString::SanitizeFloat(matrixPerlin[0].Num());
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *print2);

	for (int y = 1; y < y_size; y++) {
		for (int z = 1; z < z_size - 1; z++) {
			int extent = round(matrixPerlin[z][y]);
			//int extent = matrixPerlin[z][y];
			//FString print2 = FString::SanitizeFloat(extent);
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *print2);
			for (int e = 0; e < extent; e++) {
				matrix[z][y][e + 1] = float(SurfaceLevel - 1);
			}
		}
	}

}






