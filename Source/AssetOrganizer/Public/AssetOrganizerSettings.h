// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AssetOrganizerTypes.h"
#include "AssetOrganizerSettings.generated.h"

/**
 * Plugin settings - saved to project config
 * Supports 62+ asset types with full customization
 */
UCLASS(config = AssetOrganizer, defaultconfig)
class UAssetOrganizerSettings : public UObject
{
    GENERATED_BODY()

public:
    UAssetOrganizerSettings();

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
