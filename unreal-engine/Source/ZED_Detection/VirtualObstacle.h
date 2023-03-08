// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Detection.h"
#include "DetectionRepository.h"
#include "VirtualObstacle.generated.h"

UCLASS()
class ZED_DETECTION_API AVirtualObstacle : public AActor
{
	GENERATED_BODY()
	
	public:	
		UPROPERTY(EditAnywhere)
		UStaticMeshComponent* Mesh;

		AVirtualObstacle();
		virtual void Tick(float DeltaTime) override;
		void SetStaticMesh(UStaticMesh* StaticMesh, UMaterialInterface* Material);
		void SetTimer(float T);
		void SetStatus(bool status);
		void SetKey(int id, int label);
		void SetKey(TTuple<int,int> k);
		void SetLatestUpdate(FDateTime Timestamp);
		void SetDetectionRepository(DetectionRepository* Repo);
		void SetScalingMotionParams(float scaleStabilityTolerance, int stabilityFrames, float movementThreshold, float movementDamper);
		int GetID();
		TTuple<int,int> GetKey();
		float GetTimer();
		bool GetStatus();
		void EnableObstacleUpdate();


	protected:
		virtual void BeginPlay() override;

	private:
		TTuple<int,int> Key;
		float Timer;
		bool Status;
		DetectionRepository* DetectionRepo;
		FDateTime LatestUpdate;
		float BaseTimer;
		float ScaleStabilityTolerance;
		int StabilityFrames;
		int StabilityFrameCounter;
		float MovementThreshold;
		float MovementDamper;
		bool FixedSize = false;
};
