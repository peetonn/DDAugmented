// Copyright 2018 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ARPlaneRenderer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ARBlueprintLibrary.h"
#include <Net/UnrealNetwork.h>
#include "DDLog.h"

// Sets default values
AARPlaneRenderer::AARPlaneRenderer()
{
//    DLOG_MODULE_TRACE(DDAugmented, "Ctor AARPlaneRenderer::AARPlaneRenderer()");
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	EdgeFeatheringDistance = 10.0f;
	NewPlaneIndex = 0.0f;
    bReplicates = true;
}

void AARPlaneRenderer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME_CONDITION(AARPlaneRenderer, GeoDataArray, COND_SkipOwner);
}

// Called when the game starts or when spawned
void AARPlaneRenderer::BeginPlay()
{
    DLOG_MODULE_TRACE(DDAugmented, "AARPlaneRenderer");
    
	Super::BeginPlay();
}

// Called every frame
void AARPlaneRenderer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
    // process current AR planes on mobile only
#if PLATFORM_ANDROID || PLATFORM_IOS
    if (GetLocalRole() >= ROLE_AutonomousProxy)
    {
        if (UARBlueprintLibrary::GetTrackingQuality() == EARTrackingQuality::OrientationAndPosition)
        {
            TArray<UARTrackedGeometry*> AllGeometries = UARBlueprintLibrary::GetAllGeometries();
            for (UARTrackedGeometry* Geometry : AllGeometries)
            {
                if (Geometry->IsA(UARPlaneGeometry::StaticClass()))
                {
                    UARPlaneGeometry* PlaneGeometry = Cast<UARPlaneGeometry>(Geometry);
                    UpdatePlaneData(PlaneGeometry);
                }
            }
        }
    }
#endif
    
    TArray<UARTrackedGeoData*> oldGeoData;
    GeoMeshMap.GetKeys(oldGeoData);
    
    // process plane data and create meshes if needed
    if (GeoDataArray.Num())
    {
//        DLOG_MODULE_DEBUG(DDAugmented, "GeoDataArray length {}", GeoDataArray.Num());
        for (auto& data : GeoDataArray)
        {
            oldGeoData.Remove(data);
            UpdateGeo(data);
        }
    }
    
    // remove old planes
    if (oldGeoData.Num())
    {
        DLOG_MODULE_DEBUG(DDAugmented, "Remove {} old planes", oldGeoData.Num());
        
        for (auto& data : oldGeoData)
        {
            UProceduralMeshComponent* PlanePolygonMeshComponent = *GeoMeshMap.Find(data);
        
            if (PlanePolygonMeshComponent)
            {
                PlanePolygonMeshComponent->DestroyComponent();
                GeoMeshMap.Remove(data);
            }
        }
    }
}

void AARPlaneRenderer::UpdatePlaneData(UARPlaneGeometry* ARCorePlaneObject)
{
    UARTrackedGeoData *TrackedGeoData = nullptr;
    
//    UProceduralMeshComponent* PlanePolygonMeshComponent = nullptr;
    
    if (!PlanesDataMap.Contains(ARCorePlaneObject))
    {
        if (ARCorePlaneObject->GetSubsumedBy() != nullptr || ARCorePlaneObject->GetTrackingState() == EARTrackingState::StoppedTracking)
        {
            return;
        }

        TrackedGeoData = NewObject<UARTrackedGeoData>();
        
//        PlanePolygonMeshComponent = NewObject<UProceduralMeshComponent>(this);
//        PlanePolygonMeshComponent->RegisterComponent();
//        PlanePolygonMeshComponent->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
//
//        UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(PlaneMaterial, this);
//        FColor Color = FColor::White;
//        if (PlaneColors.Num() != 0)
//        {
//            int ColorIndex = NewPlaneIndex % PlaneColors.Num();
//            Color = PlaneColors[ColorIndex];
//        }
//        DynMaterial->SetScalarParameterValue(FName(TEXT("TextureRotationAngle")), FMath::FRandRange(0.0f, 1.0f));
//        DynMaterial->SetVectorParameterValue(FName(TEXT("PlaneTint")), FLinearColor(Color));
//
//        PlanePolygonMeshComponent->SetMaterial(0, DynMaterial);
//        PlaneMeshMap.Add(ARCorePlaneObject, PlanePolygonMeshComponent);
        
        FColor Color = FColor::White;
        if (PlaneColors.Num() != 0)
        {
            int ColorIndex = NewPlaneIndex % PlaneColors.Num();
            Color = PlaneColors[ColorIndex];
        }
        
        TrackedGeoData->color_ = Color;
        TrackedGeoData->debugName_ = ARCorePlaneObject->GetDebugName();
        
        NewPlaneIndex++;
        
        AddNewGeoData(ARCorePlaneObject, TrackedGeoData);
    }
    else
    {
        TrackedGeoData = *PlanesDataMap.Find(ARCorePlaneObject);
//        PlanePolygonMeshComponent = *PlaneMeshMap.Find(ARCorePlaneObject);
    }

    // update geo data here
    if(ARCorePlaneObject->GetTrackingState() == EARTrackingState::Tracking &&
       ARCorePlaneObject->GetSubsumedBy() == nullptr)
    {
//        if (!PlanePolygonMeshComponent->bVisible)
//        {
//            PlanePolygonMeshComponent->SetVisibility(true, true);
//        }
//        UpdatePlaneMesh(ARCorePlaneObject, PlanePolygonMeshComponent);
        UpdateGeoData(ARCorePlaneObject, TrackedGeoData);
    }
//    else if (PlanePolygonMeshComponent->bVisible)
//    {
//        PlanePolygonMeshComponent->SetVisibility(false, true);
//    }
    
    if(ARCorePlaneObject->GetSubsumedBy() != nullptr || ARCorePlaneObject->GetTrackingState() == EARTrackingState::StoppedTracking)
    {
        TrackedGeoData = *PlanesDataMap.Find(ARCorePlaneObject);
//        PlanePolygonMeshComponent = *PlaneMeshMap.Find(ARCorePlaneObject);
        if(TrackedGeoData != nullptr)
        {
            RemoveGeoData(ARCorePlaneObject, TrackedGeoData);
        }
    }
}

void AARPlaneRenderer::AddNewGeoData(UARPlaneGeometry* ARCorePlaneObject, UARTrackedGeoData *data)
{
    PlanesDataMap.Add(ARCorePlaneObject, data);
    GeoDataArray.Add(data);
    
    // call RPC here
    if (GetLocalRole() == ROLE_AutonomousProxy)
    {
        RPC_GeoDataAddOrRemove(true, data);
    }
}

void AARPlaneRenderer::RemoveGeoData(UARPlaneGeometry* ARCorePlaneObject, UARTrackedGeoData *data)
{
    // call RPC here
    if (GetLocalRole() == ROLE_AutonomousProxy)
    {
        RPC_GeoDataAddOrRemove(false, data);
    }
    
    GeoDataArray.Remove(data);
    PlanesDataMap.Remove(ARCorePlaneObject);
}

void AARPlaneRenderer::UpdateGeoData(UARPlaneGeometry* ARCorePlaneObject, UARTrackedGeoData *data)
{
    data->boundaryVerts_ = ARCorePlaneObject->GetBoundaryPolygonInLocalSpace();
    data->localToWorld_ = ARCorePlaneObject->GetLocalToWorldTransform();
    data->localToTracking_ = ARCorePlaneObject->GetLocalToTrackingTransform();
    
    // call RPC here
    if (GetLocalRole() == ROLE_AutonomousProxy)
    {
        RPC_GeoDataUpdate(data);
    }
}

void AARPlaneRenderer::RPC_GeoDataAddOrRemove_Implementation(bool isAdd, UARTrackedGeoData *data)
{
//    if (HasAuthority())
    {
        if (isAdd)
        {
            DLOG_MODULE_DEBUG(DDAugmented, "SERVER ADD GEO TRACKED DATA");
            GeoDataArray.Add(data);
        }
        else
        {
            UARTrackedGeoData *dataToRemove = nullptr;
            for (auto& d : GeoDataArray)
                if (data->id_ == d->id_)
                {
                    dataToRemove = d;
                    break;
                }
            
            if (dataToRemove)
            {
                DLOG_MODULE_DEBUG(DDAugmented, "SERVER REMOVE GEO TRACKED DATA");
                GeoDataArray.Remove(dataToRemove);
            }
            else
            {
                DLOG_MODULE_ERROR(DDAugmented, "SERVER REMOVE FAILED -- CAN'T FIND DATA WITH ID {}", TCHAR_TO_ANSI(*data->id_.ToString()));
            }
        }
    }
}

void AARPlaneRenderer::RPC_GeoDataUpdate_Implementation(UARTrackedGeoData *data)
{
//    if (HasAuthority())
    {
        UARTrackedGeoData *dataToUpdate = nullptr;
        
        for (auto& d : GeoDataArray)
            if (data->id_ == d->id_)
            {
                dataToUpdate = d;
                break;
            }
        
        if (dataToUpdate)
        {
            dataToUpdate->boundaryVerts_ = data->boundaryVerts_;
            dataToUpdate->localToWorld_ = data->localToWorld_;
            dataToUpdate->localToTracking_ = data->localToTracking_;
        }
        else
        {
            DLOG_MODULE_WARN(DDAugmented, "Failed to update data with id {} -- data not found on the server", TCHAR_TO_ANSI(*data->id_.ToString()));
        }
    }
}

void AARPlaneRenderer::UpdateGeo(UARTrackedGeoData* TrackedGeoData)
{
    UProceduralMeshComponent* PlanePolygonMeshComponent = nullptr;
    if (!GeoMeshMap.Contains(TrackedGeoData))
    {
        PlanePolygonMeshComponent = NewObject<UProceduralMeshComponent>(this);
        PlanePolygonMeshComponent->RegisterComponent();
        PlanePolygonMeshComponent->AttachToComponent(this->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

        UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(PlaneMaterial, this);
        FColor Color = TrackedGeoData->color_;
        DynMaterial->SetScalarParameterValue(FName(TEXT("TextureRotationAngle")), FMath::FRandRange(0.0f, 1.0f));
        DynMaterial->SetVectorParameterValue(FName(TEXT("PlaneTint")), FLinearColor(Color));

        PlanePolygonMeshComponent->SetMaterial(0, DynMaterial);
        GeoMeshMap.Add(TrackedGeoData, PlanePolygonMeshComponent);
    }
    else
    {
        PlanePolygonMeshComponent = *GeoMeshMap.Find(TrackedGeoData);
    }

//    if(ARCorePlaneObject->GetTrackingState() == EARTrackingState::Tracking &&
//       ARCorePlaneObject->GetSubsumedBy() == nullptr)
//    {
        if (!PlanePolygonMeshComponent->IsVisible())
        {
            PlanePolygonMeshComponent->SetVisibility(true, true);
        }
        UpdateGeoMesh(TrackedGeoData, PlanePolygonMeshComponent);
//    }
//    else if (PlanePolygonMeshComponent->bVisible)
//    {
//        PlanePolygonMeshComponent->SetVisibility(false, true);
//    }
    
//    if(ARCorePlaneObject->GetSubsumedBy() != nullptr || ARCorePlaneObject->GetTrackingState() == EARTrackingState::StoppedTracking)
//    {
//        PlanePolygonMeshComponent = *PlaneMeshMap.Find(ARCorePlaneObject);
//        if(PlanePolygonMeshComponent != nullptr)
//        {
//            PlanePolygonMeshComponent->DestroyComponent(true);
//            PlaneMeshMap.Remove(ARCorePlaneObject);
//        }
//    }
}

void AARPlaneRenderer::UpdateGeoMesh(UARTrackedGeoData* TrackedGeoData, UProceduralMeshComponent* PlanePolygonMeshComponent)
{
    // Update polygon mesh vertex indices, using triangle fan due to its convex.
    TArray<FVector> BoundaryVertices;
    BoundaryVertices = TrackedGeoData->boundaryVerts_;
    int BoundaryVerticesNum = BoundaryVertices.Num();

    if (BoundaryVerticesNum < 3)
    {
        PlanePolygonMeshComponent->ClearMeshSection(0);
        return;
    }

    int PolygonMeshVerticesNum = BoundaryVerticesNum * 2;
    // Triangle number is interior(n-2 for convex polygon) plus perimeter (EdgeNum * 2);
    int TriangleNum = BoundaryVerticesNum - 2 + BoundaryVerticesNum * 2;

    TArray<FVector> PolygonMeshVertices;
    TArray<FLinearColor> PolygonMeshVertexColors;
    TArray<int> PolygonMeshIndices;
    TArray<FVector> PolygonMeshNormals;
    TArray<FVector2D> PolygonMeshUVs;

    PolygonMeshVertices.Empty(PolygonMeshVerticesNum);
    PolygonMeshVertexColors.Empty(PolygonMeshVerticesNum);
    PolygonMeshIndices.Empty(TriangleNum * 3);
    PolygonMeshNormals.Empty(PolygonMeshVerticesNum);

    FVector PlaneNormal = TrackedGeoData->localToWorld_.GetRotation().GetUpVector();
    for (int i = 0; i < BoundaryVerticesNum; i++)
    {
        FVector BoundaryPoint = BoundaryVertices[i];
        float BoundaryToCenterDist = BoundaryPoint.Size();
        float FeatheringDist = FMath::Min(BoundaryToCenterDist, EdgeFeatheringDistance);
        FVector InteriorPoint = BoundaryPoint - BoundaryPoint.GetUnsafeNormal() * FeatheringDist;

        PolygonMeshVertices.Add(BoundaryPoint);
        PolygonMeshVertices.Add(InteriorPoint);

        PolygonMeshUVs.Add(FVector2D(BoundaryPoint.X, BoundaryPoint.Y));
        PolygonMeshUVs.Add(FVector2D(InteriorPoint.X, InteriorPoint.Y));

        PolygonMeshNormals.Add(PlaneNormal);
        PolygonMeshNormals.Add(PlaneNormal);

        PolygonMeshVertexColors.Add(FLinearColor(0.0f, 0.f, 0.f, 0.f));
        PolygonMeshVertexColors.Add(FLinearColor(0.0f, 0.f, 0.f, 1.f));
    }

    // Generate triangle indices

    // Perimeter triangles
    for (int i = 0; i < BoundaryVerticesNum - 1; i++)
    {
        PolygonMeshIndices.Add(i * 2);
        PolygonMeshIndices.Add(i * 2 + 2);
        PolygonMeshIndices.Add(i * 2 + 1);

        PolygonMeshIndices.Add(i * 2 + 1);
        PolygonMeshIndices.Add(i * 2 + 2);
        PolygonMeshIndices.Add(i * 2 + 3);
    }

    PolygonMeshIndices.Add((BoundaryVerticesNum - 1) * 2);
    PolygonMeshIndices.Add(0);
    PolygonMeshIndices.Add((BoundaryVerticesNum - 1) * 2 + 1);


    PolygonMeshIndices.Add((BoundaryVerticesNum - 1) * 2 + 1);
    PolygonMeshIndices.Add(0);
    PolygonMeshIndices.Add(1);

    // interior triangles
    for (int i = 3; i < PolygonMeshVerticesNum - 1; i += 2)
    {
        PolygonMeshIndices.Add(1);
        PolygonMeshIndices.Add(i);
        PolygonMeshIndices.Add(i + 2);
    }

    // No need to fill uv and tangent;
    PlanePolygonMeshComponent->CreateMeshSection_LinearColor(0, PolygonMeshVertices, PolygonMeshIndices, PolygonMeshNormals, PolygonMeshUVs, PolygonMeshVertexColors, TArray<FProcMeshTangent>(), false);

    // Set the component transform to Plane's transform.
    PlanePolygonMeshComponent->SetWorldTransform(TrackedGeoData->localToWorld_);
}
