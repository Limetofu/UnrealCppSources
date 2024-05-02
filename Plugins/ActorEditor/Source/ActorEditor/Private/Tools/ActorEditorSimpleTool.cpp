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
	InString.ParseIntoArray(Parsed, TEXT("/"), true);  // '/'�� �и�

	if (Parsed.Num() == 3)  // ��Ȯ�� 3���� ��Ұ� �ʿ�
	{
		// FVector�� �� ��Ҹ� �Ľ̵� FString���� ���ڷ� ��ȯ
		float X = FCString::Atof(*Parsed[0]);
		float Y = FCString::Atof(*Parsed[1]);
		float Z = FCString::Atof(*Parsed[2]);

		OutVector = FVector(X, Y, Z);
		return true;
	}

	return false;
}

/** �־��� ���Ϳ��� Ư�� �±׸� �ٸ� �±׷� �����ϴ� �Լ� */
template <typename ActorType>
void ChangeTag(ActorType* Actor, const FName& OldTag, const FName& NewTag)
{
	if (Actor != nullptr)
	{
		// �±װ� �̹� �ִ��� Ȯ���ϰ�, �ִٸ� ����
		if (Actor->Tags.Contains(OldTag))
		{
			Actor->Tags.RemoveSingle(OldTag);
		}
		Actor->Tags.AddUnique(NewTag);
	}
}

/** Actor�� Tag �˻�, Color Vector ����*/
template <typename ActorType>
bool IsActorTagExist(ActorType* Actor, const FString* Text, FVector& TargetVector) {
	if (Actor && Text)
	{
		const TArray<FName>& Tags = Actor->Tags;
		for (int32 i = 0; i < Tags.Num() - 1; ++i) // i + 1 ��° ���� �����ϱ� ����
		{
			if (*Text == Tags[i].ToString()) // ���ϴ� Tag Text (Simpilfy) �� ������?
			{
				// ���� �±׸� ó��, Color �ٷ� �����ϱ�
				const FString NextTagString = Tags[i + 1].ToString();
				if (FStringToVector(NextTagString, TargetVector))
				{
					return true;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to convert next tag to FVector."));
					return false; // ��ȯ ����
				}
			}
		}
	}
	return false;
}

/**Actor�� ���ؼ� (Static Mesh�� �ش� �Լ� ����ϸ� �ȵ�!!) */\
template <typename ActorType>
void SetStaticMeshVisibility(ActorType* Actor, bool bVisible)
{
	if (!Actor) return;

	// Actor�� ��� ������Ʈ�� ��ȸ
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

	// World ��ȸ, ActorType�� Actor ã��, Array�� �߰�
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
* Material �ܻ��� Actor�� �����ϰ�, ������ Tag, �� Actor�� Name ���� (_Cloned_Suffix)
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

		// ClonedActor�� �� �� �ִ°͵�??
		// ���߿� �����ؾ� ��. Ref�� ���ڿ��� Add.
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
	// Pair�� �����.
	// Actor��, ColorValue�� ����ִ� Vector��.
	TArray<TPair<AActor*, FVector>> ActorsToChangeColorPair;
	TArray<TPair<AStaticMeshActor*, FVector>> StaticMeshActorsToChangeColorPair;

	// Tag���� ActorType���� ���� Array Getter
	FString* TagToCheck = new FString(TEXT("Simplify"));
	ActorsToChangeColorPair = GetActorColorPairArrayByTag<AActor>(TagToCheck);
	StaticMeshActorsToChangeColorPair = GetActorColorPairArrayByTag<AStaticMeshActor>(TagToCheck);
	delete TagToCheck;


	// ����� Actor���� ��� Array
	TArray<AActor*> ClonedActorArray;
	TArray<AStaticMeshActor*> ClonedStaticMeshActorArray;
	CopySolidColorActorWithNewTag<AActor>(ActorsToChangeColorPair, ClonedActorArray);
	CopySolidColorActorWithNewTag<AStaticMeshActor>(StaticMeshActorsToChangeColorPair, ClonedStaticMeshActorArray);

	// ClonedActorArray�� Simplify�ؼ�, �ٽ� ����?

	//// Foliage Actors
	//AInstancedFoliageActor* FoliageActor = FindInstancedFoliageActor(TargetWorld);

	

	//if (FoliageActor != nullptr)
	//{
	//	// FoliageActor�� ������Ʈ ����Ʈ�� ��ȸ
	//	TInlineComponentArray<UInstancedStaticMeshComponent*> Components;
	//	
	//	FoliageActor->GetComponents<UInstancedStaticMeshComponent>(Components);
	//	
	//	// �� FoliageInstancedStaticMeshComponent�� ���� ó��
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