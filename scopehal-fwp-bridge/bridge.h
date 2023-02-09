#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>

#pragma pack(push, 4)
struct WaveformHeader
{
	int version;
	int flags;
	int headerSize;
	int windowSize;
	int numSamples;
	int segmentIndex;
	int numSweeps;
	int reserved;
	double verticalGain;
	double verticalOffset;
	double verticalResolution;
	double horizontalInterval;
	double horizontalOffset;
	double horizontalResolution;
	int64_t trigTime;
	char verticalUnit[48];
	char horizontalUnit[48];
};
#pragma pack(pop)