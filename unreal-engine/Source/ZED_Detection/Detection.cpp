// Fill out your copyright notice in the Description page of Project Settings.


#include "Detection.h"

Detection::Detection(){
    ID = -1;
    Label = -1;
    Position = FVector(0.0, 0.0, 0.0);
}

Detection::Detection(int id, int l, FVector p, std::vector<FVector3d> bb){
    ID = id;
    Label = l;
    Position = p;
    BoundingBox = bb;
}

int Detection::GetID(){
    return ID;
}

int Detection::GetLabel(){
    return Label;
}

FVector Detection::GetPosition(){
    return Position;
}

std::vector<FVector> Detection::GetBBox(){
    return BoundingBox;
}

bool Detection::IsValid(){
    return Label > -1 && ID > -1;
}