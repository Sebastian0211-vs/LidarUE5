// CameraToRosPublisher.cpp
#include "CameraToRosPublisher.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Async/Async.h"
#include "RenderResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogCameraToROS, Log, All);

UCameraToROSPublisher::UCameraToROSPublisher()
{
    PrimaryComponentTick.bCanEverTick = true;
    Socket = nullptr;
}

void UCameraToROSPublisher::BeginPlay()
{
    Super::BeginPlay();

    if (!RenderTarget || !CaptureComponent)
    {
        UE_LOG(LogCameraToROS, Error, TEXT("RenderTarget or CaptureComponent not assigned."));
        return;
    }

    CaptureComponent->bCaptureEveryFrame = false;
    CaptureComponent->bCaptureOnMovement = false;

    CaptureInterval = 1.0f / SendRate;

    if (!SetupSocketConnection())
    {
        UE_LOG(LogCameraToROS, Error, TEXT("Failed to connect to ROS socket."));
    }
    else
    {
        UE_LOG(LogCameraToROS, Log, TEXT("CameraToROSPublisher has started with %.2f FPS send rate."), 1.f / CaptureInterval);
    }
}

void UCameraToROSPublisher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
    }
    Super::EndPlay(EndPlayReason);
}

bool UCameraToROSPublisher::SetupSocketConnection()
{
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("ROS_Socket"), false);
    RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    bool bIsValid;
    RemoteAddress->SetIp(*RosIPAddress, bIsValid);
    RemoteAddress->SetPort(RosPort);

    if (!bIsValid)
    {
        UE_LOG(LogCameraToROS, Error, TEXT("Invalid IP address."));
        return false;
    }

    return Socket->Connect(*RemoteAddress);
}

void UCameraToROSPublisher::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeSinceLastCapture += DeltaTime;
    if (TimeSinceLastCapture >= CaptureInterval && !bIsSending)
    {
        TimeSinceLastCapture = 0.f;
        CaptureAndSendImage();
    }
}

void UCameraToROSPublisher::CaptureAndSendImage()
{
    if (!RenderTarget) return;

    CaptureComponent->CaptureScene();

    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> OutBMP;
    RTResource->ReadPixels(OutBMP);

    int32 Width = RenderTarget->SizeX;
    int32 Height = RenderTarget->SizeY;

    bIsSending = true;
    Async(EAsyncExecution::Thread, [this, OutBMP, Width, Height]()
        {
            SendImage(OutBMP, Width, Height);
            bIsSending = false;
        });
}

void UCameraToROSPublisher::SendImage(const TArray<FColor>& ImageData, int32 Width, int32 Height)
{
    if (!Socket || Socket->GetConnectionState() != SCS_Connected)
        return;

    FString Encoding = TEXT("rgb8");
    FTCHARToUTF8 Convert(*Encoding);
    const char* EncStr = Convert.Get();
    int32 EncLen = FCStringAnsi::Strlen(EncStr);

    TArray<uint8> RGBData;
    RGBData.Reserve(Width * Height * 3);
    for (const FColor& Pixel : ImageData)
    {
        RGBData.Add(Pixel.R);
        RGBData.Add(Pixel.G);
        RGBData.Add(Pixel.B);
    }

    int32 DataLen = RGBData.Num();

    TArray<uint8> Payload;
    Payload.Reserve(20 + EncLen + DataLen);

    auto AppendInt32BE = [&Payload](int32 Value)
        {
            Payload.Add((Value >> 24) & 0xFF);
            Payload.Add((Value >> 16) & 0xFF);
            Payload.Add((Value >> 8) & 0xFF);
            Payload.Add(Value & 0xFF);
        };

    AppendInt32BE(Width);
    AppendInt32BE(Height);
    AppendInt32BE(EncLen);
    Payload.Append(reinterpret_cast<const uint8*>(EncStr), EncLen);
    AppendInt32BE(DataLen);
    Payload.Append(RGBData);

    int32 Sent = 0;
    bool bSuccess = Socket->Send(Payload.GetData(), Payload.Num(), Sent);

    UE_LOG(LogCameraToROS, Log, TEXT("Sent %d bytes (image %dx%d, format=%s, success=%s)"),
        Sent, Width, Height, *Encoding, bSuccess ? TEXT("true") : TEXT("false"));
}
