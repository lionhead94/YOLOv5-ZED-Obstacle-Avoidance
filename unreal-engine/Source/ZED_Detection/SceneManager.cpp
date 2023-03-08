// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneManager.h"

ASceneManager::ASceneManager(){
	PrimaryActorTick.bCanEverTick = true;
	Server = new TCP_Server();
	Obstacles = TMap<int, TArray<AVirtualObstacle*>>();
}

void ASceneManager::BeginPlay(){
	Super::BeginPlay();	
	NumClasses = ObjectMappings.Num();
	if(NumClasses == 0)
		UE_LOG(LogTemp, Warning, TEXT("Warning: no object mapping inserted"));
	for(int i=0; i<NumClasses; ++i)
		Obstacles.Add(i, TArray<AVirtualObstacle*>());
	APawn* CurrentPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	FVector DevicePosition = CurrentPawn->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("Position: %f, %f, %f"), DevicePosition.X, DevicePosition.Y, DevicePosition.Z);
}

void ASceneManager::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	if(EnableServer){
		if(!ServerInitialized){
			Server->Init(NumClasses, PositionDelta, CameraRotationAngles);
			ServerInitialized = true;
			UE_LOG(LogTemp, Warning, TEXT("Server initialized"));
			Server->ZEDConnect();
		} else if(Server->IsZEDConnected()){
			UpdateInstances(DeltaTime);
		} else {
			Server->ZEDConnect();
		}
	}
}

void ASceneManager::EndPlay(const EEndPlayReason::Type Reason){
	Super::EndPlay(Reason);
	if(ServerInitialized)
		Server->Shutdown();
}

void ASceneManager::UpdateInstances(float DeltaTime){
	if(!Server->InstantiationQueue.empty()){
		TTuple<int,int> Key = Server->InstantiationQueue.front();
		Server->InstantiationQueue.pop();
		int Label = Key.Get<0>();
		UE_LOG(LogTemp, Warning, TEXT("Adding object %d (%d)"), Key.Get<1>(), Label);
		AVirtualObstacle* Obstacle = GetWorld()->SpawnActor<AVirtualObstacle>();
		Obstacle->SetKey(Key);
		Obstacle->SetStaticMesh(ObjectMappings[Key.Get<0>()], MaterialMappings[Key.Get<0>()]);
		Obstacle->SetTimer(ObjectTimeout);
		Obstacle->SetLatestUpdate(FDateTime::MinValue());	// force update to set position and scale
		Obstacle->SetDetectionRepository(Server->GetRepo());
		Obstacle->SetScalingMotionParams(ScaleStabilityTolerance, StabilityFrames, MovementThreshold, MovementDamper);
		// Obstacle->EnableObstacleUpdate();
		Obstacles[Key.Get<0>()].Add(Obstacle);
	}
	for(int i=0; i<Obstacles.Num(); ++i){	// for each label
		for(int j=0; j<Obstacles[i].Num(); ++j){	// for each instance
			if(!Obstacles[i][j]->GetStatus()){
				UE_LOG(LogTemp, Warning, TEXT("Deactivating object %d"), Obstacles[i][j]->GetID())
				Obstacles[i][j]->Destroy();
				Obstacles[i].RemoveAtSwap(j);
			}
		}
	}
}