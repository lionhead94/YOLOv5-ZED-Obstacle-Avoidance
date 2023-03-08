// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Detection.h"
#include "Misc/DateTime.h"
#include "CoreMinimal.h"

class ZED_DETECTION_API DetectionRepository{
	public:
		DetectionRepository();
		bool AddDetection(TTuple<int,int> Key, Detection Det);
		Detection GetLatestDetection(TTuple<int,int> Key);
		FDateTime GetLatestTimestamp(TTuple<int,int> Key);
		bool Contains(TTuple<int,int> Key);
		FCriticalSection RepositoryAccess;
	
	private:
		TMap<TTuple<int,int>, TTuple<Detection,FDateTime>> Detections;
};
