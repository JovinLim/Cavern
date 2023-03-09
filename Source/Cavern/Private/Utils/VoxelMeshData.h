#pragma once

#include "CoreMinimal.h"
#include "VoxelMeshData.generated.h"

USTRUCT()
struct FVoxelMeshData
{
	GENERATED_BODY();

public:
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector> Normals;
	TArray<FLinearColor> Colors;
	TArray<FVector2D> UV0;

	void Clear();
};

inline void FVoxelMeshData::Clear()
{
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	Colors.Empty();
	UV0.Empty();
}