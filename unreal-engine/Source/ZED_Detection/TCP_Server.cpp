// Fill out your copyright notice in the Description page of Project Settings.


#include "TCP_Server.h"

// FCriticalSection CriticalStructureUpdate;
char Buffer[BUFFER_SIZE] = {0};

Detection ParseMessage(FVector PositionDelta, FMatrix RotationMatrix){
    std::stringstream ssGlobal(Buffer);
	std::string tokenGlobal;
	std::string tokenLocal;
    int label, id;
    FVector position;
    std::vector<FVector> bbox;

    if(std::getline(ssGlobal, tokenGlobal, ';')){
        if(!tokenGlobal.empty()){
            try {
                id = std::stoi(tokenGlobal);
            }
            catch (std::invalid_argument ia) {
                UE_LOG(LogTemp, Error, TEXT("ERROR (invalid argument): no str->int conversion can be performed. Argument: %s"), tokenGlobal.c_str());
                return Detection();
            }
            catch (std::out_of_range oor) {
                UE_LOG(LogTemp, Error, TEXT("ERROR (out of range): no str->int conversion can be performed. Argument: %s"), tokenGlobal.c_str());
                return Detection();
            }
            if(std::getline(ssGlobal, tokenGlobal, ';')){
                if(!tokenGlobal.empty()){
                    try {
                        label = std::stoi(tokenGlobal);
                    }
                    catch (std::invalid_argument ia) {
                        UE_LOG(LogTemp, Error, TEXT("ERROR (invalid argument): no str->int conversion can be performed. Argument: %s"), tokenGlobal.c_str());
                        return Detection();
                    }
                    catch (std::out_of_range oor) {
                        UE_LOG(LogTemp, Error, TEXT("ERROR (out of range): no str->int conversion can be performed. Argument: %s"), tokenGlobal.c_str());
                        return Detection();
                    }
                    // FString l = std::to_string(label).c_str();
                    // UE_LOG(LogTemp, Warning, TEXT("Label: %s"), *l);
                    if(std::getline(ssGlobal, tokenGlobal, ';')){
                        if(!tokenGlobal.empty()){
                            // FString s = tokenGlobal.c_str();
                            // UE_LOG(LogTemp, Warning, TEXT("Position: (%s)"), *s);
                            std::stringstream ssLocal(tokenGlobal);
                            std::vector<float> point;
                            while(std::getline(ssLocal, tokenLocal, ',')){
                                if(!tokenLocal.empty()){
                                    try{
                                        point.push_back(std::stof(tokenLocal));
                                    } 
                                    catch (std::invalid_argument ia) {
                                        UE_LOG(LogTemp, Error, TEXT("ERROR (invalid argument): no str->float conversion can be performed. Argument: %s"), tokenLocal.c_str());
                                        return Detection();
                                    }
                                    catch (std::out_of_range oor) {
                                        UE_LOG(LogTemp, Error, TEXT("ERROR (out of range): no str->float conversion can be performed. Argument: %s"), tokenLocal.c_str());
                                        return Detection();
                                    }
                                }
                            }
                            position = RotationMatrix.TransformVector(FVector(point[0]+PositionDelta.X, point[1]+PositionDelta.Y, point[2]+PositionDelta.Z));
                        }
                    }
                    while(std::getline(ssGlobal, tokenGlobal, ';')){
                        if(!tokenGlobal.empty()){
                            // FString s = tokenGlobal.c_str();
                            // UE_LOG(LogTemp, Warning, TEXT("%s"), *s);
                            std::stringstream ssLocal(tokenGlobal);
                            std::vector<float> point;
                            while(std::getline(ssLocal, tokenLocal, ',')){
                                if(!tokenLocal.empty()){
                                    try{
                                        point.push_back(std::stof(tokenLocal));
                                    } 
                                    catch (std::invalid_argument ia) {
                                        UE_LOG(LogTemp, Error, TEXT("ERROR (invalid argument): no str->float conversion can be performed. Argument: %s"), tokenLocal.c_str());
                                        return Detection();
                                    }
                                    catch (std::out_of_range oor) {
                                        UE_LOG(LogTemp, Error, TEXT("ERROR (out of range): no str->float conversion can be performed. Argument: %s"), tokenLocal.c_str());
                                        return Detection();
                                    }
                                }
                            }
                            bbox.push_back(FVector3d(point[0], point[1], point[2]));
                        }
                    }
                    return Detection(id, label, position, bbox);
                }
            }
        }
    }
    return Detection();
}

void UpdateStructures(TCP_Server* Server, TTuple<int,int> Key, Detection Det){
    // FScopeLock ScopeLock(&CriticalStructureUpdate);
    if(Server->DetRepo.AddDetection(Key, Det))  // true if key is fresh
        Server->InstantiationQueue.push(Key);
}

void ReceiveDetections(TCP_Server* Server){
    UE_LOG(LogTemp, Warning, TEXT("Receiving thread started"));
    while(Server->DetectionRunning){
        memset(Buffer, 0, sizeof(Buffer));
        int c = recv(Server->ClientSocket, Buffer, BUFFER_SIZE, 0);
        if(c > 0){
            Detection Det = ParseMessage(Server->PositionDelta, Server->RotationMatrix);
            int label = Det.GetLabel();
            int id = Det.GetID();
            if(Det.IsValid() && Server->NumClasses > 0 && label < Server->NumClasses){
                TTuple<int,int> Key(label, id);
                UpdateStructures(Server, Key, Det);
            }
            else{
                UE_LOG(LogTemp, Error, TEXT("Received not valid message"));
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Receiving thread stopped"));
}

TCP_Server::TCP_Server(){
    UE_LOG(LogTemp, Warning, TEXT("Server created"));
}

bool TCP_Server::Init(int classes, FVector positionDelta, FVector cameraRotationAngles){
    UE_LOG(LogTemp, Warning, TEXT("Initializing server..."))
    
    NumClasses = classes;
    PositionDelta = positionDelta;
    CameraRotationAngles = cameraRotationAngles;
    RotationMatrix = FRotationMatrix(FRotator(CameraRotationAngles.X, CameraRotationAngles.Y, CameraRotationAngles.Z));
    
    
    // Creating socket file descriptor
	if ((ServerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		UE_LOG(LogTemp, Error, TEXT("Socket generation failed."));
        return false;
    }

    // setting non-blocking mode
    int flags = fcntl(ServerSocket, F_GETFL, 0);
	fcntl(ServerSocket, F_SETFL, flags | O_NONBLOCK);

    // setting up server address structure
    bzero((char*) &ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.s_addr = INADDR_ANY;
	ServerAddress.sin_port = htons(PORT);

	// Binding socket to the port
	if (bind(ServerSocket, (struct sockaddr*)&ServerAddress, ServerAddressLength) < 0){
		UE_LOG(LogTemp, Error, TEXT("Socket binding failed, errno=%d"), errno);
        return false;
    }

    // begin listening on socket
	if (listen(ServerSocket, 1) < 0) {
        UE_LOG(LogTemp, Error, TEXT("Connection listening failed."));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("Server up, listening..."));
    return true;
}

bool TCP_Server::ZEDConnect(){
    if(!ZEDConnected){
        ClientSocket = accept(ServerSocket, (struct sockaddr*)&ServerAddress, (socklen_t*)&ServerAddressLength);
        if(ClientSocket >= 0){
            ZEDConnected = true;
            DetectionRunning = true;
            UE_LOG(LogTemp, Warning, TEXT("ZED connected!"));
            Async(EAsyncExecution::Thread, [=]() {ReceiveDetections(this);});
        } else if(errno == EAGAIN || errno == EWOULDBLOCK){
            if(!WarnedListening){
                UE_LOG(LogTemp, Warning, TEXT("Waiting for ZED connecting..."));
                WarnedListening = true;
            }
        } else {
            UE_LOG(LogTemp, Error, TEXT("Accept failed."));
            close(ClientSocket);
            return false;
        }
    }
    return true;
}

bool TCP_Server::IsZEDConnected(){
    return ZEDConnected;
}

void TCP_Server::Shutdown(){
    DetectionRunning = false;
    close(ServerSocket);
    close(ClientSocket);
}

TCP_Server::~TCP_Server(){
    UE_LOG(LogTemp, Warning, TEXT("Server down"));
}

DetectionRepository* TCP_Server::GetRepo(){
    return &DetRepo;
}