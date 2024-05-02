// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorEditorModule.h"
#include "ActorEditorEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "ActorEditorModule"

void FActorEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FActorEditorEditorModeCommands::Register();
}

void FActorEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FActorEditorEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FActorEditorModule, ActorEditorEditorMode)