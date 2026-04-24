// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DeveloperSettings.h"
#include "AssetOrganizerTypes.h"
#include "AssetOrganizerSettings.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnCustomRulesChanged);

/**
 * Plugin settings - saved to project config
 * Supports 62+ asset types with full customization
 */
UCLASS(config = AssetOrganizer, defaultconfig, meta = (DisplayName = "Asset Organizer"))
class UAssetOrganizerSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UAssetOrganizerSettings();

    /** Static delegate broadcast when CustomRules array changes */
    static FOnCustomRulesChanged OnCustomRulesChanged;

    virtual FName GetCategoryName() const override;
    virtual FName GetSectionName() const override;

    /** Root path to scan for assets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "General")
    FString RootPath;

    /** Create folders if they don't exist */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "General")
    bool bCreateFolders;

    /** Fix up references after moving assets */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "General")
    bool bFixReferences;

    /** Remove empty folders after organization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "General")
    bool bCleanEmptyFolders;

    /** Folder creation strategy */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "General")
    EFolderCreationStrategy FolderStrategy;

    /** Root folder wrapper configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Root Folder")
    FRootFolderConfig RootFolderConfig;

    /** Built-in type configurations (62 types) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Asset Types")
    TArray<FAssetTypeInfo> AssetTypeConfigs;

    /** Custom asset rules (user-defined types) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Custom Rules")
    TArray<FCustomAssetRule> CustomRules;

    /** Folders to skip during organization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Advanced")
    TArray<FString> ExcludedFolders;

    /** Folders whose assets will NEVER be moved (whitelist).
     *  These are also written into FOrganizeConfig.WhitelistedFolders at organize time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Whitelist",
              meta = (ToolTip = "Assets in these folders will never be moved during organizing."))
    TArray<FString> WhitelistedFolders;

    /** Auto-run reference verification pass after organize (checks all moved assets). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Verification")
    bool bRunVerificationAfterOrganize;

    /** If verification finds broken refs, attempt auto-fix via redirectors. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "Verification")
    bool bAutoFixBrokenReferences;

    /** Maximum number of history entries to keep */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "History",
              meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxHistoryEntries;

    /** Enable operation history logging */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "History")
    bool bEnableHistory;

    /** Auto-save history after each operation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, config, Category = "History")
    bool bAutoSaveHistory;

    /** Reset to default settings */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void ResetToDefault();

    /** Initialize default asset type configurations (62 types) */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void InitializeDefaultAssetTypes();

    /** Get type info by class name */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    FAssetTypeInfo GetTypeInfo(const FName& AssetClass) const;

    /** Update type info */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void UpdateTypeInfo(const FAssetTypeInfo& TypeInfo);

    /** Enable/disable specific asset type */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void SetTypeEnabled(EAssetOrganizeType Type, bool bEnabled);

    /** Update target path for a type */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void SetTypeTargetPath(EAssetOrganizeType Type, const FString& NewPath);

    /** Add custom rule */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void AddCustomRule(const FCustomAssetRule& Rule);

    /** Remove custom rule by index */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void RemoveCustomRule(int32 Index);

    /** Save settings to config */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    void SaveSettings();

    /** Check if a folder path is whitelisted (prefix-match). */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    bool IsFolderWhitelisted(const FString& FolderPath) const;

    /** One-time migration: copy old ExcludedFolders INI key into WhitelistedFolders. */
    void MigrateFromLegacyExcludedFolders();

    /** Get all enabled types that exist in project */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    TArray<FAssetTypeInfo> GetActiveTypes() const;

    /** Get folder paths that will be created (lazy evaluation) */
    UFUNCTION(BlueprintCallable, Category = "Asset Organizer")
    TArray<FString> GetNeededFolders(const TArray<FAssetData>& AssetsToOrganize) const;

    //~ Begin UObject Interface
    virtual void PostInitProperties() override;
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    //~ End UObject Interface
};
