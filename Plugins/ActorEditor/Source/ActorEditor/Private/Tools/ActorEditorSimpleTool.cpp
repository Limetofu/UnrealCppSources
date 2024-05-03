// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorEditorSimpleTool.h"


// localization namespace
#define LOCTEXT_NAMESPACE "ActorEditorSimpleTool"

using namespace UE::Geometry;

/*
 * ToolBuilder implementation
 */

UInteractiveTool* UActorEditorSimpleToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UActorEditorSimpleTool* NewTool = NewObject<UActorEditorSimpleTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}



/*
 * ToolProperties implementation
 */

UActorEditorSimpleToolProperties::UActorEditorSimpleToolProperties()
{
	ShowExtendedInfo = true;
}


/*
 * Tool implementation
 */

UActorEditorSimpleTool::UActorEditorSimpleTool()
{
}


void UActorEditorSimpleTool::SetWorld(UWorld* World)
{
	this->TargetWorld = World;
}


void UActorEditorSimpleTool::Setup()
{
	USingleClickTool::Setup();

	Properties = NewObject<UActorEditorSimpleToolProperties>(this);
	AddToolPropertySource(Properties);
}




bool FStringToVector(const FString& InString, FVector& OutVector)
{
	TArray<FString> Parsed;
	InString.ParseIntoArray(Parsed, TEXT("/"), true);  // '/'로 분리

	if (Parsed.Num() == 3)  // 정확히 3개의 요소가 필요
	{
		// FVector의 각 요소를 파싱된 FString에서 숫자로 변환
		float X = FCString::Atof(*Parsed[0]);
		float Y = FCString::Atof(*Parsed[1]);
		float Z = FCString::Atof(*Parsed[2]);

		OutVector = FVector(X, Y, Z);
		return true;
	}

	return false;
}

/** 주어진 액터에서 특정 태그를 다른 태그로 변경하는 함수 */
template <typename ActorType>
void ChangeTag(ActorType* Actor, const FName& OldTag, const FName& NewTag)
{
	if (Actor != nullptr)
	{
		// 태그가 이미 있는지 확인하고, 있다면 제거
		if (Actor->Tags.Contains(OldTag))
		{
			Actor->Tags.RemoveSingle(OldTag);
		}
		Actor->Tags.AddUnique(NewTag);
	}
}

/** Actor의 Tag 검사, Color Vector 수정*/
template <typename ActorType>
bool IsActorTagExist(ActorType* Actor, const FString* Text, FVector& TargetVector) {
	if (Actor && Text)
	{
		const TArray<FName>& Tags = Actor->Tags;
		for (int32 i = 0; i < Tags.Num() - 1; ++i) // i + 1 번째 원소 접근하기 때문
		{
			if (*Text == Tags[i].ToString()) // 원하는 Tag Text (Simpilfy) 와 같은지?
			{
				// 다음 태그를 처리, Color 바로 대입하기
				const FString NextTagString = Tags[i + 1].ToString();
				if (FStringToVector(NextTagString, TargetVector))
				{
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to convert next tag to FVector."));
					return false; // 변환 실패
				}
			}
		}
	}
	return false;
}

bool IsComponentTagExist(UInstancedStaticMeshComponent* Component, const FString* Text, FVector& TargetVector) {

	if (Component && Text)
	{
		const TArray<FName>& Tags = Component->ComponentTags;
		for (int32 i = 0; i < Tags.Num() - 1; ++i) // i + 1 번째 원소 접근하기 때문
		{
			if (*Text == Tags[i].ToString()) // 원하는 Tag Text (Simpilfy) 와 같은지?
			{
				// 다음 태그를 처리, Color 바로 대입하기
				const FString NextTagString = Tags[i + 1].ToString();
				if (FStringToVector(NextTagString, TargetVector))
				{
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to convert next tag to FVector."));
					return false; // 변환 실패
				}
			}
		}
	}
	return false;
}



/**Actor에 대해서 (Static Mesh는 해당 함수 사용하면 안됨!!) */\
template <typename ActorType>
void SetStaticMeshVisibility(ActorType* Actor, bool bVisible)
{
	if (!Actor) return;

	// Actor의 모든 컴포넌트를 순회
	TArray<UStaticMeshComponent*> Components;
	Actor->GetComponents<UStaticMeshComponent>(Components);

	for (UStaticMeshComponent* Component : Components)
	{
		if (Component)
		{
			//UE_LOG(LogTemp, Log, TEXT("Component Name : %s"), Component->GetFName().ToString());

			Component->SetVisibility(bVisible);
			//Component->SetHiddenInGame(!bVisible, true);
		}
	}
}


template <typename ActorType>
TArray<TPair<ActorType*, FVector>> UActorEditorSimpleTool::GetActorColorPairArrayByTag(FString* TargetTag) {

	TArray<TPair<ActorType*, FVector>> OutResult;

	// World 순회, ActorType인 Actor 찾기, Array에 추가
	for (TActorIterator<ActorType> It(TargetWorld); It; ++It)
	{
		ActorType* Actor = *It;
		if (Actor)
		{
			FVector ColorVector;
			if (IsActorTagExist(Actor, TargetTag, ColorVector))
			{
				OutResult.Add({ Actor, ColorVector });
			}
		}
	}

	return OutResult;
}


/*
* Material 단색인 Actor로 복사하고, 서로의 Tag, 새 Actor의 Name 수정 (_Cloned_Suffix)
*/
template <typename ActorType>
void UActorEditorSimpleTool::CopySolidColorActorWithNewTag(TArray<TPair<ActorType*, FVector>>& TargetArray, TArray<ActorType*>& ClonedActorArray)
{
	for (const TPair<ActorType*, FVector>& p : TargetArray)
	{
		UClass* OriginalActorClass = p.Key->GetClass();
		FTransform OriginalActorTransform = p.Key->GetActorTransform();
		ActorType* ClonedActor = TargetWorld->SpawnActor<ActorType>(OriginalActorClass, OriginalActorTransform);
		ChangeTag(p.Key, "Simplify", "ExistSimplified");

		FString NewNameForClonedActor = OriginalActorClass->GetName();
		FString UniqueSuffix = FString::Printf(TEXT("_Cloned_%lld"), FDateTime::Now().GetTicks());
		NewNameForClonedActor += UniqueSuffix;
		ClonedActor->SetActorLabel(NewNameForClonedActor);
		ChangeTag(ClonedActor, "Simplify", "Simplified");

		UE_LOG(LogTemp, Warning, TEXT("NEW ACTOR NAME : %s"), *NewNameForClonedActor);

		if (ClonedActor == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Didn't Cloned.."));
		}

		// p.Key->SetActorHiddenInGame(true);
		SetStaticMeshVisibility(p.Key, false);

		FLinearColor Color(static_cast<float>(p.Value.X), static_cast<float>(p.Value.Y), static_cast<float>(p.Value.Z), 1.f);
		FVector ColorVector(Color.R, Color.G, Color.B);

		ClonedActor->FindComponentByClass<UMeshComponent>()->SetVectorParameterValueOnMaterials(FName("SolidColor"), ColorVector);
		ClonedActor->FindComponentByClass<UMeshComponent>()->SetScalarParameterValueOnMaterials(FName("IsLowPoly?"), 1.f);

		// ClonedActor로 할 수 있는것들??
		// 나중에 수정해야 함. Ref만 인자에서 Add.
		ClonedActorArray.Add(ClonedActor);
	}
}



FVector3d ConvertFVector3fToFVector3d(const FVector3f& Vec3f)
{
	return FVector3d((double)Vec3f.X, (double)Vec3f.Y, (double)Vec3f.Z);
}

FVector3f ConvertFVector3dToFVector3f(const FVector3d& Vec3f)
{
	return FVector3f((float)Vec3f.X, (float)Vec3f.Y, (float)Vec3f.Z);
}

FDynamicMesh3* CreateDynamicMeshFromStaticMesh(const UStaticMesh* StaticMesh)
{
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid static mesh pointer."));
		return nullptr;
	}

	FDynamicMesh3* NewDynamicMesh = new FDynamicMesh3();

	// StaticMesh로부터 MeshDescription 가져오기
	FMeshDescription MeshDescription = *(StaticMesh->GetMeshDescription(0));
	FStaticMeshAttributes Attributes(MeshDescription);

	// 정점 위치 속성 가져오기
	TVertexAttributesConstRef<FVector> VertexPositions = Attributes.GetVertexPositions();

	// 모든 정점을 DynamicMesh에 추가
	for (const FVertexID VertexID : MeshDescription.Vertices().GetElementIDs())
	{
		FVector Position = VertexPositions[VertexID];
		NewDynamicMesh->AppendVertex(FVector3d(Position));
	}

	Attributes.GetTriangleVertexInstanceIndices();

	// 삼각형 데이터 추가
	TTriangleAttributesRef<TArrayView<FVertexInstanceID>> TriangleVertexInstances = Attributes.GetTriangleVertexInstanceIndices();
	for (const FTriangleID TriangleID : MeshDescription.Triangles().GetElementIDs())
	{
		FIndex3i Tri;
		Tri.A = TriangleVertexInstances[TriangleID][0].GetValue();
		Tri.B = TriangleVertexInstances[TriangleID][1].GetValue();
		Tri.C = TriangleVertexInstances[TriangleID][2].GetValue();
		NewDynamicMesh->AppendTriangle(Tri.A, Tri.B, Tri.C);
	}

	return NewDynamicMesh;
}

UStaticMesh* CreateStaticMeshFromDynamicMesh(const FDynamicMesh3* DynamicMesh, UObject* InOuter)
{
	// 새로운 UStaticMesh 애셋 생성
	UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(InOuter);
	if (!NewStaticMesh)
	{
		return nullptr;
	}

	// MeshDescription을 생성하고 초기화
	FMeshDescription MeshDescription;
	NewStaticMesh->CreateMeshDescription(0, MeshDescription);
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	// 속성 가져오기
	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> PolygonGroupMaterialNames = Attributes.GetPolygonGroupMaterialSlotNames();

	// 정점 추가
	for (int32 VertexID = 0; VertexID < DynamicMesh->MaxVertexID(); VertexID++)
	{
		if (DynamicMesh->IsVertex(VertexID))
		{
			FVector3f Position(DynamicMesh->GetVertex(VertexID).X, DynamicMesh->GetVertex(VertexID).Y, DynamicMesh->GetVertex(VertexID).Z);
			const FVertexID AddedVertexID = MeshDescription.CreateVertex();
			VertexPositions[AddedVertexID] = Position;
		}
	}

	// 삼각형 및 폴리곤 그룹 추가
	FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
	PolygonGroupMaterialNames[PolygonGroupID] = FName(TEXT("DefaultMaterial"));

	for (int32 TriangleID = 0; TriangleID < DynamicMesh->MaxTriangleID(); TriangleID++)
	{
		if (DynamicMesh->IsTriangle(TriangleID))
		{
			FIndex3i Tri = DynamicMesh->GetTriangle(TriangleID);
			const FVertexID VertexID0 = MeshDescription.GetVertexInstanceVertex(FVertexInstanceID(Tri.A));
			const FVertexID VertexID1 = MeshDescription.GetVertexInstanceVertex(FVertexInstanceID(Tri.B));
			const FVertexID VertexID2 = MeshDescription.GetVertexInstanceVertex(FVertexInstanceID(Tri.C));

			// 폴리곤과 삼각형 추가
			TArray<FVertexInstanceID> Perimeter;
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID0));
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID1));
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID2));

			// 폴리곤 생성
			const FPolygonID NewPolygonID = MeshDescription.CreatePolygon(PolygonGroupID, Perimeter);
		}
	}

	// 메시 빌드
	NewStaticMesh->CommitMeshDescription(0);
	NewStaticMesh->MarkPackageDirty();
	NewStaticMesh->PostEditChange();

	return NewStaticMesh;
}

void QEMSimplifyDynamicMesh(FDynamicMesh3* DynamicMesh, int TriangleCount)
{
	if (DynamicMesh == nullptr || TriangleCount <= 2)
	{
		UE_LOG(LogTemp, Error, TEXT("DynamicMesh is null"));
	}
	
	FQEMSimplification Simplifer(DynamicMesh);
	Simplifer.SimplifyToTriangleCount(TriangleCount);
}

void SimplifyStaticMeshByDynamicMesh(UStaticMesh* StaticMesh)
{
	if (StaticMesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("StaticMesh is null"));
		return;
	}

	FDynamicMesh3* DynamicMesh = CreateDynamicMeshFromStaticMesh(StaticMesh);
	// Simplify
	QEMSimplifyDynamicMesh(DynamicMesh, 50);

	// 새로운 StaticMesh를 저장할 패키지 생성
	UPackage* Package = CreatePackage(TEXT("/Game/ConvertedMeshes/MyNewStaticMesh"));
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package"));
		return;
	}
	// 패키지에 StaticMesh 생성
	UStaticMesh* NewStaticMesh = CreateStaticMeshFromDynamicMesh(DynamicMesh, Package);

	if (NewStaticMesh)
	{
		// StaticMesh를 패키지에 추가
		NewStaticMesh->Rename(*FString("MyNewStaticMesh"));
		NewStaticMesh->MarkPackageDirty(); // 변경 사항 저장 플래그 설정

		// 에디터에서 변경 사항 반영
		FAssetRegistryModule::AssetCreated(NewStaticMesh);
		Package->SetDirtyFlag(true);

		UE_LOG(LogTemp, Log, TEXT("Successfully created a new static mesh: %s"), *NewStaticMesh->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create static mesh from dynamic mesh"));
	}
}

AInstancedFoliageActor* FindInstancedFoliageActor(UWorld* TargetWorld)
{
	if (!TargetWorld)
	{
		return nullptr;
	}

	for (TActorIterator<AInstancedFoliageActor> It(TargetWorld); It; ++It)
	{
		AInstancedFoliageActor* FoliageActor = *It;
		if (FoliageActor)
		{
			return FoliageActor; // 반환할 첫 번째 인스턴스화된 포리지 액터를 찾음
		}
	}

	return nullptr; // 포리지 액터를 찾지 못함
}

void UActorEditorSimpleTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	// Pair를 만들기.
	// Actor와, ColorValue가 들어있는 Vector로.
	TArray<TPair<AActor*, FVector>> ActorsToChangeColorPair;
	TArray<TPair<AStaticMeshActor*, FVector>> StaticMeshActorsToChangeColorPair;

	// Tag따라 ActorType별로 따로 Array Getter
	FString* TagToCheck = new FString(TEXT("Simplify"));
	ActorsToChangeColorPair = GetActorColorPairArrayByTag<AActor>(TagToCheck);
	StaticMeshActorsToChangeColorPair = GetActorColorPairArrayByTag<AStaticMeshActor>(TagToCheck);
	delete TagToCheck;


	// 복사된 Actor들이 담길 Array 
	TArray<AActor*> ClonedActorArray;
	TArray<AStaticMeshActor*> ClonedStaticMeshActorArray;
	CopySolidColorActorWithNewTag<AActor>(ActorsToChangeColorPair, ClonedActorArray);
	CopySolidColorActorWithNewTag<AStaticMeshActor>(StaticMeshActorsToChangeColorPair, ClonedStaticMeshActorArray);

	for (const TPair<AStaticMeshActor*, FVector> p : StaticMeshActorsToChangeColorPair)
	{
		UStaticMesh* OriginalStaticMesh = p.Key->GetStaticMeshComponent()->GetStaticMesh();
		SimplifyStaticMeshByDynamicMesh(OriginalStaticMesh);
	}


	// ClonedActorArray를 Simplify해서, 다시 설정?

	// Foliage Actors
	AInstancedFoliageActor* FoliageActor = FindInstancedFoliageActor(TargetWorld);

	// Change
	TArray<TPair<UInstancedStaticMeshComponent*, FVector>> ComponentsToChangeColorPair;


	if (FoliageActor != nullptr)
	{
		// FoliageActor의 컴포넌트 리스트를 순회
		TInlineComponentArray<UInstancedStaticMeshComponent*> Components;
		
		FoliageActor->GetComponents<UInstancedStaticMeshComponent>(Components);
		
		// 각 FoliageInstancedStaticMeshComponent에 대한 처리
		for (UInstancedStaticMeshComponent* OriginalComponent : Components)
		{
			FString* Tag = new FString(TEXT("Simplify"));
		
			delete Tag;

			// 새 컴포넌트 생성 및 설정
			UInstancedStaticMeshComponent* DuplicatedComponent = NewObject<UInstancedStaticMeshComponent>(FoliageActor, UInstancedStaticMeshComponent::StaticClass());
			if (DuplicatedComponent != nullptr)
			{
				// 부모 액터에 컴포넌트 추가
				FoliageActor->AddInstanceComponent(DuplicatedComponent);
				DuplicatedComponent->RegisterComponent();

				// 스태틱 메쉬 설정
				DuplicatedComponent->SetStaticMesh(OriginalComponent->GetStaticMesh());

				// 인스턴스 데이터 복사
				const int32 InstanceCount = OriginalComponent->GetInstanceCount();
				for (int32 i = 0; i < InstanceCount; ++i)
				{
					FTransform InstanceTransform;
					if (OriginalComponent->GetInstanceTransform(i, InstanceTransform, true))
					{
						DuplicatedComponent->AddInstance(InstanceTransform);
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Found Component with %d Instances"), OriginalComponent->GetInstanceCount());
			OriginalComponent->GetStaticMesh();
			// Component->SetStaticMesh();
		}
	}
}



// FText Title = LOCTEXT("ActorInfoDialogTitle", "Actor Info");
// FMessageDialog::Open(EAppMsgType::Ok, ActorInfoMsg, Title);
#undef LOCTEXT_NAMESPACE