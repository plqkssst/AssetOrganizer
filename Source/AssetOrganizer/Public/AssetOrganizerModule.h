// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Styling/SlateStyle.h"

class FAssetOrganizerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    TSharedRef<class SDockTab> SpawnTab(const FSpawnTabArgs& Args);

    /** Returns the custom style set (for icon access in Widget) */
    static TSharedPtr<FSlateStyleSet> GetStyleSet() { return StyleSet; }

    /** Get the plugin version string */
    static FString GetVersionString();

private:
    static const FName TabName;

    void RegisterMenus();

    /** Create and register the plugin's Slate style set */
    void RegisterStyle();
    void UnregisterStyle();

    static TSharedPtr<FSlateStyleSet> StyleSet;
};
