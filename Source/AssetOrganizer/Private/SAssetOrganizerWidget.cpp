// Copyright UEMaster. All Rights Reserved.

#include "SAssetOrganizerWidget.h"
#include "SlateOptMacros.h"
#include "AssetOrganizerExecutor.h"
#include "AssetOrganizerSettings.h"
#include "AssetOrganizerModule.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyleRegistry.h"
#include "EditorStyleSet.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/MessageDialog.h"
#include "Editor.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "AssetOrganizerUI"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

// ─────────────────────────────────────────────────────────────────────────────
// Color Constants
// ─────────────────────────────────────────────────────────────────────────────
namespace AOColors
{
    static const FLinearColor BgBase(0.065f, 0.065f, 0.065f, 1.f);
    static const FLinearColor BgHeader(0.075f, 0.075f, 0.075f, 1.f);
    static const FLinearColor BgToolbar(0.14f, 0.14f, 0.14f, 1.f);
    static const FLinearColor BgToolbarBtn(0.18f, 0.18f, 0.18f, 1.f);
    static const FLinearColor BgRow(0.104f, 0.104f, 0.104f, 1.f);
    static const FLinearColor BgRowAlt(0.094f, 0.094f, 0.094f, 1.f);
    static const FLinearColor BgCategory(0.078f, 0.078f, 0.078f, 1.f);
    static const FLinearColor BgPanel(0.08f, 0.08f, 0.08f, 1.f);
    static const FLinearColor BgCapsule(0.082f, 0.294f, 0.522f, 1.f);
    static const FLinearColor BgCapsuleGreen(0.2f, 0.5f, 0.2f, 1.f);
    static const FLinearColor BgCapsuleRed(0.5f, 0.2f, 0.2f, 1.f);
    static const FLinearColor TextPrimary(0.83f, 0.83f, 0.83f, 1.f);
    static const FLinearColor TextSecondary(0.63f, 0.63f, 0.63f, 1.f);
    static const FLinearColor TextDim(0.44f, 0.44f, 0.44f, 1.f);
    static const FLinearColor TextCategory(0.78f, 0.75f, 0.63f, 1.f);
    static const FLinearColor TextPrefix(0.60f, 0.72f, 0.85f, 1.f);
    static const FLinearColor TextCapsule(0.78f, 0.88f, 0.97f, 1.f);
    static const FLinearColor GreenBadge(0.294f, 0.549f, 0.294f, 1.f);
    static const FLinearColor GreenBadgeText(0.43f, 0.78f, 0.43f, 1.f);
    static const FLinearColor AccentBlue(0.f, 0.471f, 0.843f, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
SAssetOrganizerWidget::~SAssetOrganizerWidget()
{
    UAssetOrganizerSettings::OnCustomRulesChanged.RemoveAll(this);

    // Unregister asset registry delegates
    FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
    if (AssetRegistryModule)
    {
        IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
        AssetRegistry.OnAssetAdded().Remove(OnAssetAddedHandle);
        AssetRegistry.OnAssetRemoved().Remove(OnAssetRemovedHandle);
        AssetRegistry.OnAssetRenamed().Remove(OnAssetRenamedHandle);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Construct
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::Construct(const FArguments& InArgs)
{
    LoadSettings();

    if (Settings)
    {
        UAssetOrganizerSettings::OnCustomRulesChanged.AddSP(
            this, &SAssetOrganizerWidget::OnCustomRulesExternallyChanged);
    }

    // Check if settings loaded successfully
    if (!Settings)
    {
        UE_LOG(LogTemp, Error, TEXT("AssetOrganizer: Failed to load settings!"));
        return;
    }

    // Initialize default asset types if empty
    if (Settings->AssetTypeConfigs.Num() == 0)
    {
        Settings->InitializeDefaultAssetTypes();
        Settings->SaveConfig();
    }

    // Initialize categories from settings
    Categories.Empty();

    TMap<FName, FAssetCategory*> CategoryMap;

    // Create categories based on settings
    for (const FAssetTypeInfo& Info : Settings->AssetTypeConfigs)
    {
        if (!Info.bEnabled)
            continue;

        FAssetCategory** CatPtr = CategoryMap.Find(Info.Category);
        if (!CatPtr)
        {
            FAssetCategory NewCat(Info.Category.ToString());
            int32 CatIndex = Categories.Add(NewCat);
            CategoryMap.Add(Info.Category, &Categories[CatIndex]);
            CatPtr = CategoryMap.Find(Info.Category);
        }

        FAssetTypeRow Row(Info.Type, Info.ClassName, Info.DisplayName.ToString(), Info.Prefix, Info.TargetPath, Info.bEnabled);
        Row.ClassName = Info.ClassName;
        Row.Priority = Info.Priority;
        (*CatPtr)->Rows.Add(Row);
    }

    // Note: "Other/Uncategorized" category is already defined in settings (AssetTypeConfigs)
    // No need to manually add it here to avoid duplication

    // Build UI
    CategoryContainer = SNew(SVerticalBox);

    ChildSlot
    [
        SNew(SVerticalBox)

        // Toolbar
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildToolbar()
        ]

        // Settings Panel (collapsed by default)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SAssignNew(SettingsPanel, SBox)
            .Visibility(EVisibility::Collapsed)
            [
                BuildSettingsPanel()
            ]
        ]

        // Whitelist panel (collapsed by default)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SAssignNew(WhitelistPanel, SBox)
            .Visibility(EVisibility::Collapsed)
            [
                BuildWhitelistPanel()
            ]
        ]

        // Table header
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildTableHeader()
        ]

        // Scrollable asset list
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        [
            BuildScrollArea()
        ]

        // Log panel
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildLogPanel()
        ]

        // History panel
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildHistoryPanel()
        ]

        // Preview panel
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildPreviewPanel()
        ]

        // Progress overlay (hidden by default)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildProgressOverlay()
        ]

        // Status bar
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BuildStatusBar()
        ]
    ];

    RefreshAssetData();
    RebuildCategoryList();
    RefreshHistoryList();
    RebuildHistoryListUI();

    // Initialize whitelist data from settings and populate UI
    if (Settings)
    {
        WhitelistFolders = Settings->WhitelistedFolders;
        if (WhitelistContainer.IsValid())
        {
            WhitelistContainer->ClearChildren();
            for (int32 i = 0; i < WhitelistFolders.Num(); ++i)
            {
                const FString FolderPath = WhitelistFolders[i];
                const int32 Idx = i;
                WhitelistContainer->AddSlot()
                    .AutoHeight()
                    .Padding(FMargin(0.f, 1.f))
                    [
                        SNew(SBorder)
                        .BorderBackgroundColor(FSlateColor(AOColors::BgRow))
                        .Padding(FMargin(8.f, 3.f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(FolderPath))
                                .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                                .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(SButton)
                                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .OnClicked_Lambda([this, Idx]() -> FReply
                                {
                                    return OnRemoveWhitelistFolder(Idx);
                                })
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("RemoveWhitelist", "Remove"))
                                    .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                                ]
                            ]
                        ]
                    ];
            }
        }
    }

    // Populate custom rules list
    RebuildCustomRulesList();
    RefreshClassSelectorOptions();

    // Register asset registry delegates for incremental refresh
    FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
    if (AssetRegistryModule)
    {
        IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();

        OnAssetAddedHandle = AssetRegistry.OnAssetAdded().AddSP(this, &SAssetOrganizerWidget::OnAssetAddedCallback);
        OnAssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddSP(this, &SAssetOrganizerWidget::OnAssetRemovedCallback);
        OnAssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddSP(this, &SAssetOrganizerWidget::OnAssetRenamedCallback);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolbar
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildToolbar()
{
    auto MakeToolbarBtn = [&](
        const FText& Label,
        const FText& SubLabel,
        FOnClicked OnClicked,
        bool bPrimary = false,
        bool bEnabled = true) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .Padding(FMargin(2.f, 0.f))
            [
                SNew(SButton)
                .IsEnabled(bEnabled)
                .ButtonColorAndOpacity(bPrimary
                    ? FLinearColor(0.08f, 0.28f, 0.45f, 1.f)
                    : AOColors::BgToolbarBtn)
                .OnClicked(OnClicked)
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Center)
                .ContentPadding(FMargin(10.f, 4.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(Label)
                            .Font(FCoreStyle::Get().GetFontStyle("NormalText"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(SubLabel)
                            .Font(FCoreStyle::Get().GetFontStyle("SmallText"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]
            ];
    };

    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgToolbar))
        .Padding(FMargin(8.f, 6.f))
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("OrganizeBtn", "一键整理"),
                    LOCTEXT("OrganizeSub", "Organize All"),
                    FOnClicked::CreateSP(this, &SAssetOrganizerWidget::OnOrganizeClicked),
                    true
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(FMargin(6.f, 4.f))
            [
                SNew(SSeparator).Orientation(Orient_Vertical)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("PreviewBtn", "预览"),
                    LOCTEXT("PreviewSub", "Preview"),
                    FOnClicked::CreateSP(this, &SAssetOrganizerWidget::OnPreviewClicked)
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("RefreshBtn", "刷新"),
                    LOCTEXT("RefreshSub", "Refresh"),
                    FOnClicked::CreateSP(this, &SAssetOrganizerWidget::OnRefreshClicked)
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(FMargin(6.f, 4.f))
            [
                SNew(SSeparator).Orientation(Orient_Vertical)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("HistoryBtn", "历史"),
                    LOCTEXT("HistorySub", "History/Undo"),
                    FOnClicked::CreateSP(this, &SAssetOrganizerWidget::OnShowHistoryClicked)
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("LogBtn", "日志"),
                    LOCTEXT("LogSub", "Log"),
                    FOnClicked::CreateLambda([this]() -> FReply
                    {
                        ToggleLogVisibility();
                        return FReply::Handled();
                    })
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("SettingsBtn", "设置主文件夹"),
                    LOCTEXT("SettingsSub", "Root Folder"),
                    FOnClicked::CreateLambda([this]() -> FReply
                    {
                        ToggleSettingsVisibility();
                        return FReply::Handled();
                    })
                )
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeToolbarBtn(
                    LOCTEXT("WhitelistBtn", "白名单"),
                    LOCTEXT("WhitelistSub", "Whitelist"),
                    FOnClicked::CreateLambda([this]() -> FReply
                    {
                        bIsWhitelistVisible = !bIsWhitelistVisible;
                        if (WhitelistPanel.IsValid())
                        {
                            WhitelistPanel->SetVisibility(bIsWhitelistVisible
                                ? EVisibility::Visible : EVisibility::Collapsed);
                        }
                        return FReply::Handled();
                    })
                )
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.f)

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("VersionLabel", "v2.0"))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings Panel
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildSettingsPanel()
{
    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgPanel))
        .Padding(FMargin(12.f, 8.f))
        [
            SNew(SVerticalBox)

            // Title
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 0.f, 0.f, 8.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SettingsTitle", "Settings"))
                .Font(FCoreStyle::Get().GetFontStyle("NormalText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
            ]

            // Separator
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 0.f, 0.f, 8.f))
            [
                SNew(SSeparator)
            ]

            // Root Folder Wrapper Settings
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 4.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("RootFolderTitle", "Root Folder Wrapper (Optional)"))
                .Font(FCoreStyle::Get().GetFontStyle("SmallText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
            ]

            // Enable Root Folder checkbox
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 4.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SAssignNew(RootFolderCheckBox, SCheckBox)
                    .IsChecked_Lambda([this]() -> ECheckBoxState
                    {
                        return Settings && Settings->RootFolderConfig.bEnabled
                            ? ECheckBoxState::Checked
                            : ECheckBoxState::Unchecked;
                    })
                    .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
                    {
                        if (Settings)
                        {
                            Settings->RootFolderConfig.bEnabled = (NewState == ECheckBoxState::Checked);
                            Settings->SaveConfig();
                            ApplyRootFolderChange();
                        }
                    })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(8.f, 0.f, 0.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("EnableRootFolder", "Enable Root Folder Wrapper"))
                    .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                ]
            ]

            // Root Folder Name input
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(24.f, 4.f, 0.f, 4.f))  // Indented
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("RootFolderNameLabel", "Folder Name: "))
                    .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SAssignNew(RootFolderNameBox, SEditableTextBox)
                    .Text_Lambda([this]() -> FText
                    {
                        if (Settings)
                        {
                            return FText::FromString(Settings->RootFolderConfig.FolderName);
                        }
                        return FText::FromString(TEXT("Organized"));
                    })
                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
                    {
                        if (Settings)
                        {
                            FString NewName = NewText.ToString().TrimStartAndEnd();
                            if (!NewName.IsEmpty())
                            {
                                // Validate folder name (no slashes, no spaces)
                                NewName = NewName.Replace(TEXT("/"), TEXT(""));
                                NewName = NewName.Replace(TEXT("\\"), TEXT(""));
                                NewName = NewName.Replace(TEXT(" "), TEXT("_"));

                                Settings->RootFolderConfig.FolderName = NewName;
                                Settings->SaveConfig();
                                ApplyRootFolderChange();
                            }
                        }
                    })
                    .MinDesiredWidth(150.f)
                    .IsEnabled_Lambda([this]() -> bool
                    {
                        return Settings && Settings->RootFolderConfig.bEnabled;
                    })
                ]
            ]

            // Preview of paths
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(24.f, 4.f, 0.f, 0.f))
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    if (Settings && Settings->RootFolderConfig.bEnabled)
                    {
                        FString FolderName = Settings->RootFolderConfig.FolderName;
                        return FText::Format(
                            LOCTEXT("RootFolderPreview", "Preview: /Game/{0}/Materials, /Game/{0}/Blueprints, ..."),
                            FText::FromString(FolderName)
                        );
                    }
                    return LOCTEXT("RootFolderDisabled", "Preview: /Game/Materials, /Game/Blueprints, ... (disabled)");
                })
                .Font(FCoreStyle::Get().GetFontStyle("TinyText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
            ]

            // Description text
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 8.f, 0.f, 0.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("RootFolderDesc", "When enabled, all organized assets will be placed under a single parent folder for cleaner project structure."))
                .Font(FCoreStyle::Get().GetFontStyle("TinyText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                .AutoWrapText(true)
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Table Header
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildTableHeader()
{
    auto MakeTH = [&](const FText& Label, float FixW = 0.f) -> TSharedRef<SWidget>
    {
        TSharedRef<STextBlock> TB = SNew(STextBlock)
            .Text(Label)
            .Font(FCoreStyle::Get().GetFontStyle("SmallText"))
            .ColorAndOpacity(FSlateColor(AOColors::TextSecondary));

        if (FixW > 0.f)
        {
            return SNew(SBox).WidthOverride(FixW).VAlign(VAlign_Center)[ TB ];
        }
        return SNew(SBox).VAlign(VAlign_Center)[ TB ];
    };

    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgHeader))
        .Padding(FMargin(12.f, 5.f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[ MakeTH(LOCTEXT("ThActive", "Active"), 44.f) ]
            + SHorizontalBox::Slot().FillWidth(1.5f).Padding(FMargin(4.f, 0.f))[ MakeTH(LOCTEXT("ThType", "Type")) ]
            + SHorizontalBox::Slot().AutoWidth()[ MakeTH(LOCTEXT("ThPrefix", "Prefix"), 50.f) ]
            + SHorizontalBox::Slot().FillWidth(2.f).Padding(FMargin(8.f, 0.f))[ MakeTH(LOCTEXT("ThPath", "Target Path")) ]
            + SHorizontalBox::Slot().AutoWidth()[ MakeTH(LOCTEXT("ThCount", "Pending/Total"), 100.f) ]
            + SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(28.f) ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Scroll Area
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildScrollArea()
{
    CustomRulesContainer = SNew(SVerticalBox);

    return SNew(SScrollBox)
        .Orientation(Orient_Vertical)

        // Built-in asset type categories
        + SScrollBox::Slot()
        [
            CategoryContainer.ToSharedRef()
        ]

        // Custom rules section
        + SScrollBox::Slot()
        .Padding(FMargin(0.f, 4.f, 0.f, 0.f))
        [
            SNew(SVerticalBox)

            // Section header
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderBackgroundColor(FSlateColor(AOColors::BgCategory))
                .Padding(FMargin(12.f, 5.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("CustomRulesHeader", "Custom Rules"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextCategory))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f))
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .ContentPadding(FMargin(6.f, 2.f))
                        .OnClicked(this, &SAssetOrganizerWidget::OnRefreshCustomRulesClicked)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("RefreshCustomRules", "Refresh"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f))
                    [
                        SAssignNew(ClassSelectorComboBox, SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&AssetClassOptions)
                        .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewSelection, ESelectInfo::Type)
                        {
                            SelectedClassOption = NewSelection;
                        })
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
                        {
                            return SNew(STextBlock)
                                .Text(FText::FromString(InOption.IsValid() ? *InOption : TEXT("")))
                                .ColorAndOpacity(FSlateColor(AOColors::TextPrimary));
                        })
                        .Content()
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() -> FText
                            {
                                return SelectedClassOption.IsValid()
                                    ? FText::FromString(*SelectedClassOption)
                                    : LOCTEXT("SelectClass", "Select asset class...");
                            })
                            .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .ButtonColorAndOpacity(FLinearColor(0.08f, 0.28f, 0.45f, 1.f))
                        .ContentPadding(FMargin(8.f, 2.f))
                        .OnClicked(this, &SAssetOrganizerWidget::OnAddCustomRuleWithSelectionClicked)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("AddCustomRuleBtn", "+ Add Rule"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                        ]
                    ]
                ]
            ]

            // Custom rule rows
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                CustomRulesContainer.ToSharedRef()
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Category List
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::RebuildCategoryList()
{
    CategoryContainer->ClearChildren();

    // Sort categories with fixed order based on UE5 Style Guide
    // IMPORTANT: We now use indices in Slate lambdas instead of pointers,
    // so sorting is safe and won't cause dangling pointer issues.
    // This ensures categories never "jump" position when refreshing.
    Categories.Sort([](const FAssetCategory& A, const FAssetCategory& B)
    {
        // Define fixed category order (lower = higher priority)
        auto GetCategoryPriority = [](const FString& CategoryName) -> int32
        {
            // Based on https://github.com/Allar/ue5-style-guide and UE official documentation
            static const TMap<FString, int32> PriorityMap =
            {
                // 1. Levels - Always first (most important)
                { TEXT("关卡"),       1 },
                { TEXT("Levels"),     1 },

                // 2. Meshes - Core visual assets
                { TEXT("Meshes"),     10 },

                // 3. Materials - Depend on textures, used by meshes
                { TEXT("Materials"),  20 },

                // 4. Textures - Foundation assets
                { TEXT("Textures"),   30 },

                // 5. Blueprints - Core gameplay logic
                { TEXT("Blueprints"), 40 },

                // 6. Animation - Character/gameplay animation
                { TEXT("Animation"),  50 },

                // 7. Audio - Sound assets
                { TEXT("Audio"),      60 },

                // 8. VFX - Visual effects
                { TEXT("VFX"),        70 },
                { TEXT("Effects"),    71 },

                // 9. AI - Behavior trees, blackboards
                { TEXT("AI"),         80 },

                // 10. Data - Data tables, curves
                { TEXT("Data"),       90 },

                // 11. Input - Input actions, mapping contexts
                { TEXT("Input"),      100 },

                // 12. Environment - Foliage, landscape
                { TEXT("Environment"), 110 },

                // 13. Media - Video, media players
                { TEXT("Media"),      120 },

                // 14. Other/Uncategorized - Last
                { TEXT("Other"),      900 },
                { TEXT("Uncategorized"), 901 },
            };

            if (const int32* Found = PriorityMap.Find(CategoryName))
            {
                return *Found;
            }
            // Unknown categories go at the end
            return 999;
        };

        int32 PriorityA = GetCategoryPriority(A.Name);
        int32 PriorityB = GetCategoryPriority(B.Name);

        // If same priority, sort alphabetically for stability
        if (PriorityA == PriorityB)
        {
            return A.Name < B.Name;
        }

        return PriorityA < PriorityB;
    });

    for (int32 CatIdx = 0; CatIdx < Categories.Num(); ++CatIdx)
    {
        CategoryContainer->AddSlot()
            .AutoHeight()
            [
                BuildCategoryHeader(Categories[CatIdx], CatIdx)
            ];

        TSharedRef<SVerticalBox> RowBox = SNew(SVerticalBox);

        // Sort rows within category by priority for stable display order
        Categories[CatIdx].Rows.Sort([](const FAssetTypeRow& A, const FAssetTypeRow& B)
        {
            return A.Priority < B.Priority;
        });

        for (int32 RowIdx = 0; RowIdx < Categories[CatIdx].Rows.Num(); ++RowIdx)
        {
            RowBox->AddSlot()
                .AutoHeight()
                [
                    BuildAssetRow(CatIdx, RowIdx)
                ];
        }

        CategoryContainer->AddSlot()
            .AutoHeight()
            [
                SNew(SBox)
                .Visibility_Lambda([this, CatIdx]()
                {
                    if (Categories.IsValidIndex(CatIdx))
                    {
                        return Categories[CatIdx].bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
                    }
                    return EVisibility::Collapsed;
                })
                [
                    RowBox
                ]
            ];
    }
}

TSharedRef<SWidget> SAssetOrganizerWidget::BuildCategoryHeader(FAssetCategory& Cat, int32 CategoryIndex)
{
    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgCategory))
        .Padding(FMargin(12.f, 5.f))
        [
            SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .HAlign(HAlign_Fill)
            .OnClicked_Lambda([this, CategoryIndex]() -> FReply
            {
                if (Categories.IsValidIndex(CategoryIndex))
                {
                    Categories[CategoryIndex].bExpanded = !Categories[CategoryIndex].bExpanded;
                    Invalidate(EInvalidateWidget::LayoutAndVolatility);
                }
                return FReply::Handled();
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0.f, 0.f, 6.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, CategoryIndex]()
                    {
                        if (Categories.IsValidIndex(CategoryIndex))
                        {
                            return Categories[CategoryIndex].bExpanded
                                ? FText::FromString(TEXT("\u25bc"))
                                : FText::FromString(TEXT("\u25b6"));
                        }
                        return FText::FromString(TEXT("\u25b6"));
                    })
                    .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, CategoryIndex]()
                    {
                        if (Categories.IsValidIndex(CategoryIndex))
                        {
                            return FText::FromString(Categories[CategoryIndex].Name);
                        }
                        return FText::GetEmpty();
                    })
                    .ColorAndOpacity(FSlateColor(AOColors::TextCategory))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, CategoryIndex]()
                    {
                        if (Categories.IsValidIndex(CategoryIndex))
                        {
                            return FText::Format(LOCTEXT("CatCount", "({0}/{1})"),
                                FText::AsNumber(Categories[CategoryIndex].TotalPending),
                                FText::AsNumber(Categories[CategoryIndex].TotalCount));
                        }
                        return FText::GetEmpty();
                    })
                    .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                ]
            ]
        ];
}

TSharedRef<SWidget> SAssetOrganizerWidget::BuildAssetRow(int32 CategoryIndex, int32 RowIndex)
{
    FLinearColor RowBg = AOColors::BgRow;

    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(RowBg))
        .Padding(FMargin(12.f, 0.f))
        [
            SNew(SBox)
            .HeightOverride(28.f)
            [
                SNew(SHorizontalBox)

                // Checkbox
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(44.f)
                    .HAlign(HAlign_Center)
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([this, CategoryIndex, RowIndex]()
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                return Categories[CategoryIndex].Rows[RowIndex].bActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                            }
                            return ECheckBoxState::Unchecked;
                        })
                        .OnCheckStateChanged_Lambda([this, CategoryIndex, RowIndex](ECheckBoxState NewState)
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                Categories[CategoryIndex].Rows[RowIndex].bActive = (NewState == ECheckBoxState::Checked);
                            }
                        })
                    ]
                ]

                // Type name
                + SHorizontalBox::Slot()
                .FillWidth(1.5f)
                .VAlign(VAlign_Center)
                .Padding(FMargin(4.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text_Lambda([this, CategoryIndex, RowIndex]()
                    {
                        if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                        {
                            return FText::FromString(Categories[CategoryIndex].Rows[RowIndex].TypeName);
                        }
                        return FText::GetEmpty();
                    })
                    .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                ]

                // Prefix
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(50.f)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, CategoryIndex, RowIndex]()
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                const FString& Prefix = Categories[CategoryIndex].Rows[RowIndex].Prefix;
                                return FText::FromString(Prefix.IsEmpty() ? TEXT("\u2014") : Prefix);
                            }
                            return FText::FromString(TEXT("\u2014"));
                        })
                        .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrefix))
                    ]
                ]

                // Target path (editable)
                + SHorizontalBox::Slot()
                .FillWidth(2.f)
                .VAlign(VAlign_Center)
                .Padding(FMargin(8.f, 0.f))
                [
                    SNew(SEditableTextBox)
                    .Text_Lambda([this, CategoryIndex, RowIndex]()
                    {
                        if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                        {
                            return FText::FromString(Categories[CategoryIndex].Rows[RowIndex].TargetPath);
                        }
                        return FText::GetEmpty();
                    })
                    .OnTextCommitted_Lambda([this, CategoryIndex, RowIndex](const FText& NewText, ETextCommit::Type)
                    {
                        if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                        {
                            OnTargetPathEdited(&Categories[CategoryIndex].Rows[RowIndex], NewText);
                        }
                    })
                ]

                // Count capsule (shows Pending/Total)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(100.f)
                    .HAlign(HAlign_Left)
                    [
                        SNew(SButton)
                        .IsEnabled_Lambda([this, CategoryIndex, RowIndex]()
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                return Categories[CategoryIndex].Rows[RowIndex].Count > 0;
                            }
                            return false;
                        })
                        .ButtonColorAndOpacity_Lambda([this, CategoryIndex, RowIndex]()
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                const FAssetTypeRow& Row = Categories[CategoryIndex].Rows[RowIndex];
                                if (Row.PendingCount == 0 && Row.Count > 0)
                                    return AOColors::BgCapsuleGreen;
                                return Row.Count > 0 ? AOColors::BgCapsule : FLinearColor(0.1f, 0.1f, 0.1f, 1.f);
                            }
                            return FLinearColor(0.1f, 0.1f, 0.1f, 1.f);
                        })
                        .HAlign(HAlign_Center)
                        .OnClicked_Lambda([this, CategoryIndex, RowIndex]() -> FReply
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                const FAssetTypeRow& Row = Categories[CategoryIndex].Rows[RowIndex];
                                if (Row.Count > 0)
                                {
                                    FocusAndSelectAssets(Row.TargetPath, Row.Type);
                                }
                            }
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this, CategoryIndex, RowIndex]()
                            {
                                if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                                {
                                    const FAssetTypeRow& Row = Categories[CategoryIndex].Rows[RowIndex];
                                    return FText::Format(LOCTEXT("CountFmt", "{0}/{1}"),
                                        FText::AsNumber(Row.PendingCount),
                                        FText::AsNumber(Row.Count));
                                }
                                return FText::FromString(TEXT("0/0"));
                            })
                            .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextCapsule))
                        ]
                    ]
                ]

                // Link button
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(28.f)
                    .HAlign(HAlign_Center)
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked_Lambda([this, CategoryIndex, RowIndex]() -> FReply
                        {
                            if (Categories.IsValidIndex(CategoryIndex) && Categories[CategoryIndex].Rows.IsValidIndex(RowIndex))
                            {
                                SyncBrowserToFolder(Categories[CategoryIndex].Rows[RowIndex].TargetPath);
                            }
                            return FReply::Handled();
                        })
                        [
                            SNew(SImage)
                            .Image(FEditorStyle::Get().GetBrush("Icons.ExternalLink"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Status Bar
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildStatusBar()
{
    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgHeader))
        .Padding(FMargin(12.f, 3.f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(0.f, 0.f, 16.f, 0.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("StatusReady", "\u2022 Ready"))
                .ColorAndOpacity(FLinearColor(0.3f, 0.7f, 0.3f, 1.f))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(0.f, 0.f, 16.f, 0.f))
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    return FText::Format(LOCTEXT("TotalFmt", "Pending: {0} / Total: {1}"),
                        FText::AsNumber(GetTotalPendingCount()),
                        FText::AsNumber(GetTotalAssetCount()));
                })
                .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("VersionStr", "UE 4.27 \u00b7 Asset Organizer v2.0"))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress Overlay
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildProgressOverlay()
{
    return SAssignNew(ProgressOverlay, SBox)
        .Visibility(EVisibility::Collapsed)
        .Padding(FMargin(12.f, 6.f))
        [
            SNew(SBorder)
            .BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.15f, 1.f))
            .Padding(FMargin(12.f, 8.f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(ProgressText, STextBlock)
                    .Text(LOCTEXT("ProgressInit", "Initializing..."))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 4.f))
                [
                    SAssignNew(ProgressBar, SProgressBar)
                    .Percent(0.f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                [
                    SAssignNew(CancelButton, SButton)
                    .Text(LOCTEXT("CancelBtn", "Cancel"))
                    .OnClicked(this, &SAssetOrganizerWidget::OnCancelClicked)
                ]
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// Log Panel
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildLogPanel()
{
    return SNew(SBox)
        .Visibility_Lambda([this]() { return bIsLogVisible ? EVisibility::Visible : EVisibility::Collapsed; })
        .HeightOverride(150.f)
        [
            SNew(SBorder)
            .BorderBackgroundColor(FSlateColor(AOColors::BgHeader))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(4.f, 2.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("LogPanelTitle", "Execution Log"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked_Lambda([this]() -> FReply
                        {
                            ClearLog();
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ClearLog", "Clear"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.f)
                .Padding(FMargin(4.f, 0.f, 4.f, 4.f))
                [
                    SAssignNew(LogTextBox, SMultiLineEditableTextBox)
                    .Text_Lambda([this]() { return FText::FromString(LogContent); })
                    .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                    .IsReadOnly(true)
                    .AutoWrapText(false)
                ]
            ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// History Panel
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildHistoryPanel()
{
    return SAssignNew(HistoryPanel, SBox)
        .Visibility_Lambda([this]() { return bIsHistoryVisible ? EVisibility::Visible : EVisibility::Collapsed; })
        .HeightOverride(220.f)
        [
            SNew(SBorder)
            .BorderBackgroundColor(FSlateColor(AOColors::BgHeader))
            .Padding(FMargin(8.f, 4.f))
            [
                SNew(SVerticalBox)

                // Header row
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 0.f, 0.f, 4.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("HistoryTitle", "Operation History"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked_Lambda([this]() -> FReply
                        {
                            bIsHistoryVisible = false;
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("CloseHistory", "Close"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]

                // History list (scrollable)
                + SVerticalBox::Slot()
                .FillHeight(1.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(HistoryListContainer, SVerticalBox)
                    ]
                ]
            ]
        ];
}

void SAssetOrganizerWidget::RebuildHistoryListUI()
{
    if (!HistoryListContainer.IsValid())
        return;

    HistoryListContainer->ClearChildren();

    // Header with Clear All button
    HistoryListContainer->AddSlot()
        .AutoHeight()
        .Padding(FMargin(0.f, 0.f, 0.f, 8.f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("HistoryTitle", "Operation History"))
                .Font(FCoreStyle::Get().GetFontStyle("NormalText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SButton)
                .ButtonColorAndOpacity(FLinearColor(0.4f, 0.15f, 0.15f, 1.f))  // Dark red
                .ContentPadding(FMargin(8.f, 2.f))
                .OnClicked_Lambda([this]() -> FReply
                {
                    OnClearAllHistoryClicked();
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ClearAllHistoryBtn", "Clear All"))
                    .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                ]
            ]
        ];

    if (HistoryItems.Num() == 0)
    {
        HistoryListContainer->AddSlot()
            .AutoHeight()
            .Padding(FMargin(4.f, 8.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("NoHistory", "No history entries found."))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
            ];
        return;
    }

    for (const FHistoryListItem& Item : HistoryItems)
    {
        FString FilePath = Item.FilePath;
        bool bCanRollback = Item.bCanRollback;

        HistoryListContainer->AddSlot()
            .AutoHeight()
            .Padding(FMargin(0.f, 1.f))
            [
                SNew(SBorder)
                .BorderBackgroundColor(FSlateColor(AOColors::BgRow))
                .Padding(FMargin(8.f, 4.f))
                [
                    SNew(SHorizontalBox)

                    // Timestamp
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(0.f, 0.f, 12.f, 0.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Item.Timestamp.Left(19)))
                        .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                    ]

                    // Description + moved count
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::Format(LOCTEXT("HistoryDesc", "{0}  ({1} assets moved)"),
                            FText::FromString(Item.Description),
                            FText::AsNumber(Item.MovedAssets)))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                    ]

                    // Rollback button
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(8.f, 0.f, 0.f, 0.f))
                    [
                        SNew(SButton)
                        .IsEnabled(bCanRollback)
                        .ButtonColorAndOpacity(bCanRollback
                            ? FLinearColor(0.4f, 0.2f, 0.1f, 1.f)
                            : FLinearColor(0.15f, 0.15f, 0.15f, 1.f))
                        .ContentPadding(FMargin(8.f, 2.f))
                        .OnClicked_Lambda([this, FilePath]() -> FReply
                        {
                            OnRollbackHistoryItem(FilePath);
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("RollbackBtn", "Rollback"))
                            .ColorAndOpacity(FSlateColor(bCanRollback
                                ? AOColors::TextPrimary
                                : AOColors::TextDim))
                        ]
                    ]

                    // Delete button
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f, 0.f, 0.f))
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .ContentPadding(FMargin(4.f, 2.f))
                        .OnClicked_Lambda([this, FilePath]() -> FReply
                        {
                            OnDeleteHistoryItem(FilePath);
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("DeleteHistoryBtn", "X"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]
            ];
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Preview Panel
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildPreviewPanel()
{
    return SAssignNew(PreviewPanel, SBox)
        .Visibility_Lambda([this]() { return bIsPreviewVisible ? EVisibility::Visible : EVisibility::Collapsed; })
        .HeightOverride(260.f)
        [
            SNew(SBorder)
            .BorderBackgroundColor(FSlateColor(AOColors::BgHeader))
            .Padding(FMargin(8.f, 4.f))
            [
                SNew(SVerticalBox)

                // Header
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.f, 0.f, 0.f, 4.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("PreviewTitle", "Move Preview"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(12.f, 0.f))
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            int32 NewFolders = 0;
                            int32 Conflicts = 0;
                            for (const FMovePreviewItem& Item : CurrentPreviewItems)
                            {
                                if (Item.bIsNewFolder) NewFolders++;
                                if (Item.bWillConflict) Conflicts++;
                            }
                            return FText::Format(
                                LOCTEXT("PreviewStats", "{0} moves  |  {1} new folders  |  {2} conflicts"),
                                FText::AsNumber(CurrentPreviewItems.Num()),
                                FText::AsNumber(NewFolders),
                                FText::AsNumber(Conflicts));
                        })
                        .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .OnClicked_Lambda([this]() -> FReply
                        {
                            bIsPreviewVisible = false;
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ClosePreview", "Close"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                        ]
                    ]
                ]

                // Column headers
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderBackgroundColor(FSlateColor(AOColors::BgCategory))
                    .Padding(FMargin(4.f, 2.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth()
                        [ SNew(SBox).WidthOverride(16.f) ]
                        + SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(4.f, 0.f))
                        [ SNew(STextBlock).Text(LOCTEXT("PreviewColAsset", "Asset")).ColorAndOpacity(FSlateColor(AOColors::TextSecondary)) ]
                        + SHorizontalBox::Slot().FillWidth(1.5f).Padding(FMargin(4.f, 0.f))
                        [ SNew(STextBlock).Text(LOCTEXT("PreviewColFrom", "From")).ColorAndOpacity(FSlateColor(AOColors::TextSecondary)) ]
                        + SHorizontalBox::Slot().FillWidth(1.5f).Padding(FMargin(4.f, 0.f))
                        [ SNew(STextBlock).Text(LOCTEXT("PreviewColTo", "To")).ColorAndOpacity(FSlateColor(AOColors::TextSecondary)) ]
                    ]
                ]

                // Scrollable list
                + SVerticalBox::Slot()
                .FillHeight(1.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(PreviewListContainer, SVerticalBox)
                    ]
                ]
            ]
        ];
}

void SAssetOrganizerWidget::RebuildPreviewListUI()
{
    if (!PreviewListContainer.IsValid())
        return;

    PreviewListContainer->ClearChildren();

    for (const FMovePreviewItem& Item : CurrentPreviewItems)
    {
        bool bSamePath = (Item.FromPath == Item.ToPath);
        FLinearColor RowColor = bSamePath
            ? FLinearColor(0.08f, 0.12f, 0.08f, 1.f)
            : (Item.bWillConflict ? FLinearColor(0.15f, 0.08f, 0.05f, 1.f) : AOColors::BgRow);

        FString NewFolderTag = Item.bIsNewFolder ? TEXT("[NEW] ") : TEXT("");

        PreviewListContainer->AddSlot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderBackgroundColor(FSlateColor(RowColor))
                .Padding(FMargin(4.f, 1.f))
                [
                    SNew(SHorizontalBox)

                    // Conflict / new folder indicator
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SBox)
                        .WidthOverride(16.f)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Item.bWillConflict ? TEXT("!") : (Item.bIsNewFolder ? TEXT("+") : TEXT(" "))))
                            .ColorAndOpacity(FSlateColor(Item.bWillConflict
                                ? FLinearColor(1.f, 0.5f, 0.1f, 1.f)
                                : FLinearColor(0.3f, 0.8f, 0.3f, 1.f)))
                        ]
                    ]

                    // Asset name + class
                    + SHorizontalBox::Slot()
                    .FillWidth(1.f)
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::Format(LOCTEXT("PreviewAssetFmt", "{0}  [{1}]"),
                            FText::FromString(Item.AssetName),
                            FText::FromString(Item.AssetClass)))
                        .ColorAndOpacity(FSlateColor(bSamePath ? AOColors::TextDim : AOColors::TextPrimary))
                    ]

                    // From path
                    + SHorizontalBox::Slot()
                    .FillWidth(1.5f)
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Item.FromPath))
                        .Font(FCoreStyle::Get().GetFontStyle("SmallText"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                    ]

                    // To path
                    + SHorizontalBox::Slot()
                    .FillWidth(1.5f)
                    .VAlign(VAlign_Center)
                    .Padding(FMargin(4.f, 0.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(NewFolderTag + Item.ToPath))
                        .Font(FCoreStyle::Get().GetFontStyle("SmallText"))
                        .ColorAndOpacity(FSlateColor(bSamePath ? AOColors::TextDim : AOColors::TextPrefix))
                    ]
                ]
            ];
    }
}

void SAssetOrganizerWidget::ShowPreviewDialog(const TArray<FMovePreviewItem>& PreviewItems)
{
    CurrentPreviewItems = PreviewItems;
    RebuildPreviewListUI();
    bIsPreviewVisible = true;
}

void SAssetOrganizerWidget::ClosePreviewPanel()
{
    bIsPreviewVisible = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Button Handlers
// ─────────────────────────────────────────────────────────────────────────────
FReply SAssetOrganizerWidget::OnOrganizeClicked()
{
    const EAppReturnType::Type Ret = FMessageDialog::Open(EAppMsgType::OkCancel,
        LOCTEXT("ConfirmOrganize", "This will move assets and fix references.\n\nMake sure to save and backup your project first."));

    if (Ret != EAppReturnType::Ok)
    {
        return FReply::Handled();
    }

    ClearLog();
    AppendLog(TEXT("Starting asset organization..."));

    StartAsyncOrganize(false);

    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnPreviewClicked()
{
    ClearLog();
    AppendLog(TEXT("Generating preview..."));

    StartAsyncOrganize(true);

    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnRefreshClicked()
{
    // Prevent double-click or rapid clicking
    if (bIsRefreshRunning.Load())
    {
        return FReply::Handled();
    }

    RefreshAssetData();
    RebuildCategoryList();
    ShowNotification(LOCTEXT("Refreshed", "Data refreshed!"));
    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnCancelClicked()
{
    FAssetOrganizerExecutor::CancelAsyncOperation();
    AppendLog(TEXT("Cancelling operation..."));
    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnShowHistoryClicked()
{
    RefreshHistoryList();
    bIsHistoryVisible = !bIsHistoryVisible;
    return FReply::Handled();
}

// ─────────────────────────────────────────────────────────────────────────────
// Async Execution
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::StartAsyncOrganize(bool bDryRun)
{
    if (bIsAsyncRunning)
        return;

    bIsAsyncRunning = true;
    if (ProgressOverlay.IsValid())
        ProgressOverlay->SetVisibility(EVisibility::Visible);

    FOrganizeConfig Config;
    Config.RootPath = Settings->RootPath;
    Config.bCreateFolders = Settings->bCreateFolders;
    Config.bFixReferences = Settings->bFixReferences;
    Config.bCleanEmptyFolders = Settings->bCleanEmptyFolders;
    Config.bDryRun = bDryRun;
    Config.FolderStrategy = Settings->FolderStrategy;
    Config.RootFolder = Settings->RootFolderConfig;
    Config.bSaveHistory = Settings->bEnableHistory && !bDryRun;
    Config.ExcludedFolders = Settings->ExcludedFolders;
    Config.WhitelistedFolders = Settings->WhitelistedFolders;
    Config.bUpdateWhitelistedAssetReferences = true;

    // Build type map from active rows (built-in types)
    for (FAssetCategory& Cat : Categories)
    {
        for (FAssetTypeRow& Row : Cat.Rows)
        {
            if (Row.bActive)
            {
                Config.TypeToFolderMap.Add(Row.Type, Row.TargetPath);
            }
        }
    }

    // Add custom rules to type map (all custom assets share the first enabled rule's target path)
    if (Settings)
    {
        for (const FCustomAssetRule& Rule : Settings->CustomRules)
        {
            if (Rule.bEnabled && !Rule.TargetPath.IsEmpty())
            {
                Config.TypeToFolderMap.Add(EAssetOrganizeType::Custom, Rule.TargetPath);
                break; // Only one Custom target path is supported in current design
            }
        }
    }

    FAssetOrganizerExecutor::FOnProgress ProgressDelegate;
    ProgressDelegate.BindSP(this, &SAssetOrganizerWidget::OnAsyncProgress);

    FAssetOrganizerExecutor::FOnLog LogDelegate;
    LogDelegate.BindSP(this, &SAssetOrganizerWidget::OnAsyncLog);

    FAssetOrganizerExecutor::OrganizeAssetsAsync(Config, ProgressDelegate, LogDelegate);

    // Poll for completion via ticker
    AsyncTickerHandle = FTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &SAssetOrganizerWidget::OnAsyncTick), 0.5f);
}

bool SAssetOrganizerWidget::OnAsyncTick(float DeltaTime)
{
    if (!FAssetOrganizerExecutor::IsAsyncOperationRunning())
    {
        OnAsyncComplete();
        return false; // Remove ticker
    }
    return true; // Keep ticking
}

void SAssetOrganizerWidget::OnAsyncProgress(float Percent, const FText& Message)
{
    // Execute on game thread
    AsyncTask(ENamedThreads::GameThread, [this, Percent, Message]()
    {
        UpdateProgressUI(Percent, Message);
    });
}

void SAssetOrganizerWidget::OnAsyncLog(const FString& Message)
{
    AsyncTask(ENamedThreads::GameThread, [this, Message]()
    {
        AppendLog(Message);
    });
}

void SAssetOrganizerWidget::OnAsyncComplete()
{
    bIsAsyncRunning = false;

    if (ProgressOverlay.IsValid())
        ProgressOverlay->SetVisibility(EVisibility::Collapsed);

    // Retrieve result from the completed async task
    TSharedPtr<FAsyncTask<FOrganizeAssetsTask>> Task = FAssetOrganizerExecutor::GetCurrentAsyncTask();
    if (Task.IsValid() && Task->IsDone())
    {
        LastResult = Task->GetTask().GetResult();
    }

    if (LastResult.bWasCancelled)
    {
        AppendLog(TEXT("Operation was cancelled."));
        ShowNotification(LOCTEXT("OrganizeCancelled", "Operation cancelled."), false);
    }
    else if (LastResult.PreviewItems.Num() > 0)
    {
        ShowPreviewDialog(LastResult.PreviewItems);
        AppendLog(FString::Printf(TEXT("Preview complete. %d assets will be moved."), LastResult.MovedAssets));
        ShowNotification(FText::Format(LOCTEXT("PreviewDone", "Preview: {0} assets to move"),
            FText::AsNumber(LastResult.MovedAssets)));
    }
    else if (LastResult.MovedAssets > 0 || LastResult.Errors.Num() > 0)
    {
        AppendLog(FString::Printf(TEXT("Done. Moved: %d, Fixed refs: %d, Removed folders: %d"),
            LastResult.MovedAssets, LastResult.FixedReferences, LastResult.RemovedEmptyFolders));

        if (LastResult.Errors.Num() > 0)
        {
            for (const FString& Err : LastResult.Errors)
                AppendLog(FString::Printf(TEXT("[ERROR] %s"), *Err));
            ShowNotification(FText::Format(LOCTEXT("OrganizeDoneErrors", "Done with {0} errors"),
                FText::AsNumber(LastResult.Errors.Num())), false);
        }
        else
        {
            ShowNotification(FText::Format(LOCTEXT("OrganizeComplete", "Done! Moved {0} assets"),
                FText::AsNumber(LastResult.MovedAssets)));
        }
    }
    else
    {
        AppendLog(TEXT("Organization complete. No assets needed moving."));
        ShowNotification(LOCTEXT("OrganizeNoop", "All assets already organized."));
    }

    RefreshAssetData();
    RebuildCategoryList();
    RefreshHistoryList();
    RebuildHistoryListUI();

    // Broken reference warning notification
    if (!LastResult.bWasCancelled && LastResult.PreviewItems.Num() == 0)
    {
        int32 BrokenCount = LastResult.Warnings.Num();
        if (BrokenCount > 0)
        {
            AppendLog(FString::Printf(TEXT("[WARN] %d broken reference(s) detected after organize."), BrokenCount));
            ShowNotification(FText::Format(
                LOCTEXT("BrokenRefWarn", "Warning: {0} broken reference(s) detected. Check log for details."),
                FText::AsNumber(BrokenCount)), false);
        }
    }
}

void SAssetOrganizerWidget::UpdateProgressUI(float Percent, const FText& Message)
{
    if (ProgressBar.IsValid())
    {
        ProgressBar->SetPercent(Percent);
    }
    if (ProgressText.IsValid())
    {
        ProgressText->SetText(Message);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Data Operations
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::RefreshAssetData()
{
    // Prevent concurrent refresh operations
    FScopeLock Lock(&RefreshLock);
    if (bIsRefreshRunning.Load())
    {
        return;
    }
    bIsRefreshRunning.Store(true);

    // Auto-reset flag when done (using atomic exchange for thread safety)
    struct FAutoResetFlag
    {
        TAtomic<bool>& Flag;
        FAutoResetFlag(TAtomic<bool>& InFlag) : Flag(InFlag) {}
        ~FAutoResetFlag() { Flag.Store(false); }
    } AutoReset(bIsRefreshRunning);

    if (!Settings)
    {
        LoadSettings();
        if (!Settings)
            return;
    }

    // Ensure Categories is initialized
    if (Categories.Num() == 0)
    {
        return;
    }

    FAssetRegistryModule* ARModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
    if (!ARModule)
        return;

    IAssetRegistry& AssetRegistry = ARModule->Get();

    if (AssetRegistry.IsLoadingAssets())
        return;

    // Reset counts
    for (FAssetCategory& Cat : Categories)
    {
        Cat.TotalCount = 0;
        Cat.TotalPending = 0;
        for (FAssetTypeRow& Row : Cat.Rows)
        {
            Row.Count = 0;
            Row.PendingCount = 0;
        }
    }

    // Scan assets
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Settings->RootPath));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AllAssets;
    AssetRegistry.GetAssets(Filter, AllAssets);

    // Count assets - use a set to track which assets we've already counted
    // to prevent double-counting if an asset matches multiple types
    TSet<FName> CountedPackages;

    for (const FAssetData& Asset : AllAssets)
    {
        if (Asset.AssetClass == FName(TEXT("ObjectRedirector")))
            continue;

        // Skip if already counted
        if (CountedPackages.Contains(Asset.PackageName))
            continue;

        EAssetOrganizeType AssetType = FAssetOrganizerExecutor::DetermineAssetType(Asset, &Settings->CustomRules);
        FString AssetPath = Asset.PackagePath.ToString();

        bool bMatched = false;
        for (FAssetCategory& Cat : Categories)
        {
            for (FAssetTypeRow& Row : Cat.Rows)
            {
                if (Row.Type == AssetType)
                {
                    Row.Count++;
                    Cat.TotalCount++;
                    FString EffectiveTargetPath = GetEffectiveTargetPath(Row.TargetPath);
                    if (!AssetPath.StartsWith(EffectiveTargetPath))
                    {
                        Row.PendingCount++;
                        Cat.TotalPending++;
                    }
                    CountedPackages.Add(Asset.PackageName);
                    bMatched = true;
                    break;
                }
            }
            if (bMatched)
                break;
        }
    }
}

int32 SAssetOrganizerWidget::GetTotalAssetCount() const
{
    int32 Total = 0;
    for (const FAssetCategory& Cat : Categories)
    {
        Total += Cat.TotalCount;
    }
    return Total;
}

int32 SAssetOrganizerWidget::GetTotalPendingCount() const
{
    int32 Total = 0;
    for (const FAssetCategory& Cat : Categories)
    {
        Total += Cat.TotalPending;
    }
    return Total;
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::LoadSettings()
{
    Settings = GetMutableDefault<UAssetOrganizerSettings>();
    if (!Settings)
    {
        Settings = NewObject<UAssetOrganizerSettings>();
    }
}

void SAssetOrganizerWidget::SaveSettings()
{
    if (Settings)
    {
        Settings->SaveConfig();
    }
}

void SAssetOrganizerWidget::OnTargetPathEdited(FAssetTypeRow* Row, const FText& NewText)
{
    if (Row && Settings)
    {
        Row->TargetPath = NewText.ToString();
        Settings->SetTypeTargetPath(Row->Type, Row->TargetPath);
        SaveSettings();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// History
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::RefreshHistoryList()
{
    HistoryItems.Empty();

    TArray<FString> HistoryFiles = FAssetOrganizerExecutor::GetHistoryFiles(10);
    for (const FString& FilePath : HistoryFiles)
    {
        FOrganizeHistoryEntry Entry;
        if (FAssetOrganizerExecutor::LoadHistory(FilePath, Entry))
        {
            FHistoryListItem Item;
            Item.Id = Entry.Id;
            Item.Timestamp = Entry.Timestamp;
            Item.Description = Entry.Description;
            Item.MovedAssets = Entry.MovedAssets;
            Item.bCanRollback = Entry.bCanRollback;
            Item.FilePath = FilePath;
            HistoryItems.Add(Item);
        }
    }
}

void SAssetOrganizerWidget::OnRollbackHistoryItem(const FString& HistoryFile)
{
    const EAppReturnType::Type Ret = FMessageDialog::Open(EAppMsgType::OkCancel,
        LOCTEXT("ConfirmRollback", "Rollback will move assets back to their original locations.\n\nContinue?"));

    if (Ret == EAppReturnType::Ok)
    {
        FAssetOrganizerExecutor::FOnProgress ProgressDelegate;
        ProgressDelegate.BindSP(this, &SAssetOrganizerWidget::OnAsyncProgress);

        FOrganizeResult Result = FAssetOrganizerExecutor::RollbackOrganize(HistoryFile, ProgressDelegate);

        if (Result.Errors.Num() == 0)
        {
            ShowNotification(LOCTEXT("RollbackSuccess", "Rollback successful!"));
        }
        else
        {
            ShowErrorNotification(FText::FromString(Result.Errors[0]));
        }

        RefreshAssetData();
        RebuildCategoryList();
        RefreshHistoryList();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Content Browser Integration
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::FocusAndSelectAssets(const FString& FolderPath, EAssetOrganizeType Type)
{
    FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

    TArray<FString> Folders;
    Folders.Add(FolderPath);
    CBModule.Get().SyncBrowserToFolders(Folders);

    FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = ARModule.Get();

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*FolderPath));
    Filter.bRecursivePaths = false;

    FName ClassName = FAssetOrganizerExecutor::GetClassNameFromType(Type);
    if (!ClassName.IsNone())
    {
        Filter.ClassNames.Add(ClassName);
    }

    TArray<FAssetData> Assets;
    AssetRegistry.GetAssets(Filter, Assets);

    if (Assets.Num() > 0)
    {
        CBModule.Get().SyncBrowserToAssets(Assets, true);
    }
}

void SAssetOrganizerWidget::SyncBrowserToFolder(const FString& FolderPath)
{
    FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FString> Folders;
    Folders.Add(FolderPath);
    CBModule.Get().SyncBrowserToFolders(Folders);
    ShowNotification(FText::Format(LOCTEXT("NavigatedTo", "Navigated to: {0}"), FText::FromString(FolderPath)));
}

// ─────────────────────────────────────────────────────────────────────────────
// Notifications
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::ShowNotification(const FText& Message, bool bSuccess)
{
    FNotificationInfo Info(Message);
    Info.ExpireDuration = 2.8f;
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> Notif = FSlateNotificationManager::Get().AddNotification(Info);
    if (Notif.IsValid())
    {
        Notif->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
    }
}

void SAssetOrganizerWidget::ShowErrorNotification(const FText& Message)
{
    ShowNotification(Message, false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Log
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::AppendLog(const FString& Message)
{
    FDateTime Now = FDateTime::Now();
    FString Timestamp = Now.ToString(TEXT("%H:%M:%S"));
    LogContent += FString::Printf(TEXT("[%s] %s\n"), *Timestamp, *Message);

    if (LogTextBox.IsValid())
    {
        // Scroll to bottom by moving caret to end
        FTextLocation EndLocation(LogContent.Len());
        LogTextBox->GoTo(EndLocation);
    }
}

void SAssetOrganizerWidget::ClearLog()
{
    LogContent.Empty();
}

void SAssetOrganizerWidget::ToggleLogVisibility()
{
    bIsLogVisible = !bIsLogVisible;
}

void SAssetOrganizerWidget::ToggleSettingsVisibility()
{
    bIsSettingsVisible = !bIsSettingsVisible;
    if (SettingsPanel.IsValid())
    {
        SettingsPanel->SetVisibility(bIsSettingsVisible ? EVisibility::Visible : EVisibility::Collapsed);
        SettingsPanel->Invalidate(EInvalidateWidget::LayoutAndVolatility);
    }
}

FString SAssetOrganizerWidget::GetEffectiveTargetPath(const FString& BasePath) const
{
    if (!Settings || !Settings->RootFolderConfig.bEnabled || Settings->RootFolderConfig.FolderName.IsEmpty())
    {
        return BasePath;
    }

    // Apply root folder prefix
    if (BasePath.StartsWith(TEXT("/Game/")))
    {
        FString SubPath = BasePath.Mid(6); // Remove "/Game/"
        return FString::Printf(TEXT("/Game/%s/%s"), *Settings->RootFolderConfig.FolderName, *SubPath);
    }
    return BasePath;
}

void SAssetOrganizerWidget::ApplyRootFolderChange()
{
    // Row.TargetPath stores the BASE path (without root folder prefix)
    // The root folder prefix is applied dynamically by GetEffectiveTargetPath
    // So we just need to refresh the UI display here

    // Refresh the UI
    RebuildCategoryList();

    // Log the change
    if (Settings)
    {
        if (Settings->RootFolderConfig.bEnabled)
        {
            AppendLog(FString::Printf(TEXT("Root folder enabled: /Game/%s/"), *Settings->RootFolderConfig.FolderName));
        }
        else
        {
            AppendLog(TEXT("Root folder disabled. Assets will be organized directly under /Game/"));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// History helpers
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::OnDeleteHistoryItem(const FString& HistoryFile)
{
    FAssetOrganizerExecutor::DeleteHistory(HistoryFile);
    RefreshHistoryList();
    RebuildHistoryListUI();
}

void SAssetOrganizerWidget::OnClearAllHistoryClicked()
{
    const EAppReturnType::Type Ret = FMessageDialog::Open(EAppMsgType::OkCancel,
        LOCTEXT("ConfirmClearAllHistory", "This will delete ALL history entries.\n\nAre you sure?"));

    if (Ret == EAppReturnType::Ok)
    {
        TArray<FString> DeletedFiles;
        int32 DeletedCount = FAssetOrganizerExecutor::ClearAllHistory(&DeletedFiles);

        RefreshHistoryList();
        RebuildHistoryListUI();

        ShowNotification(FText::Format(
            LOCTEXT("ClearAllHistorySuccess", "Deleted {0} history entries"),
            FText::AsNumber(DeletedCount)));
    }
}

FReply SAssetOrganizerWidget::OnRollbackClicked()
{
    TArray<FString> Files = FAssetOrganizerExecutor::GetHistoryFiles(1);
    if (Files.Num() > 0)
    {
        OnRollbackHistoryItem(Files[0]);
    }
    return FReply::Handled();
}

// ─────────────────────────────────────────────────────────────────────────────
// Custom Rules UI
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildCustomRuleRow(const FCustomAssetRule& Rule, int32 Index)
{
    int32 RuleIndex = Index;

    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgRowAlt))
        .Padding(FMargin(12.f, 2.f))
        [
            SNew(SBox)
            .HeightOverride(28.f)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .FillWidth(1.5f)
                .VAlign(VAlign_Center)
                .Padding(FMargin(0.f, 0.f, 8.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Rule.ClassName.IsEmpty() ? TEXT("(empty)") : Rule.ClassName))
                    .ColorAndOpacity(FSlateColor(Rule.ClassName.IsEmpty() ? AOColors::TextDim : AOColors::TextPrimary))
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride(50.f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Rule.Prefix.IsEmpty() ? TEXT("\u2014") : Rule.Prefix))
                        .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrefix))
                    ]
                ]

                + SHorizontalBox::Slot()
                .FillWidth(2.f)
                .VAlign(VAlign_Center)
                .Padding(FMargin(8.f, 0.f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Rule.TargetPath))
                    .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                    .ContentPadding(FMargin(6.f, 2.f))
                    .OnClicked_Lambda([this, RuleIndex]() -> FReply
                    {
                        RemoveCustomRule(RuleIndex);
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("RemoveRule", "Remove"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                    ]
                ]
            ]
        ];
}

void SAssetOrganizerWidget::RebuildCustomRulesList()
{
    if (!CustomRulesContainer.IsValid() || !Settings)
        return;

    CustomRulesContainer->ClearChildren();

    for (int32 i = 0; i < Settings->CustomRules.Num(); ++i)
    {
        CustomRulesContainer->AddSlot()
            .AutoHeight()
            [
                BuildCustomRuleRow(Settings->CustomRules[i], i)
            ];
    }
}

FReply SAssetOrganizerWidget::OnAddCustomRuleClicked()
{
    FCustomAssetRule NewRule;
    NewRule.ClassName = TEXT("MyCustomAsset");
    NewRule.Prefix = TEXT("CA_");
    NewRule.TargetPath = TEXT("/Game/Custom");
    NewRule.CategoryName = TEXT("Custom");
    NewRule.bEnabled = true;

    AddCustomRule(NewRule);
    ShowNotification(LOCTEXT("CustomRuleAdded",
        "Custom rule added. Edit class name in Project Settings > Asset Organizer."));
    return FReply::Handled();
}

void SAssetOrganizerWidget::OnCustomRulesExternallyChanged()
{
    RebuildCustomRulesList();
    RefreshClassSelectorOptions();
}

void SAssetOrganizerWidget::RefreshClassSelectorOptions()
{
    AssetClassOptions.Empty();
    TSet<FString> UniqueNames;

    // 1. Built-in asset types (62 types)
    if (Settings)
    {
        for (const FAssetTypeInfo& Info : Settings->AssetTypeConfigs)
        {
            if (!Info.ClassName.IsNone())
            {
                UniqueNames.Add(Info.ClassName.ToString());
            }
        }
    }

    // 2. Registered asset type actions from AssetTools module
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
        IAssetTools& AssetTools = AssetToolsModule.Get();
        TArray<TWeakPtr<IAssetTypeActions>> Actions;
        AssetTools.GetAssetTypeActionsList(Actions);

        for (const TWeakPtr<IAssetTypeActions>& Action : Actions)
        {
            if (Action.IsValid())
            {
                UClass* SupportedClass = Action.Pin()->GetSupportedClass();
                if (SupportedClass)
                {
                    UniqueNames.Add(SupportedClass->GetName());
                }
            }
        }
    }

    // 3. Asset classes from AssetRegistry scan
    FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
    if (AssetRegistryModule)
    {
        IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
        TArray<FAssetData> AssetList;
        FARFilter Filter;
        Filter.PackagePaths.Add(FName("/Game"));
        Filter.bRecursivePaths = true;
        AssetRegistry.GetAssets(Filter, AssetList);

        for (const FAssetData& Asset : AssetList)
        {
            if (!Asset.AssetClass.IsNone())
            {
                UniqueNames.Add(Asset.AssetClass.ToString());
            }
        }
    }

    // 4. Existing custom rules
    if (Settings)
    {
        for (const FCustomAssetRule& Rule : Settings->CustomRules)
        {
            if (!Rule.ClassName.IsEmpty())
            {
                UniqueNames.Add(Rule.ClassName);
            }
        }
    }

    // Sort and populate
    TArray<FString> SortedNames = UniqueNames.Array();
    SortedNames.Sort();

    for (const FString& Name : SortedNames)
    {
        AssetClassOptions.Add(MakeShared<FString>(Name));
    }

    if (ClassSelectorComboBox.IsValid())
    {
        ClassSelectorComboBox->RefreshOptions();
    }
}

FReply SAssetOrganizerWidget::OnAddCustomRuleWithSelectionClicked()
{
    if (!SelectedClassOption.IsValid() || SelectedClassOption->IsEmpty())
    {
        ShowErrorNotification(LOCTEXT("NoClassSelected", "Please select an asset class from the dropdown first."));
        return FReply::Handled();
    }

    FString SelectedClass = *SelectedClassOption;

    // Check if already exists in custom rules
    if (Settings)
    {
        for (const FCustomAssetRule& Rule : Settings->CustomRules)
        {
            if (Rule.ClassName == SelectedClass)
            {
                ShowErrorNotification(FText::Format(
                    LOCTEXT("RuleAlreadyExists", "A custom rule for '{0}' already exists."),
                    FText::FromString(SelectedClass)));
                return FReply::Handled();
            }
        }
    }

    // Derive prefix and target path from built-in type if available
    FString Prefix = TEXT("CUSTOM_");
    FString TargetPath = TEXT("/Game/Custom");

    if (Settings)
    {
        for (const FAssetTypeInfo& Info : Settings->AssetTypeConfigs)
        {
            if (Info.ClassName.ToString() == SelectedClass)
            {
                Prefix = Info.Prefix;
                TargetPath = Info.TargetPath;
                break;
            }
        }
    }

    FCustomAssetRule NewRule;
    NewRule.ClassName = SelectedClass;
    NewRule.Prefix = Prefix;
    NewRule.TargetPath = TargetPath;
    NewRule.CategoryName = TEXT("Custom");
    NewRule.bEnabled = true;

    AddCustomRule(NewRule);
    ShowNotification(FText::Format(
        LOCTEXT("CustomRuleAddedFmt", "Custom rule for '{0}' added."),
        FText::FromString(SelectedClass)));
    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnRefreshCustomRulesClicked()
{
    RebuildCustomRulesList();
    RefreshClassSelectorOptions();
    ShowNotification(LOCTEXT("CustomRulesRefreshed", "Custom rules refreshed from Project Settings."));
    return FReply::Handled();
}

void SAssetOrganizerWidget::AddCustomRule(const FCustomAssetRule& Rule)
{
    if (Settings)
    {
        Settings->AddCustomRule(Rule);
        Settings->PostEditChange();
        RebuildCustomRulesList();
    }
}

void SAssetOrganizerWidget::RemoveCustomRule(int32 Index)
{
    if (Settings)
    {
        Settings->RemoveCustomRule(Index);
        Settings->PostEditChange();
        RebuildCustomRulesList();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AssetRegistry Delegate Wrappers
// ─────────────────────────────────────────────────────────────────────────────
void SAssetOrganizerWidget::OnAssetAddedCallback(const FAssetData& /*AssetData*/)
{
    // Use lock to prevent race condition between check and execution
    FScopeLock Lock(&RefreshLock);

    // Skip if refresh is already in progress (atomic load)
    if (bIsRefreshRunning.Load())
        return;

    RefreshAssetData();
}

void SAssetOrganizerWidget::OnAssetRemovedCallback(const FAssetData& /*AssetData*/)
{
    // Use lock to prevent race condition between check and execution
    FScopeLock Lock(&RefreshLock);

    // Skip if refresh is already in progress (atomic load)
    if (bIsRefreshRunning.Load())
        return;

    RefreshAssetData();
}

void SAssetOrganizerWidget::OnAssetRenamedCallback(const FAssetData& /*AssetData*/, const FString& /*OldPath*/)
{
    // Use lock to prevent race condition between check and execution
    FScopeLock Lock(&RefreshLock);

    // Skip if refresh is already in progress (atomic load)
    if (bIsRefreshRunning.Load())
        return;

    RefreshAssetData();
}

// ─────────────────────────────────────────────────────────────────────────────
// Whitelist Panel
// ─────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SAssetOrganizerWidget::BuildWhitelistPanel()
{
    return SNew(SBorder)
        .BorderBackgroundColor(FSlateColor(AOColors::BgPanel))
        .Padding(FMargin(12.f, 8.f))
        [
            SNew(SVerticalBox)

            // Title row
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 0.f, 0.f, 6.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("WhitelistTitle", "Whitelist Folders"))
                    .Font(FCoreStyle::Get().GetFontStyle("NormalText"))
                    .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .ButtonColorAndOpacity(FLinearColor(0.08f, 0.28f, 0.45f, 1.f))
                    .ContentPadding(FMargin(10.f, 3.f))
                    .OnClicked(this, &SAssetOrganizerWidget::OnAddWhitelistFolderClicked)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("AddWhitelistBtn", "+ Add Folder"))
                        .ColorAndOpacity(FSlateColor(AOColors::TextPrimary))
                    ]
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 0.f, 0.f, 6.f))
            [
                SNew(SSeparator)
            ]

            // Description
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0.f, 0.f, 0.f, 6.f))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WhitelistDesc", "Assets in whitelisted folders will never be moved during organizing."))
                .Font(FCoreStyle::Get().GetFontStyle("TinyText"))
                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                .AutoWrapText(true)
            ]

            // Folder list
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SScrollBox)
                .Orientation(Orient_Vertical)
                + SScrollBox::Slot()
                [
                    SAssignNew(WhitelistContainer, SVerticalBox)
                ]
            ]
        ];
}

FReply SAssetOrganizerWidget::OnAddWhitelistFolderClicked()
{
    if (!Settings)
        return FReply::Handled();

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
        return FReply::Handled();

    FString OutFolder;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    const bool bOpened = DesktopPlatform->OpenDirectoryDialog(
        ParentWindowHandle,
        TEXT("Select Whitelist Folder"),
        FPaths::ProjectContentDir(),
        OutFolder
    );

    if (!bOpened || OutFolder.IsEmpty())
        return FReply::Handled();

    // Convert absolute disk path to /Game/ relative path
    FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    FPaths::NormalizeDirectoryName(ContentDir);
    FPaths::NormalizeDirectoryName(OutFolder);
    // Ensure both paths use the same slash style for reliable comparison on Windows
    ContentDir = ContentDir.Replace(TEXT("\\"), TEXT("/"));
    OutFolder  = OutFolder.Replace(TEXT("\\"), TEXT("/"));

    FString GamePath;
    if (FPaths::IsUnderDirectory(OutFolder, ContentDir))
    {
        FString Relative = OutFolder.Mid(ContentDir.Len());
        Relative.RemoveFromStart(TEXT("/"));
        GamePath = TEXT("/Game/") + Relative;
    }
    else
    {
        // Fallback: use as-is if outside Content dir
        GamePath = OutFolder;
    }

    // Avoid duplicates
    if (!Settings->WhitelistedFolders.Contains(GamePath))
    {
        Settings->WhitelistedFolders.Add(GamePath);
        Settings->SaveConfig();
        WhitelistFolders = Settings->WhitelistedFolders;

        // Rebuild list UI
        if (WhitelistContainer.IsValid())
        {
            WhitelistContainer->ClearChildren();
            for (int32 i = 0; i < WhitelistFolders.Num(); ++i)
            {
                const FString FolderPath = WhitelistFolders[i];
                const int32 Idx = i;
                WhitelistContainer->AddSlot()
                    .AutoHeight()
                    .Padding(FMargin(0.f, 1.f))
                    [
                        SNew(SBorder)
                        .BorderBackgroundColor(FSlateColor(AOColors::BgRow))
                        .Padding(FMargin(8.f, 3.f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(FolderPath))
                                .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                                .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(SButton)
                                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                                .ContentPadding(FMargin(6.f, 2.f))
                                .OnClicked_Lambda([this, Idx]() -> FReply
                                {
                                    return OnRemoveWhitelistFolder(Idx);
                                })
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("RemoveWhitelist", "Remove"))
                                    .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                                ]
                            ]
                        ]
                    ];
            }
        }

        ShowNotification(FText::Format(
            LOCTEXT("WhitelistAdded", "Added to whitelist: {0}"),
            FText::FromString(GamePath)));
    }
    else
    {
        ShowNotification(LOCTEXT("WhitelistDuplicate", "Folder already in whitelist."), false);
    }

    return FReply::Handled();
}

FReply SAssetOrganizerWidget::OnRemoveWhitelistFolder(int32 Index)
{
    if (!Settings || !Settings->WhitelistedFolders.IsValidIndex(Index))
        return FReply::Handled();

    FString Removed = Settings->WhitelistedFolders[Index];
    Settings->WhitelistedFolders.RemoveAt(Index);
    Settings->SaveConfig();
    WhitelistFolders = Settings->WhitelistedFolders;

    // Rebuild list UI
    if (WhitelistContainer.IsValid())
    {
        WhitelistContainer->ClearChildren();
        for (int32 i = 0; i < WhitelistFolders.Num(); ++i)
        {
            const FString FolderPath = WhitelistFolders[i];
            const int32 Idx = i;
            WhitelistContainer->AddSlot()
                .AutoHeight()
                .Padding(FMargin(0.f, 1.f))
                [
                    SNew(SBorder)
                    .BorderBackgroundColor(FSlateColor(AOColors::BgRow))
                    .Padding(FMargin(8.f, 3.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(FolderPath))
                            .Font(FCoreStyle::Get().GetFontStyle("Mono"))
                            .ColorAndOpacity(FSlateColor(AOColors::TextSecondary))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SButton)
                            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                            .ContentPadding(FMargin(6.f, 2.f))
                            .OnClicked_Lambda([this, Idx]() -> FReply
                            {
                                return OnRemoveWhitelistFolder(Idx);
                            })
                            [
                                SNew(STextBlock)
                                .Text(LOCTEXT("RemoveWhitelist", "Remove"))
                                .ColorAndOpacity(FSlateColor(AOColors::TextDim))
                            ]
                        ]
                    ]
                ];
        }
    }

    ShowNotification(FText::Format(
        LOCTEXT("WhitelistRemoved", "Removed from whitelist: {0}"),
        FText::FromString(Removed)));

    return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
