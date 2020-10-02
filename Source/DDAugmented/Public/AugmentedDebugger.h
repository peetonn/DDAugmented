// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPlaneRenderer.h"
#include "Misc/Guid.h"

#include "AugmentedDebugger.generated.h"

USTRUCT(Blueprintable)
struct FTrackingInfo {
    GENERATED_BODY();
    
    UPROPERTY(BlueprintReadWrite)
    FTransform PawnToTrackOrigin;
    
    UPROPERTY(BlueprintReadWrite)
    EARSessionStatus ArSessionStatus;
    
    UPROPERTY(BlueprintReadWrite)
    FString SessionStatusInfo;
    
    UPROPERTY(BlueprintReadWrite)
    EARTrackingQuality TrackingQuality;
    
    UPROPERTY(BlueprintReadWrite)
    EARWorldMappingState WorldMappingState;
    
    UPROPERTY(BlueprintReadWrite)
    FTransform TrackingAlignment;
};

USTRUCT(Blueprintable)
struct FTrackedImageData {
    GENERATED_BODY();
    
    UPROPERTY(BlueprintReadWrite)
    FTransform PawnToImage;
    
    UPROPERTY(BlueprintReadWrite)
    FVector2D EstimatedSize;
    
    UPROPERTY(BlueprintReadWrite)
    EARTrackingState TrackingState;
    
    UPROPERTY(BlueprintReadWrite)
    FString ImageName;
    
    UPROPERTY(BlueprintReadWrite)
    FGuid id_;
    
    UPROPERTY(BlueprintReadWrite)
    bool PickedForEstimation;
};

UCLASS(ClassGroup=(DDAugmentedUI),Blueprintable, meta=(BlueprintSpawnableComponent))
class DDAUGMENTED_API UTrackedGeoListItem : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FText geoDataTextLabel;
};

UCLASS( ClassGroup=(Custom),Blueprintable, meta=(BlueprintSpawnableComponent) )
class DDAUGMENTED_API UAugmentedDebugger : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAugmentedDebugger();
    
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_PlaneRenderer)
    AARPlaneRenderer *PlaneRenderer;
    
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(Server, Reliable)
    void ServerSpawnPlaneRenderer();
		
    UFUNCTION(BlueprintNativeEvent)
    void OnNotify_PlaneRendererSpawned();
    virtual void OnNotify_PlaneRendererSpawned_Implementation();
    
    
    UPROPERTY(BlueprintReadWrite)
    APlayerController *ArController;
    
    UPROPERTY(BlueprintReadWrite)
    APawn *ArPawn;
    
    UPROPERTY(BlueprintReadWrite, Replicated)
    bool isRenderingPawn;
    
    UPROPERTY(BlueprintReadWrite)
    bool isRenderingCamera;
    
    UPROPERTY(BlueprintReadWrite , Replicated)
    bool isRenderingTrackOrigin;
    
    UPROPERTY(BlueprintReadWrite, Replicated)
    bool isRenderingImages;
    
    UPROPERTY(BlueprintReadWrite)
    FTrackingInfo TrackingInfo;
    
    UPROPERTY(BlueprintReadWrite, Replicated)
    TArray<FTrackedImageData> TrackedImages;
    
    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerUpdateTrackingInfo(FTrackingInfo tInfo);
    
    UFUNCTION(BlueprintCallable)
    bool Equals(const FTrackingInfo& tInfo1, const FTrackingInfo& tInfo2);
    
    UFUNCTION(Client, Reliable, BlueprintCallable)
    void ClientSetPawnAdjustment(FTransform adjustmentTransform);
    
    UFUNCTION(Client, Reliable, BlueprintCallable)
    void ClientSetAlignmentAdjustment(FTransform adjustmentTransform);
    
    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerAddTrackedImage(FTrackedImageData tImage);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerRemoveTrackedImage(const TArray<FString>& imageIds);
    
    UFUNCTION(Server, Unreliable, BlueprintCallable)
    void ServerUpdateTrackedImage(FTrackedImageData tImage);
    
    UFUNCTION(BlueprintCallable)
    FTrackedImageData MakeNewTrackedImageData() const;
    
    UFUNCTION(BlueprintCallable)
    void SnapFiducials(const FString& fileName, bool onlyTracking = false) const;
    
    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSnapFiducials(const FString& fileName, bool onlyTracking = false) const;
    
    UFUNCTION(BlueprintCallable)
    static bool SaveFiducialImages(const FString& savePath,
                                   const TArray<FTrackedImageData>& imageData);
    
    UFUNCTION(BlueprintCallable)
    static bool LoadFiducialImages(const FString& loadPath,
                                   TArray<FTrackedImageData>& imageData);
    
    UFUNCTION(BlueprintCallable)
    FTransform GetPawnAdjustment() const { return pawnAdjustment_; }
    
    UFUNCTION(BlueprintCallable)
    FTransform GetAlignemntAdjustment() const { return alignmentAdjustment_; }
    
protected:
    // Called when the game starts
    virtual void BeginPlay() override;
    
    FTransform pawnAdjustment_, alignmentAdjustment_;
    
private:
    
    UFUNCTION()
    void OnRep_PlaneRenderer();
    
    static void SaveLoadTrackedImage(FArchive& Ar, FTrackedImageData& imageData);
    
};
