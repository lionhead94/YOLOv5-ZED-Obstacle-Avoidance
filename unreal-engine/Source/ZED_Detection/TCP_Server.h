// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <queue>
#include <fcntl.h>

#include "CoreMinimal.h"
#include "DetectionRepository.h"
#include "Math/Rotator.h"
#include "Detection.h"

#define PORT 2654
#define BUFFER_SIZE 1024

class ZED_DETECTION_API TCP_Server {
	public:
		bool DetectionRunning = false;
	
		TCP_Server();
		~TCP_Server();
		bool Init(int classes, FVector positionDelta, FVector cameraRotationAngles);
		bool ZEDConnect();
		bool IsZEDConnected();
		void Shutdown();
		DetectionRepository* GetRepo();
		std::queue<TTuple<int,int>> InstantiationQueue;
		DetectionRepository DetRepo;
		int NumClasses;
		int ClientSocket;
		FVector PositionDelta;
		FVector CameraRotationAngles;
		FMatrix RotationMatrix;

	private:
		int ServerSocket;
		struct sockaddr_in ServerAddress;
		int ServerAddressLength = sizeof(ServerAddress);
		int Opt = 1;
		bool ZEDConnected = false;
		bool WarnedListening = false;
};
