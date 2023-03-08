// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include <vector>

class ZED_DETECTION_API Detection
{
	public:
		Detection();
		Detection(int id, int l, FVector p, std::vector<FVector> bb);
		int GetID();
		int GetLabel();
		FVector GetPosition();
		std::vector<FVector> GetBBox();
		bool IsValid();
	
	private:
		int ID; 
		int Label;
		FVector Position;
		std::vector<FVector> BoundingBox;
};
