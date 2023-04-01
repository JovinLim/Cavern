// Fill out your copyright notice in the Description page of Project Settings.


#include "CavernGenerator.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "Utils/VoxelMeshData.h"
#include "ProceduralMeshComponent.h"

template <typename ObjClass>
static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path)
{
	if (Path == NAME_None) return nullptr;

	return Cast<ObjClass>(StaticLoadObject(ObjClass::StaticClass(), nullptr, *Path.ToString()));
}

static FORCEINLINE UMaterial* LoadMaterialFromPath(const FName& Path)
{
	if (Path == NAME_None) return nullptr;

	return LoadObjFromPath<UMaterial>(Path);
}

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

	// Get material

	TArray<TArray<TArray<float>>> matrix = GenerateMatrix();
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
	GenPerlinStag(matrix);
	//ApplyMesh(0);

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
			TArray<TArray<float>> matrixPerlin = PerlinNoise2D(rand(), Py, Pz, WallNoiseOffset, 0);
			CreateSurfaceMatrix(matrixPerlin, i, 0);
		}
		else if (i == 2 || i == 3) {
			TArray<TArray<float>> matrixPerlin = PerlinNoise2D(rand(), Px, Py, WallNoiseOffset, 0);
			CreateSurfaceMatrix(matrixPerlin, i, 0);
		}

	}

	UE_LOG(LogTemp, Warning, TEXT("Mesh set"));
}

void ACavernGenerator::CreateSurfaceMatrix(TArray<TArray<float>>& matrix, int type, int section)
{
	int matrixLen = matrix.Num();
	int matrixBreadth = matrix[0].Num();
	int smallestVal = 500;
	int largestVal = 0;
	for (int y = 0; y < matrixLen; y++) {
		for (int x = 0; x < matrixBreadth; x++) {
			if (matrix[y][x] <= smallestVal) {
				smallestVal = matrix[y][x];
			}
			if (matrix[y][x] >= largestVal) {
				largestVal = matrix[y][x];
			}
		}
	}

	// Types 0 - Right wall, 1 - Left wall, 2 - Ceiling, 3 - Ground, 4 - Stag
	if (type == 0) {
		for (int z = 0; z < matrix.Num() - 1; z++) {
			for (int y = 0; y < matrix[0].Num() - 1; y++) {
				FVector V1 = FVector(matrix[z][y], float(y * Pgrid), float(z * Pgrid));
				FVector V2 = FVector(matrix[z][y + 1], float((y + 1) * Pgrid), float(z * Pgrid));
				FVector V3 = FVector(matrix[z + 1][y], float(y * Pgrid), float((z + 1) * Pgrid));
				FVector V4 = FVector(matrix[z + 1][y + 1], float((y + 1) * Pgrid), float((z + 1) * Pgrid));

				auto Normal = FVector::CrossProduct(V4 - V1, V3 - V1);
				auto Normal2 = FVector::CrossProduct(V1 - V4, V2 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V1, V3, V4 });
				MeshData.UV0.Append({
					FVector2D(float(y * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
					FVector2D(float(y * Pgrid) / PerlinUVScale, float((z + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float((y + 1) * Pgrid) / PerlinUVScale, float((z + 1) * Pgrid) / PerlinUVScale),
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
					FVector2D(float((y + 1) * Pgrid) / PerlinUVScale, float((z + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float((y + 1) * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
					FVector2D(float(y * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
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
		for (int z = 0; z < matrix.Num() - 1; z++) {
			for (int y = 0; y < matrix[0].Num() - 1; y++) {
				FVector V1 = FVector(matrix[z][y] + ((Px - (gridSize / Pgrid)) * Pgrid), float(y * Pgrid), float(z * Pgrid));
				FVector V2 = FVector(matrix[z][y + 1] + ((Px - (gridSize / Pgrid)) * Pgrid), float((y + 1) * Pgrid), float(z * Pgrid));
				FVector V3 = FVector(matrix[z + 1][y] + ((Px - (gridSize / Pgrid)) * Pgrid), float(y * Pgrid), float((z + 1) * Pgrid));
				FVector V4 = FVector(matrix[z + 1][y + 1] + ((Px - (gridSize / Pgrid)) * Pgrid), float((y + 1) * Pgrid), float((z + 1) * Pgrid));

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V4, V3, V1 });
				MeshData.UV0.Append({
					FVector2D(float((y + 1) * Pgrid) / PerlinUVScale, float((z + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float(y * Pgrid) / PerlinUVScale, float((z + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float(y * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
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
					FVector2D(float(y * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
					FVector2D(float((y + 1) * Pgrid) / PerlinUVScale, float(z * Pgrid) / PerlinUVScale),
					FVector2D(float((y + 1)* Pgrid) / PerlinUVScale, float((z + 1)* Pgrid) / PerlinUVScale),
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
		for (int y = 0; y < matrix.Num() - 1; y++) {
			for (int x = 0; x < matrix[0].Num() - 1; x++) {
				FVector V1 = FVector(float(x * Pgrid), float(y * Pgrid), matrix[y][x] + float((Pz - (gridSize / Pgrid)) * Pgrid));
				FVector V2 = FVector(float(x * Pgrid), float((y + 1) * Pgrid), matrix[y+1][x] + float((Pz - (gridSize / Pgrid)) * Pgrid));
				FVector V3 = FVector(float((x+1)* Pgrid), float(y * Pgrid), matrix[y][x+1] + float((Pz - (gridSize / Pgrid)) *Pgrid));
				FVector V4 = FVector(float((x+1) * Pgrid), float((y + 1) * Pgrid), matrix[y + 1][x + 1] + float((Pz - (gridSize / Pgrid)) * Pgrid));

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V1, V3, V4 });
				MeshData.UV0.Append({
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float((x+1) * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float((x + 1) * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
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
					FVector2D(float((x + 1) * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
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
		for (int y = 0; y < matrix.Num() - 1; y++) {
			for (int x = 0; x < matrix[0].Num() - 1; x++) {
				FVector V1 = FVector(float(x * Pgrid), float(y * Pgrid), matrix[y][x] );
				FVector V2 = FVector(float(x * Pgrid), float((y + 1) * Pgrid), matrix[y + 1][x] );
				FVector V3 = FVector(float((x + 1) * Pgrid), float(y * Pgrid), matrix[y][x + 1] );
				FVector V4 = FVector(float((x + 1) * Pgrid), float((y + 1) * Pgrid), matrix[y + 1][x + 1] );

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V4, V3, V1 });
				MeshData.UV0.Append({
					FVector2D(float((x+1) * Pgrid) / PerlinUVScale, float((y+1) * Pgrid) / PerlinUVScale),
					FVector2D(float((x+1) * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
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
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float((y+1) * Pgrid) / PerlinUVScale),
					FVector2D(float((x+1) * Pgrid) / PerlinUVScale, float((y+1) * Pgrid) / PerlinUVScale),
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

	else if (type == 4) {
		for (int y = 0; y < matrix.Num() - 1; y++) {
			for (int x = 0; x < matrix[0].Num() - 1; x++) {
				FVector V1 = FVector(float(x * Pgrid), float(y * Pgrid), matrix[y][x] - smallestVal);
				FVector V2 = FVector(float(x * Pgrid), float((y + 1) * Pgrid), matrix[y + 1][x] - smallestVal);
				FVector V3 = FVector(float((x + 1) * Pgrid), float(y * Pgrid), matrix[y][x + 1] - smallestVal);
				FVector V4 = FVector(float((x + 1) * Pgrid), float((y + 1) * Pgrid), matrix[y + 1][x + 1] - smallestVal);

				auto Normal = FVector::CrossProduct(V3 - V1, V4 - V1);
				auto Normal2 = FVector::CrossProduct(V2 - V4, V1 - V4);
				auto Color = FLinearColor::MakeRandomColor();

				Normal.Normalize();
				Normal2.Normalize();

				MeshData.Vertices.Append({ V4, V3, V1 });
				MeshData.UV0.Append({
					FVector2D(float((x + 1) * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float((x + 1) * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
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
					FVector2D(float(x * Pgrid) / PerlinUVScale, float(y * Pgrid) / PerlinUVScale),
					FVector2D(float(x * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
					FVector2D(float((x + 1) * Pgrid) / PerlinUVScale, float((y + 1) * Pgrid) / PerlinUVScale),
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

	ApplyMesh(section);
}

void ACavernGenerator::GenPerlinStag(TArray<TArray<TArray<float>>>& matrix)
{
	// Setting random seed
	srand(stagSeed);

	// Calculate max grid cells occupied by stags on ground
	int maxStagGrid = (x_size - 1) * (y_size - 1) * float((100.0 - EmptySpace) / 100.0);
	//FString print2 = FString::FromInt(maxStagGrid);
	//UE_LOG(LogTemp, Warning, TEXT("max number of grids : %s"), *print2);

	// Making copy of ground matrix points to check against; prevent duplicate points from being chosen
	// 0 - Empty space, 1 - Chosen for stag (unmerged), 2 - Merged for stag
	TArray<TArray<float>> matrixCheck;
	for (int y = 0; y < matrix[0].Num() - 1; y++) {
		TArray<float> matrixChecky;
		for (int x = 0; x < matrix[0][0].Num() - 1; x++) {
			matrixChecky.Add(0);
		}
		matrixCheck.Add(matrixChecky);
	}

	// Set variable to check stag grid cell count
	int stagGridCount = 0;

	while (stagGridCount < maxStagGrid) {

		// Check if grid cell has already been chosen
		int randXind = rand() % (x_size - 1);
		int randYind = rand() % (y_size - 1);
		int randSize = rand() % 3 + 1;
		if (matrixCheck[randYind][randXind] == 1) {
			continue;
		}

		// Pick grid cells on z = 0
		else {
			if (randYind + randSize > (y_size - 1) || randXind + randSize > (x_size - 1)) {
				continue;
			}
			else {
				TArray<TArray<float>> srfMatrix;
				for (int y = 0; y < randSize; y++) {
					for (int x = 0; x < randSize; x++) {
						matrixCheck[randYind + y][randXind + x] = 1;
					}
				}

				TArray<TArray<float>> matrixPerlin = PerlinNoise2D(rand(), ((gridSize / Pgrid) * randSize), ((gridSize / Pgrid) * randSize), StagNoiseOffset, 1);
				for (int y = 0; y < ((gridSize / Pgrid) * randSize); y++) {
					TArray<float> srfMatrixY;
					for (int x = 0; x < ((gridSize / Pgrid) * randSize); x++) {
						if (y == 0 || x == 0 || y == ((gridSize / Pgrid) * randSize) - 1 || x == ((gridSize / Pgrid) * randSize) - 1) {
							srfMatrixY.Add(0);
						}
						else {
							srfMatrixY.Add(matrixPerlin[y][x]);
						}
					}
					srfMatrix.Add(srfMatrixY);
				}
				CreateSurfaceMatrix(matrixPerlin, 4, 1);

			}

			stagGridCount += randSize*randSize;
		}
	}

	PrintMatrix(matrixCheck);

	//FString print = FString::FromInt(stagGridCount);
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);

	// Form stags on chosen grid cells
	for (int y = 0; y < matrixCheck.Num(); y++) {
		for (int x = 0; x < matrixCheck[0].Num(); x++) {
			if (matrixCheck[y][x] == 2 || matrixCheck[y][x] == 0) {
				continue;
			}
			else {

			}
		}

	}
}

void ACavernGenerator::ApplyMesh(int section) const
{
	// Section guide
	// 0 - Main room meshes, 1 - stag meshes

	FString sPath = "/Script/Engine.Material'/Game/StarterContent/Materials/M_Concrete_Grime_Scaleable.M_Concrete_Grime_Scaleable'";
	UMaterial* mat = LoadMaterialFromPath(FName(*sPath));
	Mesh->SetMaterial(0, mat);

	//Check if mesh is double sided; 0 or 1;
	//FString print = FString::FromInt(Mesh->GetBodySetup()->bDoubleSidedGeometry);
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *print);

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

