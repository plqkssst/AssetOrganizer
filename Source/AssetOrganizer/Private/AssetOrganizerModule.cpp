// Copyright UEMaster. All Rights Reserved.

#include "AssetOrganizerModule.h"
#include "SAssetOrganizerWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "AssetOrganizer"

const FName FAssetOrganizerModule::TabName = FName("AssetOrganizer");
TSharedPtr<FSlateStyleSet> FAssetOrganizerModule::StyleSet = nullptr;

FString FAssetOrganizerModule::GetVersionString()
{
    return TEXT("2.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// Style registration
// ─────────────────────────────────────────────────────────────────────────────
void FAssetOrganizerModule::RegisterStyle()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShareable(new FSlateStyleSet("AssetOrganizerStyle"));

    // Root content dir: <PluginDir>/Resources/
    FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("AssetOrganizer"))->GetBaseDir()
        / TEXT("Resources");

    StyleSet->SetContentRoot(ResourcesDir);
    StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

    // Register Icon.png as the toolbar / tab icon (40x40 for toolbar, 16x16 for menus)
    StyleSet->Set("AssetOrganizer.TabIcon",
        new FSlateImageBrush(ResourcesDir / TEXT("Icon.png"), FVector2D(40.f, 40.f)));

    StyleSet->Set("AssetOrganizer.TabIcon.Small",
        new FSlateImageBrush(ResourcesDir / TEXT("Icon.png"), FVector2D(20.f, 20.f)));

    // Additional icons
    StyleSet->Set("AssetOrganizer.History",
        new FSlateImageBrush(ResourcesDir / TEXT("Icon.png"), FVector2D(16.f, 16.f)));

    StyleSet->Set("AssetOrganizer.Settings",
        new FSlateImageBrush(ResourcesDir / TEXT("Icon.png"), FVector2D(16.f, 16.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FAssetOrganizerModule::UnregisterStyle()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
        StyleSet.Reset();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Module lifecycle
// ─────────────────────────────────────────────────────────────────────────────
void FAssetOrganizerModule::StartupModule()
{
    RegisterStyle();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabName,
        FOnSpawnTab::CreateRaw(this, &FAssetOrganizerModule::SpawnTab)
    )
    .SetDisplayName(FText::FromString(TEXT("资产整理 v2.0")))
    .SetTooltipText(FText::FromString(TEXT("打开资产整理面板 v2.0")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
    .SetMenuType(ETabSpawnerMenuType::Enabled)
    .SetIcon(FSlateIcon("AssetOrganizerStyle", "AssetOrganizer.TabIcon", "AssetOrganizer.TabIcon.Small"));

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetOrganizerModule::RegisterMenus)
    );
}

void FAssetOrganizerModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
    UnregisterStyle();
}

TSharedRef<SDockTab> FAssetOrganizerModule::SpawnTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SBox)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SAssetOrganizerWidget)
            ]
        ];
}

void FAssetOrganizerModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenus* ToolMenus = UToolMenus::Get();
    if (!ToolMenus)
    {
        return;
    }

    // ── Window menu entry ─────────────────────────────────────────────────────
    {
        UToolMenu* WindowMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Window");
        FToolMenuSection& Section = WindowMenu->FindOrAddSection("AssetOrganizerSection");
        Section.AddMenuEntry(
            "AssetOrganizerOpen",
            FText::FromString(TEXT("资产整理 v2.0")),
            FText::FromString(TEXT("打开资产整理面板 v2.0 (62+ 类型支持)")),
            FSlateIcon("AssetOrganizerStyle", "AssetOrganizer.TabIcon", "AssetOrganizer.TabIcon.Small"),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(FName("AssetOrganizer"));
            }))
        );
    }

    // ── Level Editor toolbar button ───────────────────────────────────────────
    {
        UToolMenu* ToolBar = ToolMenus->ExtendMenu("LevelEditor.LevelEditorToolBar");
        FToolMenuSection& Section = ToolBar->FindOrAddSection("AssetOrganizerToolBar");
        FToolMenuEntry& Entry = Section.AddEntry(
            FToolMenuEntry::InitToolBarButton(
                "AssetOrganizerBtn",
                FUIAction(FExecuteAction::CreateLambda([]()
                {
                    FGlobalTabmanager::Get()->TryInvokeTab(FName("AssetOrganizer"));
                })),
                FText::FromString(TEXT("资产整理")),
                FText::FromString(TEXT("打开资产整理面板 v2.0")),
                FSlateIcon("AssetOrganizerStyle", "AssetOrganizer.TabIcon", "AssetOrganizer.TabIcon.Small")
            )
        );
        (void)Entry;
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetOrganizerModule, AssetOrganizer)
