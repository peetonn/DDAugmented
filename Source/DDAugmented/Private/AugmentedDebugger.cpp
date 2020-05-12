// Fill out your copyright notice in the Description page of Project Settings.


#include "AugmentedDebugger.h"
#include "DDLog.h"
#include <Net/UnrealNetwork.h>
#include <Math/UnrealMathUtility.h>

// Sets default values for this component's properties
UAugmentedDebugger::UAugmentedDebugger()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    bReplicates = true;
}

void UAugmentedDebugger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAugmentedDebugger, PlaneRenderer);
    DOREPLIFETIME_CONDITION(UAugmentedDebugger, isRenderingPawn, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UAugmentedDebugger, isRenderingTrackOrigin, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UAugmentedDebugger, isRenderingImages, COND_OwnerOnly);
}

// Called when the game starts
void UAugmentedDebugger::BeginPlay()
{
    Super::BeginPlay();
    
    DLOG_MODULE_TRACE(DDAugmented, "UAugmentedDebugger. role {} remote role {}",
                      GetOwnerRole(), GetOwner()->GetRemoteRole());
    
    // if we are authoritative -- spawn plane renderer. this spawn
    // will be replicated automatically to all clients
    if (GetOwnerRole() >= ROLE_Authority)
    {
        ServerSpawnPlaneRenderer();
        OnNotify_PlaneRendererSpawned();
    }
}


// Called every frame
void UAugmentedDebugger::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UAugmentedDebugger::ServerSpawnPlaneRenderer_Implementation()
{
    DLOG_MODULE_TRACE(DDAugmented, "Spawn PlaneRenderer server-side");
    
    FActorSpawnParameters spawnParams;
    spawnParams.Owner = this->GetOwner();
    
    FVector pos(0);
    FRotator rot(0);
    
    PlaneRenderer = GetWorld()->SpawnActor<AARPlaneRenderer>(pos, rot, spawnParams);
}

void UAugmentedDebugger::OnNotify_PlaneRendererSpawned_Implementation()
{
    // empty. override in the blueprint
    DLOG_MODULE_WARN(DDAugmented, "Native implementation called instead of BP");
}

void UAugmentedDebugger::OnRep_PlaneRenderer()
{
    DLOG_MODULE_TRACE(DDAugmented, "PlaneRenderer replicated");
    OnNotify_PlaneRendererSpawned();
}
    
void UAugmentedDebugger::ServerUpdateTrackingInfo_Implementation(FTrackingInfo tInfo)
{
    TrackingInfo = tInfo;
}

bool UAugmentedDebugger::Equals(const FTrackingInfo& tInfo1, const FTrackingInfo& tInfo2)
{
    return tInfo1.ArSessionStatus == tInfo2.ArSessionStatus &&
    tInfo1.WorldMappingState == tInfo2.WorldMappingState &&
    tInfo1.TrackingQuality == tInfo2.TrackingQuality &&
    tInfo1.PawnToTrackOrigin.Equals(tInfo2.PawnToTrackOrigin, 0.01);
}

void UAugmentedDebugger::ClientSetPawnAdjustment_Implementation(FTransform adjustmentTransform)
{
    if (ArPawn)
    {
        DLOG_MODULE_TRACE(DDAugmented, "Adjusting pawn by {}",
                      TCHAR_TO_ANSI(*adjustmentTransform.ToHumanReadableString()));

        FTransform pawnT = ArPawn->GetActorTransform();
        FTransform newT;
        FTransform::Multiply(&newT, &adjustmentTransform, &pawnT);

        FHitResult res;
        ETeleportType teleport = ETeleportType::None;
        ArPawn->SetActorTransform(newT, false, &res, teleport);
    }
    else
        DLOG_MODULE_WARN(DDAugmented, "Can't adjust pawn -- it is NULL");

}

void UAugmentedDebugger::ServerAddTrackedImage_Implementation(FTrackedImageData tImage)
{
    DLOG_MODULE_TRACE(DDAugmented, "Add New TrackedImage {} - {}, transform: {}",
                     TCHAR_TO_ANSI(*tImage.id_.ToString()),
                     TCHAR_TO_ANSI(*tImage.ImageName),
                     TCHAR_TO_ANSI(*tImage.PawnToImage.ToHumanReadableString()));
    TrackedImages.Add(tImage);
}

void UAugmentedDebugger::ServerRemoveTrackedImage_Implementation(const TArray<FString>& imageIds)
{
    DLOG_MODULE_TRACE(DDAugmented, "Removing {} old tracked image", imageIds.Num());
    
    TrackedImages.RemoveAll([imageIds](FTrackedImageData v){
        return imageIds.Contains(v.id_.ToString());
    });
}

void UAugmentedDebugger::ServerUpdateTrackedImage_Implementation(FTrackedImageData tImage)
{
//    DLOG_MODULE_TRACE(DDAugmented, "Update TrackedImage {} - {}, transform: {}",
//                     TCHAR_TO_ANSI(*tImage.id_.ToString()),
//                     TCHAR_TO_ANSI(*tImage.ImageName),
//                     TCHAR_TO_ANSI(*tImage.PawnToImage.ToHumanReadableString()));
    
    auto* updateImageData =
    TrackedImages.FindByPredicate([&tImage](const FTrackedImageData& img){
        return tImage.id_.ToString().Equals(img.id_.ToString());
    });
    
    if (updateImageData)
    {
        updateImageData->PawnToImage = tImage.PawnToImage;
        updateImageData->EstimatedSize = tImage.EstimatedSize;
        updateImageData->TrackingState = tImage.TrackingState;
        updateImageData->ImageName = tImage.ImageName;
    }
}

FTrackedImageData UAugmentedDebugger::MakeNewTrackedImageData() const
{
    FGuid guid(FMath::RandRange(0,32000),
               FMath::RandRange(0,32000),
               FMath::RandRange(0,32000),
               TrackedImages.Num());
    
    FTrackedImageData data;
    data.id_ = guid;
    return data;
}

