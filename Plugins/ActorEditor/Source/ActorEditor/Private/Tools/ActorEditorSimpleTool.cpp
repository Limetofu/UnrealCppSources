// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorEditorSimpleTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"


// localization namespace
#define LOCTEXT_NAMESPACE "ActorEditorSimpleTool"

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
TArray<TPair<ActorType*, FVector>> GetActorColorPairArrayByTag(FString* TargetTag) {

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
void CopySolidColorActorWithNewTag(TArray<TPair<ActorType*, FVector>>& TargetArray, TArray<ActorType*>& ClonedActorArray)
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

		if (ClonedActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cloned!!!"));
		}
		else
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

	// ClonedActorArray를 Simplify해서, 다시 설정?

	//// Foliage Actors
	//AInstancedFoliageActor* FoliageActor = FindInstancedFoliageActor(TargetWorld);

	

	//if (FoliageActor != nullptr)
	//{
	//	// FoliageActor의 컴포넌트 리스트를 순회
	//	TInlineComponentArray<UInstancedStaticMeshComponent*> Components;
	//	
	//	FoliageActor->GetComponents<UInstancedStaticMeshComponent>(Components);
	//	
	//	// 각 FoliageInstancedStaticMeshComponent에 대한 처리
	//	for (UInstancedStaticMeshComponent* Component : Components)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Found Component with %d Instances"), Component->GetInstanceCount());
	//		Component->GetStaticMesh();
	//		// Component->SetStaticMesh();
	//	}
	//}
}



// FText Title = LOCTEXT("ActorInfoDialogTitle", "Actor Info");
// FMessageDialog::Open(EAppMsgType::Ok, ActorInfoMsg, Title);
#undef LOCTEXT_NAMESPACE