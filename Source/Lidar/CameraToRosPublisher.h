// CameraToRosPublisher.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "HAL/RunnableThread.h"
#include "CameraToRosPublisher.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LIDAR_API UCameraToROSPublisher : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraToROSPublisher();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool SetupSocketConnection();
    void CaptureAndSendImage();
    void SendImage(const TArray<FColor>& ImageData, int32 Width, int32 Height);

    FSocket* Socket;
    TSharedPtr<FInternetAddr> RemoteAddress;
    FThreadSafeBool bIsSending = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UTextureRenderTarget2D* RenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USceneCaptureComponent2D* CaptureComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROS", meta = (AllowPrivateAccess = "true"))
    FString RosIPAddress = "192.168.137.53";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROS", meta = (AllowPrivateAccess = "true"))
    int32 RosPort = 9999;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROS", meta = (AllowPrivateAccess = "true"))
    float SendRate = 30.0f;

    float TimeSinceLastCapture = 0.0f;
    float CaptureInterval = 0.1f;
};
