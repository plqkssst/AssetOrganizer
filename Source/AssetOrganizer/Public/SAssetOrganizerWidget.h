// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AssetOrganizerTypes.h"

class UAssetOrganizerSettings;
class SMultiLineEditableTextBox;
class SBox;
class SVerticalBox;
class SCheckBox;
class SEditableTextBox;

/** Per-type asset count data with pending/total split */
struct FAssetTypeRow
{
    EAssetOrganizeType Type;
    FName              ClassName;       // Asset class name
    FString            TypeName;        // Display name
    FString            Prefix;          // Naming prefix
    FString            TargetPath;      // Target folder path
    int32              Count;           // Total assets of this type
    int32              PendingCount;    // Assets not in target folder (need organizing)
    bool               bActive;         // Whether this type is enabled
    bool               bEnabled;        // Whether this type is available in config
    int32              Priority;        // Move priority

    FAssetTypeRow(EAssetOrganizeType InType, const FName& InClass, const FString& InName,
                  const FString& InPrefix, const FString& InPath, bool bInActive = true)
        : Type(InType)
        , ClassName(InClass)
        , TypeName(InName)
        , Prefix(InPrefix)
        , TargetPath(InPath)
        , Count(0)
        , PendingCount(0)
        , bActive(bInActive)
        , bEnabled(true)
        , Priority(100)
    {}
};

/** Category grouping of asset rows */
struct FAssetCategory
{
    FString                Name;      // Display name
    TArray<FAssetTypeRow>  Rows;
    bool                   bExpanded;
    bool                   bIsUncategorized;
    int32                  TotalCount;
    int32                  TotalPending;

    FAssetCategory(const FString& InName, bool bInUncategorized = false)
        : Name(InName)
        , bExpanded(true)
        , bIsUncategorized(bInUncategorized)
        , TotalCount(0)
        , TotalPending(0)
    {}
};

/** History list item */
struct FHistoryListItem
{
    FString Id;
    FString Timestamp;
    FString Description;
    int32   MovedAssets;
    bool    bCanRollback;
    FString FilePath;
};

/**
 * Asset Organizer Widget v2.0
 * - 62+ asset type support
 * - Lazy folder creation
 * - Async execution with cancel
 * - History/Undo
 * - Detailed preview
 */
class SAssetOrganizerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAssetOrganizerWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SAssetOrganizerWidget();

private:
    // ── Build helpers ─────────────────────────────────────────────────────────
    TSharedRef<SWidget> BuildToolbar();
    TSharedRef<SWidget> BuildSettingsPanel();
    TSharedRef<SWidget> BuildTableHeader();
    TSharedRef<SWidget> BuildScrollArea();
    TSharedRef<SWidget> BuildStatusBar();
    TSharedRef<SWidget> BuildProgressOverlay();
    TSharedRef<SWidget> BuildLogPanel();
    TSharedRef<SWidget> BuildHistoryPanel();
    TSharedRef<SWidget> BuildPreviewPanel();
    TSharedRef<SWidget> BuildWhitelistPanel();

    /** Rebuild the entire category/row list inside the scroll box */
    void                RebuildCategoryList();

    TSharedRef<SWidget> BuildCategoryHeader(FAssetCategory& Cat, int32 CategoryIndex);
    TSharedRef<SWidget> BuildAssetRow(int32 CategoryIndex, int32 RowIndex);
    TSharedRef<SWidget> BuildCustomRuleRow(const FCustomAssetRule& Rule, int32 Index);

    // ── Button handlers ───────────────────────────────────────────────────────
    FReply OnOrganizeClicked();
    FReply OnPreviewClicked();
    FReply OnRefreshClicked();
    FReply OnCancelClicked();
    FReply OnShowHistoryClicked();
    FReply OnRollbackClicked();
    FReply OnAddCustomRuleClicked();
    FReply OnAddWhitelistFolderClicked();
    FReply OnRemoveWhitelistFolder(int32 Index);

    /** Jump Content Browser to folder and select all matching assets */
    void FocusAndSelectAssets(const FString& FolderPath, EAssetOrganizeType Type);

    /** Open content browser to folder */
    void SyncBrowserToFolder(const FString& FolderPath);

    // ── Data ──────────────────────────────────────────────────────────────────
    void RefreshAssetData();
    int32 GetTotalAssetCount() const;
    int32 GetTotalPendingCount() const;

    // ── Settings ──────────────────────────────────────────────────────────────
    void LoadSettings();
    void SaveSettings();
    void ApplyRootFolderChange();
    void OnTargetPathEdited(FAssetTypeRow* Row, const FText& NewText);

    /** Get effective target path with root folder prefix applied */
    FString GetEffectiveTargetPath(const FString& BasePath) const;

    // ── Async execution ───────────────────────────────────────────────────────
    void StartAsyncOrganize(bool bDryRun);
    void OnAsyncProgress(float Percent, const FText& Message);
    void OnAsyncLog(const FString& Message);
    void OnAsyncComplete();
    void UpdateProgressUI(float Percent, const FText& Message);

    // ── History ───────────────────────────────────────────────────────────────
    void RefreshHistoryList();
    void RebuildHistoryListUI();
    void OnRollbackHistoryItem(const FString& HistoryFile);
    void OnDeleteHistoryItem(const FString& HistoryFile);
    void OnClearAllHistoryClicked();

    // ── Preview ───────────────────────────────────────────────────────────────
    void ShowPreviewDialog(const TArray<FMovePreviewItem>& PreviewItems);
    void RebuildPreviewListUI();
    void ClosePreviewPanel();

    // ── Notification ──────────────────────────────────────────────────────────
    void ShowNotification(const FText& Message, bool bSuccess = true);
    void ShowErrorNotification(const FText& Message);

    // ── Custom Rules ──────────────────────────────────────────────────────────
    void AddCustomRule(const FCustomAssetRule& Rule);
    void RemoveCustomRule(int32 Index);
    void RebuildCustomRulesList();
    void OnCustomRulesExternallyChanged();
    void RefreshClassSelectorOptions();
    FReply OnAddCustomRuleWithSelectionClicked();
    FReply OnRefreshCustomRulesClicked();

    // ── Log ───────────────────────────────────────────────────────────────────
    void AppendLog(const FString& Message);
    void ClearLog();
    void ToggleLogVisibility();
    void ToggleSettingsVisibility();

    // ── Data ──────────────────────────────────────────────────────────────────
    TArray<FAssetCategory> Categories;
    TArray<FCustomAssetRule> CustomRules;
    TArray<FString> WhitelistFolders;
    TArray<FHistoryListItem> HistoryItems;

    /** Settings reference */
    UAssetOrganizerSettings* Settings;

    /** Scroll box that holds all category rows */
    TSharedPtr<SVerticalBox> CategoryContainer;

    /** Custom rules container */
    TSharedPtr<SVerticalBox> CustomRulesContainer;

    /** Asset class selector for adding custom rules */
    TSharedPtr<class SComboBox<TSharedPtr<FString>>> ClassSelectorComboBox;
    TArray<TSharedPtr<FString>> AssetClassOptions;
    TSharedPtr<FString> SelectedClassOption;

    /** Log text box */
    TSharedPtr<SMultiLineEditableTextBox> LogTextBox;

    /** Log visibility toggle */
    bool bIsLogVisible = false;

    /** Current log content */
    FString LogContent;

    /** Status bar total text */
    TAttribute<FText> TotalCountText;
    TAttribute<FText> PendingCountText;

    /** Progress overlay widgets */
    TSharedPtr<SBox> ProgressOverlay;
    TSharedPtr<class SProgressBar> ProgressBar;
    TSharedPtr<STextBlock> ProgressText;
    TSharedPtr<SButton> CancelButton;

    /** History panel visibility */
    bool bIsHistoryVisible = false;
    TSharedPtr<SBox> HistoryPanel;
    TSharedPtr<SVerticalBox> HistoryListContainer;

    /** Preview panel */
    bool bIsPreviewVisible = false;
    TSharedPtr<SBox> PreviewPanel;

    /** Settings panel */
    bool bIsSettingsVisible = false;
    TSharedPtr<SBox> SettingsPanel;
    TSharedPtr<SBox> WhitelistPanel;
    TSharedPtr<SVerticalBox> WhitelistContainer;
    bool bIsWhitelistVisible = false;
    TSharedPtr<SVerticalBox> PreviewListContainer;
    TArray<FMovePreviewItem> CurrentPreviewItems;

    /** Async operation state */
    bool bIsAsyncRunning = false;
    TAtomic<bool> bIsRefreshRunning{ false };
    FCriticalSection RefreshLock;
    FOrganizeResult LastResult;

    /** Root folder check box */
    TSharedPtr<class SCheckBox> RootFolderCheckBox;
    TSharedPtr<class SEditableTextBox> RootFolderNameBox;

    // Editor event handles
    FDelegateHandle OnAssetAddedHandle;
    FDelegateHandle OnAssetRemovedHandle;
    FDelegateHandle OnAssetRenamedHandle;

    // Async task polling ticker
    FDelegateHandle AsyncTickerHandle;

    // AssetRegistry delegate wrappers (correct signatures)
    void OnAssetAddedCallback(const FAssetData& AssetData);
    void OnAssetRemovedCallback(const FAssetData& AssetData);
    void OnAssetRenamedCallback(const FAssetData& AssetData, const FString& OldPath);

    // Ticker callback to poll async task completion
    bool OnAsyncTick(float DeltaTime);
};
