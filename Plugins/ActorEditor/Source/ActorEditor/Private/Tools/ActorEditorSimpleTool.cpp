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

bool IsComponentTagExist(UInstancedStaticMeshComponent* Component, const FString* Text, FVector& TargetVector) {

	if (Component && Text)
	{
		const TArray<FName>& Tags = Component->ComponentTags;
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
TArray<TPair<ActorType*, FVector>> UActorEditorSimpleTool::GetActorColorPairArrayByTag(FString* TargetTag) {

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

FDynamicMesh3* CreateDynamicMeshFromStaticMesh(const UStaticMesh* StaticMesh)
{
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid static mesh pointer."));
		return nullptr;
	}

	FDynamicMesh3* NewDynamicMesh = new FDynamicMesh3();

	// StaticMesh�κ��� MeshDescription ��������
	FMeshDescription MeshDescription = *(StaticMesh->GetMeshDescription(0));
	FStaticMeshAttributes Attributes(MeshDescription);

	// ���� ��ġ �Ӽ� ��������
	TVertexAttributesConstRef<FVector> VertexPositions = Attributes.GetVertexPositions();

	// ��� ������ DynamicMesh�� �߰�
	for (const FVertexID VertexID : MeshDescription.Vertices().GetElementIDs())
	{
		FVector Position = VertexPositions[VertexID];
		NewDynamicMesh->AppendVertex(FVector3d(Position));
	}

	Attributes.GetTriangleVertexInstanceIndices();

	// �ﰢ�� ������ �߰�
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
	// ���ο� UStaticMesh �ּ� ����
	UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(InOuter);
	if (!NewStaticMesh)
	{
		return nullptr;
	}

	// MeshDescription�� �����ϰ� �ʱ�ȭ
	FMeshDescription MeshDescription;
	NewStaticMesh->CreateMeshDescription(0, MeshDescription);
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	// �Ӽ� ��������
	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> PolygonGroupMaterialNames = Attributes.GetPolygonGroupMaterialSlotNames();

	// ���� �߰�
	for (int32 VertexID = 0; VertexID < DynamicMesh->MaxVertexID(); VertexID++)
	{
		if (DynamicMesh->IsVertex(VertexID))
		{
			FVector3f Position(DynamicMesh->GetVertex(VertexID).X, DynamicMesh->GetVertex(VertexID).Y, DynamicMesh->GetVertex(VertexID).Z);
			const FVertexID AddedVertexID = MeshDescription.CreateVertex();
			VertexPositions[AddedVertexID] = Position;
		}
	}

	// �ﰢ�� �� ������ �׷� �߰�
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

			// ������� �ﰢ�� �߰�
			TArray<FVertexInstanceID> Perimeter;
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID0));
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID1));
			Perimeter.Add(MeshDescription.CreateVertexInstance(VertexID2));

			// ������ ����
			const FPolygonID NewPolygonID = MeshDescription.CreatePolygon(PolygonGroupID, Perimeter);
		}
	}

	// �޽� ����
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

	// ���ο� StaticMesh�� ������ ��Ű�� ����
	UPackage* Package = CreatePackage(TEXT("/Game/ConvertedMeshes/MyNewStaticMesh"));
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package"));
		return;
	}
	// ��Ű���� StaticMesh ����
	UStaticMesh* NewStaticMesh = CreateStaticMeshFromDynamicMesh(DynamicMesh, Package);

	if (NewStaticMesh)
	{
		// StaticMesh�� ��Ű���� �߰�
		NewStaticMesh->Rename(*FString("MyNewStaticMesh"));
		NewStaticMesh->MarkPackageDirty(); // ���� ���� ���� �÷��� ����

		// �����Ϳ��� ���� ���� �ݿ�
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
			return FoliageActor; // ��ȯ�� ù ��° �ν��Ͻ�ȭ�� ������ ���͸� ã��
		}
	}

	return nullptr; // ������ ���͸� ã�� ����
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

	for (const TPair<AStaticMeshActor*, FVector> p : StaticMeshActorsToChangeColorPair)
	{
		UStaticMesh* OriginalStaticMesh = p.Key->GetStaticMeshComponent()->GetStaticMesh();
		SimplifyStaticMeshByDynamicMesh(OriginalStaticMesh);
	}


	// ClonedActorArray�� Simplify�ؼ�, �ٽ� ����?

	// Foliage Actors
	AInstancedFoliageActor* FoliageActor = FindInstancedFoliageActor(TargetWorld);

	// Change
	TArray<TPair<UInstancedStaticMeshComponent*, FVector>> ComponentsToChangeColorPair;


	if (FoliageActor != nullptr)
	{
		// FoliageActor�� ������Ʈ ����Ʈ�� ��ȸ
		TInlineComponentArray<UInstancedStaticMeshComponent*> Components;
		
		FoliageActor->GetComponents<UInstancedStaticMeshComponent>(Components);
		
		// �� FoliageInstancedStaticMeshComponent�� ���� ó��
		for (UInstancedStaticMeshComponent* OriginalComponent : Components)
		{
			FString* Tag = new FString(TEXT("Simplify"));
		
			delete Tag;

			// �� ������Ʈ ���� �� ����
			UInstancedStaticMeshComponent* DuplicatedComponent = NewObject<UInstancedStaticMeshComponent>(FoliageActor, UInstancedStaticMeshComponent::StaticClass());
			if (DuplicatedComponent != nullptr)
			{
				// �θ� ���Ϳ� ������Ʈ �߰�
				FoliageActor->AddInstanceComponent(DuplicatedComponent);
				DuplicatedComponent->RegisterComponent();

				// ����ƽ �޽� ����
				DuplicatedComponent->SetStaticMesh(OriginalComponent->GetStaticMesh());

				// �ν��Ͻ� ������ ����
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