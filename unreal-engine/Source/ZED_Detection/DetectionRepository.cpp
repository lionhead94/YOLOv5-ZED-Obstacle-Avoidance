// Fill out your copyright notice in the Description page of Project Settings.


#include "DetectionRepository.h"

DetectionRepository::DetectionRepository(){
    UE_LOG(LogTemp, Warning, TEXT("Detection repository created."));
}

bool DetectionRepository::AddDetection(TTuple<int,int> Key, Detection Det){
    FScopeLock ScopeLock(&RepositoryAccess);
    TTuple<Detection,FDateTime> data(Det,FDateTime::Now());
    if(Detections.Contains(Key)){
        Detections[Key] = data;
        return false;
    } else {
        Detections.Add(Key, data);
        return true;
    }
}

Detection DetectionRepository::GetLatestDetection(TTuple<int,int> Key){
    FScopeLock ScopeLock(&RepositoryAccess);
    return Detections[Key].Get<0>();
}

FDateTime DetectionRepository::GetLatestTimestamp(TTuple<int,int> Key){
    FScopeLock ScopeLock(&RepositoryAccess);
    return Detections[Key].Get<1>();
}

bool DetectionRepository::Contains(TTuple<int,int> Key){
    FScopeLock ScopeLock(&RepositoryAccess);
    return Detections.Contains(Key);
}