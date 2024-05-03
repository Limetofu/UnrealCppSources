// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InteractiveToolBuilder.h"
#include "BaseTools/SingleClickTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshSimplification.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/UObjectGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "InstancedFoliageActor.h"
#include "ActorEditorSimpleTool.generated.h"

/**
 * Builder for UActorEditorSimpleTool
 */
UCLASS()
class ACTOREDITOR_API UActorEditorSimpleToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};



/**
 * Settings UObject for UActorEditorSimpleTool. This UClass inherits from UInteractiveToolPropertySet,
 * which provides an OnModified delegate that the Tool will listen to for changes in property values.
 */
UCLASS(Transient)
class ACTOREDITOR_API UActorEditorSimpleToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UActorEditorSimpleToolProperties();

	/** If enabled, dialog should display extended information about the actor clicked on. Otherwise, only basic info will be shown. */
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayName = "Show Extended Info"))
	bool ShowExtendedInfo;
};




/**
 * UActorEditorSimpleTool is an example Tool that opens a message box displaying info about an actor that the user
 * clicks left mouse button. All the action is in the ::OnClicked handler.
 */
UCLASS()
class ACTOREDITOR_API UActorEditorSimpleTool : public USingleClickTool
{
	GENERATED_BODY()

public:
	UActorEditorSimpleTool();

	virtual void SetWorld(UWorld* World);

	virtual void Setup() override;

	virtual void OnClicked(const FInputDeviceRay& ClickPos);

	template <typename ActorType>
	TArray<TPair<ActorType*, FVector>> GetActorColorPairArrayByTag(FString* TargetTag);


	template <typename ActorType>
	void CopySolidColorActorWithNewTag(TArray<TPair<ActorType*, FVector>>& TargetArray, TArray<ActorType*>& ClonedActorArray);





protected:
	UPROPERTY()
	TObjectPtr<UActorEditorSimpleToolProperties> Properties;


protected:
	/** target World we will raycast into to find actors */
	UWorld* TargetWorld;
};