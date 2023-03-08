// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TCP_Server.h"
#include "VirtualObstacle.h"
#include "Containers/Map.h"
#include "SceneManager.generated.h"

UCLASS()
class ZED_DETECTION_API ASceneManager : public AActor
{
	GENERATED_BODY()

public:
	ASceneManager();
	virtual void Tick(float DeltaTime) override;
	AVirtualObstacle *GetVirtualObstacle(Detection Det);
	Detection GetLatestDetection(TTuple<int, int> Key);

	// Lets the TCP server listen for a connection from the ZED camera
	UPROPERTY(EditAnywhere)
	bool EnableServer = false;
	// Each pair <idx,mesh> defines what mesh has to be instantiated for objects of class idx
	UPROPERTY(EditAnywhere)
	TArray<UStaticMesh *> ObjectMappings;
	// Materials for the meshes contained in ObjectMappings
	UPROPERTY(EditAnywhere)
	TArray<UMaterialInterface *> MaterialMappings;
	// The tolerance used to consider the scaling across all the axes uniform
	UPROPERTY(EditAnywhere)
	double ScaleStabilityTolerance = 0.1;
	// The lower amount to be considered movement and below which every variation will be considered
	// jittering and smoothed out
	UPROPERTY(EditAnywhere)
	float MovementThreshold = 1.0;
	// The percentage of motion to be kept when the distance covered by an object from a detection to the other
	// is below MovementThreshold
	UPROPERTY(EditAnywhere)
	float MovementDamper = 0.1;
	// The amount of time a virtual obstacle should be kept in the scene if it is not detected again
	UPROPERTY(EditAnywhere)
	double ObjectTimeout = 1.0;
	// The number of frames after which the size of the object
	// can be considered stable and fixable
	UPROPERTY(EditAnywhere)
	int StabilityFrames = 30;
	// Offsets that can be added to the receieved object positions to calibrate the system
	UPROPERTY(EditAnywhere)
	FVector PositionDelta;
	// rotation angles to correct the camera positions, expressed in degrees
	UPROPERTY(EditAnywhere)
	FVector CameraRotationAngles;

	TCP_Server* Server;
	bool ServerInitialized = false;
	int NumClasses;
	TMap<int, TArray<AVirtualObstacle *>> Obstacles;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason);

private:
	void AdjustPositionScale(AVirtualObstacle *Obstacle, Detection Det);
	void UpdateInstances(float DeltaTime);
};
