// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetOrganizerTypes.h"

class UAssetOrganizerSettings;

/**
 * Asset Organizer Executor - Core logic with 62+ type support
 * Features:
 * - Lazy folder creation (create only needed folders)
 * - Async execution with cancel support
 * - History tracking for undo
 * - Enhanced reference fixing
 */
class ASSETORGANIZER_API FAssetOrganizerExecutor
{
public:
    DECLARE_DELEGATE_TwoParams(FOnProgress, float /*Percent*/, const FText& /*Message*/);
    DECLARE_DELEGATE_OneParam(FOnLog, const FString& /*Message*/);
    DECLARE_DELEGATE(FOnCancelled);

    /** Determine asset type from asset data (62+ types) */
    static EAssetOrganizeType DetermineAssetType(const FAssetData& AssetData, const TArray<FCustomAssetRule>* CustomRules = nullptr);

    /** Get class name from organize type */
    static FName GetClassNameFromType(EAssetOrganizeType Type);

    /** Get display info for organize type */
    static FAssetTypeInfo GetTypeInfo(EAssetOrganizeType Type);

    /** Get all supported asset type infos (62 types) */
    static TArray<FAssetTypeInfo> GetAllTypeInfos();

    /** Get target folder for asset type with config applied */
    static FString GetTargetFolder(EAssetOrganizeType Type, const FOrganizeConfig& Config);

    /** Execute organization (synchronous version) */
    static FOrganizeResult OrganizeAssets(
        const FOrganizeConfig& Config,
        const FOnProgress& ProgressCallback = FOnProgress(),
        const FOnLog& LogCallback = FOnLog()
    );

    /** Execute organization (async version) */
    static TSharedPtr<FAsyncTask<class FOrganizeAssetsTask>> OrganizeAssetsAsync(
        const FOrganizeConfig& Config,
        const FOnProgress& ProgressCallback = FOnProgress(),
        const FOnLog& LogCallback = FOnLog(),
        const FOnCancelled& CancelledCallback = FOnCancelled()
    );

    /** Cancel ongoing async operation */
    static void CancelAsyncOperation();

    /** Check if async operation is running */
    static bool IsAsyncOperationRunning();

    /** Get the current async task (may be null or done) */
    static TSharedPtr<FAsyncTask<class FOrganizeAssetsTask>> GetCurrentAsyncTask();

    /** Get all assets in path */
    static TArray<FAssetData> GetAssetsInPath(const FString& Path, bool bRecursive = true, const TArray<FString>& ExcludedFolders = TArray<FString>());

    /** Group assets by type (62+ types support) */
    static TMap<EAssetOrganizeType, TArray<FAssetData>> GroupAssetsByType(
        const TArray<FAssetData>& Assets,
        const TArray<FCustomAssetRule>* CustomRules = nullptr
    );

    /** Sort assets by dependency (move dependencies first) */
    static TArray<FAssetData> SortByDependency(const TArray<FAssetData>& Assets);

    /** Move single asset to new path with full conflict handling */
    static bool MoveAsset(
        const FAssetData& Asset,
        const FString& NewPath,
        TArray<FString>& OutErrors,
        TArray<FString>& OutWarnings,
        FMoveRecord* OutRecord = nullptr
    );

    /** Fix up all ObjectRedirectors */
    static void FixUpAllReferences(const FString& RootPath, int32& OutFixedCount, const FOnLog& LogCallback = FOnLog());

    /** Enhanced reference fixing with detailed logging */
    static int32 FixBrokenReferences(
        const FString& RootPath,
        int32& OutFixedByRedirector,
        int32& OutTrulyMissing,
        TArray<FString>& OutDetailedLog
    );

    /** Check if an asset has broken references */
    static bool CheckAssetReferences(const FAssetData& Asset, TArray<FString>& OutBrokenRefs);

    /** Clean empty folders with safety checks */
    static int32 CleanEmptyFolders(const FString& RootPath, TArray<FString>* OutRemovedFolders = nullptr);

    /** Create folder if not exists */
    static bool EnsureFolderExists(const FString& FolderPath);

    /** ========== Lazy Folder Creation (Phase 2) ========== */

    /** Collect folders that actually need to be created based on assets */
    static TArray<FString> CollectNeededFolders(
        const TArray<FAssetData>& Assets,
        const FOrganizeConfig& Config,
        const TArray<FCustomAssetRule>* CustomRules = nullptr
    );

    /** Create only needed folders (lazy creation) */
    static int32 CreateNeededFolders(
        const TArray<FString>& Folders,
        TArray<FString>& OutCreated,
        TArray<FString>& OutErrors
    );

    /** ========== History / Undo (Phase 3) ========== */

    /** Save operation history to JSON */
    static FString SaveHistory(const FOrganizeHistoryEntry& Entry);

    /** Load history from JSON file */
    static bool LoadHistory(const FString& FilePath, FOrganizeHistoryEntry& OutEntry);

    /** Get list of available history files */
    static TArray<FString> GetHistoryFiles(int32 MaxCount = 10);

    /** Rollback an operation from history */
    static FOrganizeResult RollbackOrganize(const FString& HistoryFilePath, const FOnProgress& ProgressCallback = FOnProgress(), const FOnLog& LogCallback = FOnLog());

    /** Delete history file */
    static bool DeleteHistory(const FString& FilePath);

    /** Delete all history files */
    static int32 ClearAllHistory(TArray<FString>* OutDeletedFiles = nullptr);

    /** ========== Level Reference Snapshot (Critical for Rollback) ========== */

    /** Save snapshot of level references before organizing
     *  This is critical for proper rollback - it records which assets each level references
     *  before they get moved, so we can restore references after rollback.
     */
    static TArray<FLevelAssetReference> SaveLevelReferencesSnapshot(
        const TArray<FAssetData>& AssetsToMove,
        const FOnLog& LogCallback = FOnLog()
    );

    /** Restore level references after rollback
     *  Fixes up references in levels to point back to original asset locations.
     *  This ensures models don't disappear after rollback.
     */
    static void RestoreLevelReferencesAfterRollback(
        const TArray<FLevelAssetReference>& LevelSnapshots,
        const FOnLog& LogCallback = FOnLog()
    );

    /** ========== Preview (Phase 2) ========== */

    /** Generate detailed preview of what will happen */
    static TArray<FMovePreviewItem> GeneratePreview(
        const TArray<FAssetData>& Assets,
        const FOrganizeConfig& Config,
        const TArray<FCustomAssetRule>* CustomRules = nullptr
    );

    /** ========== Utility ========== */

    /** Get history directory path */
    static FString GetHistoryDirectory();

    /** Check if asset is in excluded folder */
    static bool IsInExcludedFolder(const FAssetData& Asset, const TArray<FString>& ExcludedFolders);

    /** Get priority for asset type (lower = moved first) */
    static int32 GetTypePriority(EAssetOrganizeType Type);

private:
    static TAtomic<bool> bCancelRequested;
    static FCriticalSection AsyncLock;
    static TSharedPtr<FAsyncTask<class FOrganizeAssetsTask>> CurrentAsyncTask;

    /** Check if asset A depends on asset B */
    static bool DoesAssetDependOn(const FAssetData& AssetA, const FAssetData& AssetB);

    /** Internal move with transaction support */
    static bool MoveAssetInternal(
        const FAssetData& Asset,
        const FString& NewPath,
        const FString& TargetAssetName,
        TArray<FString>& OutErrors
    );

    /** Resolve naming conflict with suffix or GUID */
    static FString ResolveNamingConflict(
        const FString& BaseName,
        const FString& TargetPath,
        int32& OutSuffixIndex,
        bool& bUsedGuid
    );

    /** Check if there's an existing asset at path */
    static bool DoesAssetExistAtPath(const FString& ObjectPath, FAssetData& OutExisting);

    /** Save all dirty packages in root path */
    static void SaveDirtyPackages(const FString& RootPath);
};

/** Async task for background execution */
class FOrganizeAssetsTask : public FNonAbandonableTask
{
public:
    FOrganizeAssetsTask(
        const FOrganizeConfig& InConfig,
        const FAssetOrganizerExecutor::FOnProgress& InProgressCallback,
        const FAssetOrganizerExecutor::FOnLog& InLogCallback
    );

    void DoWork();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FOrganizeAssetsTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    FOrganizeResult GetResult() const { return Result; }

private:
    FOrganizeConfig Config;
    FAssetOrganizerExecutor::FOnProgress ProgressCallback;
    FAssetOrganizerExecutor::FOnLog LogCallback;
    FOrganizeResult Result;
};
