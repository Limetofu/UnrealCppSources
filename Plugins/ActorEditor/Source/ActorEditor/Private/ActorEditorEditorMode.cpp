// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorEditorEditorMode.h"
#include "ActorEditorEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "ActorEditorEditorModeCommands.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/ActorEditorSimpleTool.h"
#include "Tools/ActorEditorInteractiveTool.h"

// step 2: register a ToolBuilder in FActorEditorEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "ActorEditorEditorMode"

const FEditorModeID UActorEditorEditorMode::EM_ActorEditorEditorModeId = TEXT("EM_ActorEditorEditorMode");

FString UActorEditorEditorMode::SimpleToolName = TEXT("ActorEditor_ActorInfoTool");
FString UActorEditorEditorMode::InteractiveToolName = TEXT("ActorEditor_MeasureDistanceTool");


UActorEditorEditorMode::UActorEditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UActorEditorEditorMode::EM_ActorEditorEditorModeId,
		LOCTEXT("ModeName", "ActorEditor"),
		FSlateIcon(),
		true);
}


UActorEditorEditorMode::~UActorEditorEditorMode()
{
}


void UActorEditorEditorMode::ActorSelectionChangeNotify()
{
}

void UActorEditorEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FActorEditorEditorModeCommands& SampleToolCommands = FActorEditorEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UActorEditorSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UActorEditorInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UActorEditorEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FActorEditorEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UActorEditorEditorMode::GetModeCommands() const
{
	return FActorEditorEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
