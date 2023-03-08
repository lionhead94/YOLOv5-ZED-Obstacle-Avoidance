// Fill out your copyright notice in the Description page of Project Settings.


#include "VirtualObstacle.h"

bool IsInTolRange(float n, float tol){
	return (n >= 1-tol && n <= 1+tol);
}

AVirtualObstacle::AVirtualObstacle(){
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Status = true;
}

void AVirtualObstacle::BeginPlay(){
	Super::BeginPlay();
}

void AVirtualObstacle::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	if(Status){
		FDateTime Timestamp = DetectionRepo->GetLatestTimestamp(Key);
		if(Timestamp > LatestUpdate){
			Detection Det = DetectionRepo->GetLatestDetection(Key);
			FVector DetPosition = Det.GetPosition();
			FVector ActorLocation;
			FVector ActorExtent;
			GetActorBounds(false, ActorLocation, ActorExtent, false);
			if((ActorLocation - DetPosition).Length() > MovementThreshold){
				SetActorLocation(FVector(DetPosition.X, DetPosition.Y, 73));
				// SetActorLocation(DetPosition);
			} else {
				SetActorLocation(FVector(
					FMath::Lerp(ActorLocation.X, DetPosition.X, MovementDamper), 
					FMath::Lerp(ActorLocation.Y, DetPosition.Y, MovementDamper), 
					0
				));
			}
			if(!FixedSize){
				FVector ActorScale = GetActorScale3D();
				ActorExtent *= 2;
				std::vector<FVector> BBox = Det.GetBBox();
				// |0-1|, |0,3|. |0-4| left handed z up
				// |0-3|. |0-1|, |0-4| right handed z up
				FVector ObjectExtent = FVector(
					abs(BBox[0].X - BBox[1].X), 
					abs(BBox[0].Y - BBox[3].Y), 
					abs(BBox[0].Z - BBox[4].Z)
				);
				FVector ScaleFactor = ActorExtent / ObjectExtent;
				SetActorScale3D(ActorScale / ScaleFactor);
				if(IsInTolRange(ScaleFactor.X, ScaleStabilityTolerance) && 
					IsInTolRange(ScaleFactor.Y, ScaleStabilityTolerance) && 
					IsInTolRange(ScaleFactor.Z, ScaleStabilityTolerance)) {
					StabilityFrameCounter++;
					if(StabilityFrameCounter >= StabilityFrames){
						UE_LOG(LogTemp, Warning, TEXT("Object %d is now stable"), Key.Get<1>());
						FixedSize = true;
					}
				}
				else
					StabilityFrameCounter = 0;
			}
			LatestUpdate = Timestamp;
			Timer = BaseTimer;
		} else {
			Timer -= DeltaTime;
			if(Timer <= 0.0){
				SetStatus(false);
				PrimaryActorTick.bCanEverTick = false;
				SetActorTickEnabled(false);
			}
		}
	}
}

int AVirtualObstacle::GetID(){
	return Key.Get<1>();
}

float AVirtualObstacle::GetTimer(){
	return Timer;
}

bool AVirtualObstacle::GetStatus(){
	return Status;
}

TTuple<int,int> AVirtualObstacle::GetKey(){
	return Key;
}

void AVirtualObstacle::SetKey(int id, int label){
	TTuple<int,int> k(label, id);
	Key = k;
}

void AVirtualObstacle::SetKey(TTuple<int,int> k){
	Key = k;
}

void AVirtualObstacle::SetStaticMesh(UStaticMesh* StaticMesh, UMaterialInterface* Material){
	Mesh->SetStaticMesh(StaticMesh);
	Mesh->SetMaterial(0, Material);
}

void AVirtualObstacle::SetTimer(float T){
	Timer = T;
	BaseTimer = T;
}

void AVirtualObstacle::SetStatus(bool status){
	Status = status;
}

void AVirtualObstacle::SetDetectionRepository(DetectionRepository* Repo){
	DetectionRepo = Repo;
}

void AVirtualObstacle::SetLatestUpdate(FDateTime Timestamp){
	LatestUpdate = Timestamp;
}

void AVirtualObstacle::SetScalingMotionParams(float scaleStabilityTolerance, int stabilityFrames, float movementThreshold, float movementDamper){
	ScaleStabilityTolerance = scaleStabilityTolerance;
	StabilityFrames = stabilityFrames;
	MovementThreshold = movementThreshold;
	MovementDamper = movementDamper;
}

void AVirtualObstacle::EnableObstacleUpdate(){
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetActorTickEnabled(true);
	UE_LOG(LogTemp, Warning, TEXT("Tick enabled for object %d"), Key.Get<1>());
}