// Fill out your copyright notice in the Description page of Project Settings.


#include "CavernGenerator.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "Utils/VoxelMeshData.h"
#include "ProceduralMeshComponent.h"

// Sets default values
ACavernGenerator::ACavernGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");

	// Mesh Settings
	Mesh->SetCastShadow(false);

	// Set Mesh as root
	SetRootComponent(Mesh);

}

// Called when the game starts or when spawned
void ACavernGenerator::BeginPlay()
{
	Super::BeginPlay();
	//TArray<TArray<TArray<float>>> matrix = GenerateMatrix();
	//PrintMatrix(matrix[0]);
	
	//PERLIN STUFF
	//TArray<TArray<float>> matrixPerlin = PerlinNoise2D(perlinSeed, x_size, y_size);
	//PrintMatrix(matrixPerlin);


	//DEBUGGING POINTS
	//ShowDebugGeometry(matrix);
	 

	//MAKE MESH
	//Setup();
	//Generate3DHeightMap(matrix);
	//GenerateMesh(matrix);
	//UE_LOG(LogTemp, Warning, TEXT("Vertex Count : %d"), VertexCount);
	//ApplyMesh();

	//CUSTOM MESH
	CustomMesh(perlinSeed , 1);
	ApplyMesh(0);

	//TEST STAG GENERATION
	//TArray<TArray<float>> matrix = GenerateMatrix2D();
	//generateStag(matrix);

	UE_LOG(LogTemp, Warning, TEXT("Successfully set static mesh"));
}

// Called every frame
void ACavernGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACavernGenerator::Setup()
{
	// Initialize Voxels
	Voxels.SetNum((z_size + 1) * (y_size + 1) * (x_size + 1));
}

void ACavernGenerator::Generate3DHeightMap(TArray<TArray<TArray<float>>> matrix)
{
	for (int x = 0; x < x_size; ++x)
	{
		for (int y = 0; y < y_size; ++y)
		{
			for (int z = 0; z < z_size; ++z)
			{
				Voxels[GetVoxelIndex(x, y, z)] = matrix[z][y][x];	
			}
		}
	}
}

void ACavernGenerator::March(int x, int y, int z, const float Cube[8])
{
	int VertexMask = 0;
	FVector EdgeVertex[12];

	for (int i = 0; i < 8; i++) {
		if (Cube[i] <= SurfaceLevel) VertexMask |= 1 << i;
	}

	const int EdgeMask = CubeEdgeFlags[VertexMask];
	FString print = FString::FromInt(EdgeMask);
	//UE_LOG(LogTemp, Warning, TEXT("Isoval : %s"), *print);

	if (EdgeMask == 0) return;
		
	// Find intersection points
	for (int i = 0; i < 12; ++i)
	{
		if ((EdgeMask & 1 << i) != 0)
		{
			const float Offset = Interpolation ? GetInterpolationOffset(Cube[EdgeConnection[i][0]], Cube[EdgeConnection[i][1]]) : 0.5f;

			EdgeVertex[i].X = (x + (VertexOffset[EdgeConnection[i][0]][0] + Offset * EdgeDirection[i][0])) * gridSize;
			EdgeVertex[i].Y = (y + (VertexOffset[EdgeConnection[i][0]][1] + Offset * EdgeDirection[i][1])) * gridSize;
			EdgeVertex[i].Z = (z + (VertexOffset[EdgeConnection[i][0]][2] + Offset * EdgeDirection[i][2])) * gridSize;
		}
	}

	// Save triangles, at most can be 5
	for (int i = 0; i < 5; ++i)
	{
		if (TriangleConnectionTable[VertexMask][3 * i] < 0) break;

		auto V1 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i]];
		auto V2 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 1]];
		auto V3 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 2]];

		auto Normal = FVector::CrossProduct(V2 - V1, V3 - V1);
		auto Color = FColor::MakeRandomColor();

		Normal.Normalize();

		MeshData.Vertices.Append({ V3, V2, V1 });

		MeshData.Triangles.Append({
			VertexCount + TriangleOrder[0],
			VertexCount + TriangleOrder[1],
			VertexCount + TriangleOrder[2]
			});

		MeshData.Normals.Append({
			Normal,
			Normal,
			Normal
			});

		MeshData.Colors.Append({
			Color,
			Color,
			Color
			});

		VertexCount += 3;
	}
	
}

void ACavernGenerator::MarchNoised(int x, int y, int z, const float Cube[8], TArray<TArray<TArray<float>>> matrixPerlin)
{
	int VertexMask = 0;
	FVector EdgeVertex[12];

	for (int i = 0; i < 8; i++) {
		if (Cube[i] <= SurfaceLevel) VertexMask |= 1 << i;
	}

	const int EdgeMask = CubeEdgeFlags[VertexMask];
	FString print = FString::FromInt(EdgeMask);
	//UE_LOG(LogTemp, Warning, TEXT("Isoval : %s"), *print);

	if (EdgeMask == 0) return;

	// Find intersection points
	for (int i = 0; i < 12; ++i)
	{
		if ((EdgeMask & 1 << i) != 0)
		{
			const float Offset = Interpolation ? GetInterpolationOffset(Cube[EdgeConnection[i][0]], Cube[EdgeConnection[i][1]]) : 0.5f;

			EdgeVertex[i].X = (x + (VertexOffset[EdgeConnection[i][0]][0] + Offset * EdgeDirection[i][0])) * gridSize;
			EdgeVertex[i].Y = (y + (VertexOffset[EdgeConnection[i][0]][1] + Offset * EdgeDirection[i][1])) * gridSize;
			EdgeVertex[i].Z = (z + (VertexOffset[EdgeConnection[i][0]][2] + Offset * EdgeDirection[i][2])) * gridSize;
		}
	}

	// Save triangles, at most can be 5
	for (int i = 0; i < 5; ++i)
	{
		if (TriangleConnectionTable[VertexMask][3 * i] < 0) break;

		auto V1 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i]];
		auto V2 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 1]];
		auto V3 = EdgeVertex[TriangleConnectionTable[VertexMask][3 * i + 2]];

		auto Normal = FVector::CrossProduct(V2 - V1, V3 - V1);
		auto Color = FColor::MakeRandomColor();

		Normal.Normalize();

		MeshData.Vertices.Append({ V3, V2, V1 });

		MeshData.Triangles.Append({
			VertexCount + TriangleOrder[0],
			VertexCount + TriangleOrder[1],
			VertexCount + TriangleOrder[2]
			});

		MeshData.Normals.Append({
			Normal,
			Normal,
			Normal
			});

		MeshData.Colors.Append({
			Color,
			Color,
			Color
			});

		VertexCount += 3;
	}
}

int ACavernGenerator::GetVoxelIndex(int X, int Y, int Z) const
{
	return Z * (z_size + 1) * (y_size + 1) + Y * (x_size + 1) + X;
	//return Z * (z_size) * (y_size) + Y * (x_size) + X;
}

float ACavernGenerator::GetInterpolationOffset(float V1, float V2) const
{
	const float Delta = V2 - V1;
	return Delta == 0.0f ? SurfaceLevel : (SurfaceLevel - V1) / Delta;
}

int ACavernGenerator::getCubeIndex(float Cube[8])
{
	int cubeindex = 0;
	if (Cube[0] < SurfaceLevel) cubeindex |= 1;
	if (Cube[1] < SurfaceLevel) cubeindex |= 2;
	if (Cube[2] < SurfaceLevel) cubeindex |= 4;
	if (Cube[3] < SurfaceLevel) cubeindex |= 8;
	if (Cube[4] < SurfaceLevel) cubeindex |= 16;
	if (Cube[5] < SurfaceLevel) cubeindex |= 32;
	if (Cube[6] < SurfaceLevel) cubeindex |= 64;
	if (Cube[7] < SurfaceLevel) cubeindex |= 128;
	FString print = FString::FromInt(cubeindex);
	//UE_LOG(LogTemp, Warning, TEXT("CubeIndex : %s"), *print);
	return cubeindex;
}

void ACavernGenerator::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinFrequency) || 
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinOctaves)   || 
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, perlinSeed)      ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, x_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, y_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, z_size)          ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, gridSize)        ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, StagNoiseOffset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, PerlinUVScale)   ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACavernGenerator, WallNoiseOffset)       
		) {

		MeshData.Clear();
		//Mesh->ClearAllMeshSections();
		CustomMesh(perlinSeed, 1);
		//ApplyMesh();
		Mesh->UpdateMeshSection_LinearColor(
			0,
			MeshData.Vertices,
			MeshData.Normals,
			MeshData.UV0,
			MeshData.Colors,
			TArray<FProcMeshTangent>(),
			true
			);

		///* Because you are inside the class, you should see the value already changed */
		//if (MyBool) doThings(); // This is how you access MyBool.
		//else undoThings();

		///* if you want to use bool property */
		//UBoolProperty* prop = static_cast<UBoolProperty*>(e.Property);
		//if (prop->GetPropertyValue())
		//	dothings()
		//else
		//	undothings()
	}
}

void ACavernGenerator::GenerateMesh(TArray<TArray<TArray<float>>> matrix, TArray<TArray<int>> marchInd)
{
	// Triangulation order
	if (SurfaceLevel > 0.0f)
	{
		TriangleOrder[0] = 0;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 2;
	}
	else
	{
		TriangleOrder[0] = 2;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 0;
	}

	float Cube[8];

	//for (int n = 0; n < marchInd.Num(); n++) {
	//	int x = marchInd[n][0];
	//	int y = marchInd[n][1];
	//	int z = marchInd[n][2];
	//	for (int i = 0; i < 8; ++i)
	//	{
	//		//Cube[i] = matrix[z + VertexOffset[i][2]][y + VertexOffset[i][1]][x + VertexOffset[i][0]];
	//		Cube[i] = matrix[z][y][x];
	//	}
	//	March(x, y, z, Cube);
	//}

	for (int z = 0; z < z_size-1; z++) {
		for (int y = 0; y < y_size-1; y++) {
			for (int x = 0; x < x_size-1; x++) {
				for (int i = 0; i < 8; ++i)
				{
					//Cube[i] = Voxels[GetVoxelIndex(x + VertexOffset[i][0], y + VertexOffset[i][1], z + VertexOffset[i][2])];
					//Cube[i] = Voxels[GetVoxelIndex(x, y, z)];
					Cube[i] = matrix[z + VertexOffset[i][2]][y + VertexOffset[i][1]][x + VertexOffset[i][0]];
				}

				//getCubeIndex(Cube);
				March(x, y, z, Cube);
			}
		}
	}
}

void ACavernGenerator::GenerateMeshNoised(TArray<TArray<TArray<float>>> matrix, TArray<TArray<TArray<float>>> perlinMatrix, TArray<TArray<int>> marchInd)
{
	// Triangulation order
	if (SurfaceLevel > 0.0f)
	{
		TriangleOrder[0] = 0;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 2;
	}
	else
	{
		TriangleOrder[0] = 2;
		TriangleOrder[1] = 1;
		TriangleOrder[2] = 0;
	}

	float Cube[8];


	for (int z = 0; z < z_size - 1; z++) {
		for (int y = 0; y < y_size - 1; y++) {
			for (int x = 0; x < x_size - 1; x++) {
				for (int i = 0; i < 8; ++i)
				{
					Cube[i] = matrix[z + VertexOffset[i][2]][y + VertexOffset[i][1]][x + VertexOffset[i][0]];
				}

				March( x,  y,  z, Cube);
				//March(perlinMatrix[z][y][x] + x, perlinMatrix[z][y][x] + y, perlinMatrix[z][y][x] + z, Cube);
			}
		}
	}
}



void ACavernGenerator::CustomMesh(int seed, int section)
{
	srand(seed);
	for (int i = 0; i < 4; i++) {
		if (i == 0 || i == 1) {
			TArray<TArray<float>> matrixPerlin = PerlinNoise2D(rand(), z_size * 1.5, y_size);
			CreateSurfaceMatrix(matrixPerlin, i);
		}
		else if (i == 2 || i == 3) {
			TArray<TArray<float>> matrixPerlin = PerlinNoise2D(rand(), x_size * 1.5, y_size);
			CreateSurfaceMatrix(matrixPerlin, i);
		}

	}

	UE_LOG(LogTemp, Warning, TEXT("Mesh set"));
}

void ACavernGenerator::CreateSurfaceMatrix(TArray<TArray<float>>& matrix, int type)
{
	int matrixLen = matrix.Num();
	int matrixBreadth = matrix[0].Num();
	int smallestVal = 200;
	for (int y = 0; y < matrixLen; y++) {
		for (int x = 0; x < matrixBreadth; x++) {
			if (matrix[y][x] <= smallestVal) {
				smallestVal = matrix[y][x];
			}
		}
	}

	// Types 0 - Left wall, 1 - Right wall, 2 - Ceiling, 3 - Ground
	if (type == 0) {
		for (int y = 1; y < y_size - 1; y++) {
			for (int z = 1; z < (z_size * 1.5) - 1; z++) {
				FVector V1 = FVector(matrix[z][y] - smallestVal, float(y * gridSize), float(z * gridSize));
				FVector V2 = FVector(matrix[z][y + 1] - smallestVal, float((y + 1) * gridSize), float(z * gridSize));
				FVector V3 = FVector(matrix[z + 1][y] - smallestVal, float(y * gridSize), float((z + 1) * gridSize));
				FVector V4 = FVector(matrix[z + 1][y + 1] - smallestVal, float((y + 1) * gridSize), float((z + 1) * gridSize));

				auto Normal = FVector::CrossProduct(V4 - V1, V3 - V1);
				auto Normal2 = FVector::CrossProduct(V1 - V4, V2 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V1, V3, V4 });
				MeshData.UV0.Append({
					FVector2D(float(y * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					FVector2D(float(y * gridSize) / PerlinUVScale, float((z + 1) * gridSize) / PerlinUVScale),
					FVector2D(float((y + 1) * gridSize) / PerlinUVScale, float((z + 1) * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal,
					Normal,
					Normal
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;

				MeshData.Vertices.Append({ V4, V2, V1 });
				MeshData.UV0.Append({
					FVector2D(float((y + 1) * gridSize) / PerlinUVScale, float((z + 1) * gridSize) / PerlinUVScale),
					FVector2D(float((y + 1) * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					FVector2D(float(y * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal2,
					Normal2,
					Normal2
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;
			}
		}
	}
	else if (type == 1) {
		for (int y = 1; y < y_size - 1; y++) {
			for (int z = 1; z < (z_size * 1.5) - 1; z++) {
				FVector V1 = FVector(matrix[z][y] + (x_size * gridSize) - smallestVal, float(y * gridSize), float(z * gridSize));
				FVector V2 = FVector(matrix[z][y + 1] + (x_size * gridSize) - smallestVal, float((y + 1) * gridSize), float(z * gridSize));
				FVector V3 = FVector(matrix[z + 1][y] + (x_size * gridSize) - smallestVal, float(y * gridSize), float((z + 1) * gridSize));
				FVector V4 = FVector(matrix[z + 1][y + 1] + (x_size * gridSize) - smallestVal, float((y + 1) * gridSize), float((z + 1) * gridSize));

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V4, V3, V1 });
				MeshData.UV0.Append({
					FVector2D(float((y + 1) * gridSize) / PerlinUVScale, float((z + 1) * gridSize) / PerlinUVScale),
					FVector2D(float(y * gridSize) / PerlinUVScale, float((z + 1) * gridSize) / PerlinUVScale),
					FVector2D(float(y * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal,
					Normal,
					Normal
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;

				MeshData.Vertices.Append({ V1, V2, V4 });
				MeshData.UV0.Append({
					FVector2D(float(y * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					FVector2D(float((y + 1) * gridSize) / PerlinUVScale, float(z * gridSize) / PerlinUVScale),
					FVector2D(float((y + 1)* gridSize) / PerlinUVScale, float((z + 1)* gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal2,
					Normal2,
					Normal2
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;
			}
		}
	}

	else if (type == 2) {
		for (int y = 1; y < y_size - 1; y++) {
			for (int x = 1; x < (x_size*1.5) - 1; x++) {
				FVector V1 = FVector(float(x * gridSize), float(y * gridSize), matrix[y][x] + float(z_size * gridSize) - smallestVal);
				FVector V2 = FVector(float(x * gridSize), float((y + 1) * gridSize), matrix[y+1][x] + float(z_size * gridSize) - smallestVal);
				FVector V3 = FVector(float((x+1)*gridSize), float(y * gridSize), matrix[y][x+1] + float((z_size) * gridSize) - smallestVal);
				FVector V4 = FVector(float((x+1) * gridSize), float((y + 1) * gridSize), matrix[y + 1][x + 1] + float((z_size) * gridSize) - smallestVal);

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V1, V3, V4 });
				MeshData.UV0.Append({
					FVector2D(float(x * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					FVector2D(float((x+1) * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					FVector2D(float((x + 1) * gridSize) / PerlinUVScale, float((y + 1) * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal,
					Normal,
					Normal
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;

				MeshData.Vertices.Append({ V4, V2, V1 });
				MeshData.UV0.Append({
					FVector2D(float((x + 1) * gridSize) / PerlinUVScale, float((y + 1) * gridSize) / PerlinUVScale),
					FVector2D(float(x * gridSize) / PerlinUVScale, float((y + 1) * gridSize) / PerlinUVScale),
					FVector2D(float(x * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal2,
					Normal2,
					Normal2
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;
			}
		}
	}

	else if (type == 3) {
		for (int y = 1; y < y_size - 1; y++) {
			for (int x = 1; x < (x_size * 1.5) - 1; x++) {
				FVector V1 = FVector(float(x * gridSize), float(y * gridSize), matrix[y][x] - smallestVal);
				FVector V2 = FVector(float(x * gridSize), float((y + 1) * gridSize), matrix[y + 1][x] - smallestVal);
				FVector V3 = FVector(float((x + 1) * gridSize), float(y * gridSize), matrix[y][x + 1] - smallestVal);
				FVector V4 = FVector(float((x + 1) * gridSize), float((y + 1) * gridSize), matrix[y + 1][x + 1] - smallestVal);

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V4, V3, V1 });
				MeshData.UV0.Append({
					FVector2D(float((x+1) * gridSize) / PerlinUVScale, float((y+1) * gridSize) / PerlinUVScale),
					FVector2D(float((x+1) * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					FVector2D(float(x * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal,
					Normal,
					Normal
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;

				MeshData.Vertices.Append({ V1, V2, V4 });
				MeshData.UV0.Append({
					FVector2D(float(x * gridSize) / PerlinUVScale, float(y * gridSize) / PerlinUVScale),
					FVector2D(float(x * gridSize) / PerlinUVScale, float((y+1) * gridSize) / PerlinUVScale),
					FVector2D(float((x+1) * gridSize) / PerlinUVScale, float((y+1) * gridSize) / PerlinUVScale),
					});
				MeshData.Triangles.Append({
					VertexCount + 0,
					VertexCount + 1,
					VertexCount + 2,
					});
				MeshData.Normals.Append({
					Normal2,
					Normal2,
					Normal2
					});
				MeshData.Colors.Append({
					Color,
					Color,
					Color
					});
				VertexCount += 3;
			}
		}
	}

}

void ACavernGenerator::ApplyMesh(int section) const
{
	Mesh->SetMaterial(0, Material);
	Mesh->CreateMeshSection_LinearColor(
		section,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.Colors,
		TArray<FProcMeshTangent>(),
		true
	);
}


