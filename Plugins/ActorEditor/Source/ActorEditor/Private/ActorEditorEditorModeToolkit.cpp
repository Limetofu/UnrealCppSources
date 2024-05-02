// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorEditorEditorModeToolkit.h"
#include "ActorEditorEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "ActorEditorEditorModeToolkit"

FActorEditorEditorModeToolkit::FActorEditorEditorModeToolkit()
{
}

void FActorEditorEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FActorEditorEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FActorEditorEditorModeToolkit::GetToolkitFName() const
{
	return FName("ActorEditorEditorMode");
}

FText FActorEditorEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "ActorEditorEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
