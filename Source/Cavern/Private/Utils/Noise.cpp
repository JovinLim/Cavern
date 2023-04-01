#include "CavernGenerator.h"
#include <iostream>
#include <stdlib.h>
#include <random>


int ACavernGenerator::Noise(int seed)
{
    srand(seed);
    return (rand());
}


float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float map(float val, float ogMin, float ogMax, float newMin, float newMax) {
    float prop = (val - ogMin) / (ogMax - ogMin);
    return lerp(prop, newMin, newMax);
}

float grad(unsigned char hash, float x, float y, float z) {
    int h = hash & 0b1111;
    float u = h < 0b1000 ? x : y;
    float v = h < 0b0100 ? y : h == 0b1100 || h == 0b1110 ? x : z;
    return ((h & 0b0001) == 0 ? u : -u) + ((h & 0b0010) == 0 ? v : -v);
}

// Perlin Noise
TArray<TArray<float>> ACavernGenerator::PerlinNoise2D(int seed, int x, int y, int offset, int mode) {

    TArray<TArray<float>> PerlinMatrix;
    int _seed = seed;

    // Populate permutation table
    for (unsigned int i = 0; i < 256; i++) {
        p[i] = i;
    }

    // Shuffle array
    std::shuffle(std::begin(p), std::begin(p) + 256, std::default_random_engine(seed));

    // Duplicate array for overflow
    for (unsigned int i = 0; i < 256; i++) {
        p[256 + i] = p[i];
    }

    for (int y_ = 0; y_ < y; y_++) {
        TArray<float> PerlinMatrixY;
        for (int x_ = 0; x_ < x; x_++) {
            float noiseVal = (accumulatedNoise2D(float(x_ / perlinFrequency), float(y_ / perlinFrequency), perlinOctaves, mode)) * offset;
            FString print = FString::SanitizeFloat(noiseVal);
            PerlinMatrixY.Add(noiseVal);
        }
        PerlinMatrix.Add(PerlinMatrixY);
    }

    
    return PerlinMatrix;
}

TArray<TArray<TArray<float>>> ACavernGenerator::PerlinNoise3D(int seed, int x, int y, int z)
{
    TArray<TArray<TArray<float>>> PerlinMatrix;
    int _seed = seed;

    // Populate permutation table
    for (unsigned int i = 0; i < 256; i++) {
        p[i] = i;
    }

    // Shuffle array
    std::shuffle(std::begin(p), std::begin(p) + 256, std::default_random_engine(seed));

    // Duplicate array for overflow
    for (unsigned int i = 0; i < 256; i++) {
        p[256 + i] = p[i];
    }

    for (int z_ = 0; z_ < z; z_++) {
        TArray<TArray<float>> PerlinMatrixZ;
        for (int y_ = 0; y_ < y; y_++) {
            TArray<float> PerlinMatrixY;
            for (int x_ = 0; x_ < x; x_++) {
                float noiseVal = (accumulatedNoise3D(float(x_ / perlinFrequency), float(y_ / perlinFrequency), float(z_ / perlinFrequency), perlinOctaves)) * StagNoiseOffset;
                FString print = FString::SanitizeFloat(noiseVal);
                //UE_LOG(LogTemp, Warning, TEXT("%s"), *print);
                PerlinMatrixY.Add(noiseVal);
            }
            PerlinMatrixZ.Add(PerlinMatrixY);
        }
        PerlinMatrix.Add(PerlinMatrixZ);
    }

    return PerlinMatrix;
}


float ACavernGenerator::noise2D(float x, float y, int mode) {

    // Mode guide
    // 0 - walls, 1 - stag

    int xi = (int)(std::floorf(x)) & 255;
    int yi = (int)(std::floorf(y)) & 255;

    x -= std::floorf(x);
    y -= std::floorf(y);

    float sx = fade(x);
    float sy = fade(y);

    unsigned char aa, ab, ba, bb;
    aa = p[p[xi] + yi];
    ab = p[p[xi] + yi + 1];
    ba = p[p[xi + 1] + yi];
    bb = p[p[xi + 1] + yi + 1];

    float avg = lerp(
        sy,
        lerp( // "top"
            sx,
            grad(aa, x, y, 0),
            grad(ba, x - 1, y, 0)
        ),
        lerp( // "bottom"
            sx,
            grad(ab, x, y - 1, 0),
            grad(bb, x - 1, y - 1, 0)
        )
    );

    if (mode == 0) {
        return avg;
    }

    else {
        // return avg mapped from [-1, 1] (theoretically) to [0, 1]
        return map(avg, -1, 1, 0, 1);
    }

}

float ACavernGenerator::noise3D(float x, float y, float z)
{
    // find smallest point of cube containing target
    int xi = (int)(std::floorf(x)) & 255;
    int yi = (int)(std::floorf(y)) & 255;
    int zi = (int)(std::floorf(z)) & 255;

    // get decimal value of each component
    x -= std::floorf(x);
    y -= std::floorf(y);
    z -= std::floorf(z);

    // get smooth value from fade function (becomes weight for each dimension)
    float sx = fade(x);
    float sy = fade(y);
    float sz = fade(z);

    // get hash value for all neighboring points
    unsigned char aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = p[p[p[xi] + yi] + zi];
    aba = p[p[p[xi] + yi + 1] + zi];
    aab = p[p[p[xi] + yi] + zi + 1];
    abb = p[p[p[xi] + yi + 1] + zi + 1];
    baa = p[p[p[xi + 1] + yi] + zi];
    bba = p[p[p[xi + 1] + yi + 1] + zi];
    bab = p[p[p[xi + 1] + yi] + zi + 1];
    bbb = p[p[p[xi + 1] + yi + 1] + zi + 1];

    // get weighted average
    float avg = lerp(
        sz,
        lerp( // "front"
            sy,
            lerp( // "top"
                sx,
                grad(aaa, x, y, z),
                grad(baa, x - 1, y, z)
            ),
            lerp( // "bottom"
                sx,
                grad(aba, x, y - 1, z),
                grad(bba, x - 1, y - 1, z)
            )
        ),
        lerp( // "rear"
            sy,
            lerp( // "top"
                sx,
                grad(aab, x, y, z - 1),
                grad(bab, x - 1, y, z)
            ),
            lerp( // "bottom"
                sx,
                grad(abb, x, y - 1, z - 1),
                grad(bbb, x - 1, y - 1, z - 1)
            )
        )
    );

    // return avg mapped from [-1, 1] (theoretically) to [0, 1]
    return map(avg, -1, 1, 0, 1);
}


// 2D accumulated noise
float ACavernGenerator::accumulatedNoise2D(float x, float y, int octaves, int mode) {

    // Mode guide
    // 0 - walls, 1 - stag

    float result = 0.0f;
    float amplitude = 1.3f;
    float frequency = 0.1f;
    float lacunarity = 2.0f;
    float gain = 1.0f;
    float maxVal = 0.0f; // used to normalize result

    for (; octaves > 0; octaves--) {
        result += noise2D(x * frequency, y * frequency, mode) * amplitude;
        //result += noise2D(x * frequency, y * frequency);


        maxVal += amplitude;

        amplitude *= gain;
        frequency *= lacunarity;
    }

    //FString print = FString::SanitizeFloat(result/maxVal);
    //UE_LOG(LogTemp, Warning, TEXT("%s"), *print);

    // return normalized result
    return result / maxVal;
}

float ACavernGenerator::accumulatedNoise3D(float x, float y, float z, int octaves)
{
    float result = 0.0f;
    float amplitude = 1.3f;
    float frequency = 0.1f;
    float lacunarity = 2.0f;
    float gain = 1.0f;
    float maxVal = 0.0f; // used to normalize result

    for (; octaves > 0; octaves--) {
        result += noise3D(x * frequency, y * frequency, z * frequency) * amplitude;

        maxVal += amplitude;

        amplitude *= gain;
        frequency *= lacunarity;
    }

    // return normalized result
    return result / maxVal;
}


