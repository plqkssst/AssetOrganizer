// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetOrganizerTypes.generated.h"

/**
 * AssetOrganizer Plugin - Type Definitions
 * Based on Allar UE5 Style Guide, AutoPrefixer plugin, and Epic official documentation
 * Expanded from 18 types to 62+ types for comprehensive asset organization
 */

/** Extended asset type enumeration (62+ types) */
UENUM(BlueprintType)
enum class EAssetOrganizeType : uint8
{
    // ========== Meshes (5 types) ==========
    StaticMesh,           // SM_ - Static mesh geometry
    SkeletalMesh,         // SK_ - Skeletal mesh for characters
    Skeleton,             // SKEL_ - Skeleton asset
    PhysicsAsset,         // PHYS_ - Physics asset (collision bodies)
    DestructibleMesh,     // DM_ - Apex destructible mesh

    // ========== Materials (6 types) ==========
    Material,                   // M_ - Base material
    MaterialInstanceConstant,   // MI_ - Material instance
    MaterialInstanceDynamic,    // MID_ - Dynamic material instance (runtime)
    MaterialFunction,           // MF_ - Material function
    MaterialParameterCollection,// MPC_ - Material parameter collection
    SubsurfaceProfile,          // SP_ - Subsurface scattering profile
    PhysicalMaterial,           // PM_ - Physical material (friction, etc.)

    // ========== Textures (6 types) ==========
    Texture2D,              // T_ - Standard 2D texture
    TextureCube,            // TC_ - Cubemap texture
    TextureRenderTarget2D,  // RT_ - Render target texture
    TextureRenderTargetCube,// RTC_ - Cubemap render target
    MediaTexture,           // MT_ - Media player texture
    TextureLightProfile,    // TLP_ - IES light profile texture
    VolumeTexture,          // VT_ - 3D volume texture

    // ========== Animation (11 types) ==========
    AnimSequence,           // A_ - Animation sequence
    AnimMontage,            // AM_ - Animation montage
    AnimBlueprint,          // ABP_ - Animation blueprint
    AnimComposite,          // AC_ - Animation composite
    BlendSpace,             // BS_ - 2D blend space
    BlendSpace1D,           // BS_ - 1D blend space
    AimOffsetBlendSpace,    // AO_ - Aim offset (2D)
    AimOffsetBlendSpace1D,  // AO_ - Aim offset (1D)
    PoseAsset,              // Pose_ - Pose asset
    LevelSequence,          // LS_ - Cinematic sequence

    // ========== Blueprints (4 types) ==========
    Blueprint,              // BP_ - Regular blueprint
    WidgetBlueprint,        // WBP_ - UMG widget blueprint
    UserDefinedEnum,        // E_ - User defined enumeration
    UserDefinedStruct,      // S_ - User defined structure

    // ========== AI (3 types) ==========
    BehaviorTree,           // BT_ - Behavior tree
    BlackboardData,         // BB_ - Blackboard data
    EnvQuery,               // EQS_ - Environment query

    // ========== Data (8 types) ==========
    DataTable,              // DT_ - Data table
    DataAsset,              // DA_ - Data asset
    StringTable,            // ST_ - String table (localization)
    CurveFloat,             // Curve_ - Float curve
    CurveVector,            // Curve_ - Vector curve
    CurveLinearColor,       // Curve_ - Color curve
    CurveTable,             // CT_ - Curve table
    CompositeCurveTable,    // CCT_ - Composite curve table

    // ========== Audio (8 types) ==========
    SoundWave,              // SW_ - Sound wave (raw audio)
    SoundCue,               // SC_ - Sound cue (audio event)
    SoundAttenuation,       // ATT_ - Sound attenuation settings
    SoundClass,             // (no prefix) - Sound class
    SoundMix,               // Mix_ - Sound mix (audio snapshot)
    SoundConcurrency,       // CON_ - Sound concurrency settings
    ReverbEffect,           // Reverb_ - Reverb effect preset

    // ========== VFX (4 types) ==========
    ParticleSystem,         // PS_ - Cascade particle system
    NiagaraSystem,          // NS_ - Niagara system
    NiagaraEmitter,         // NE_ - Niagara emitter
    NiagaraParameterCollection, // NPC_ - Niagara parameter collection

    // ========== Environment (3 types) ==========
    FoliageType,            // FT_ - Foliage type
    LandscapeGrassType,     // LG_ - Landscape grass type
    LandscapeLayerInfoObject, // LL_ - Landscape layer info

    // ========== Media (2 types) ==========
    MediaPlayer,            // MP_ - Media player
    FileMediaSource,        // FMS_ - File media source

    // ========== Levels (1 type) ==========
    World,                  // (no prefix, suffixes: _P, _Audio, _Lighting, _Geo)

    // ========== Special ==========
    Other,                  // * - Uncategorized assets
    Custom                  // User-defined custom types
};

/** Asset type information structure */
USTRUCT(BlueprintType)
struct FAssetTypeInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    EAssetOrganizeType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    FName ClassName;        // AssetData.AssetClass (e.g., "StaticMesh")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    FString Prefix;         // Naming prefix (e.g., "SM_")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    FString TargetPath;     // Default target folder path

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    FName Category;         // UI category name

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    FText DisplayName;      // Localized display name

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    bool bEnabled = true;   // Whether this type is enabled for organization

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Type")
    int32 Priority = 0;     // Move priority (lower = moved first)

    FAssetTypeInfo()
        : Type(EAssetOrganizeType::Other)
        , ClassName(NAME_None)
        , Prefix(TEXT(""))
        , TargetPath(TEXT("/Game/Other"))
        , Category(TEXT("Uncategorized"))
        , DisplayName(FText::GetEmpty())
        , bEnabled(true)
        , Priority(100)
    {}

    FAssetTypeInfo(EAssetOrganizeType InType, const FName& InClass, const FString& InPrefix,
                   const FString& InPath, const FName& InCategory, const FText& InDisplayName,
                   int32 InPriority = 100)
        : Type(InType)
        , ClassName(InClass)
        , Prefix(InPrefix)
        , TargetPath(InPath)
        , Category(InCategory)
        , DisplayName(InDisplayName)
        , bEnabled(true)
        , Priority(InPriority)
    {}
};

/** Custom asset rule for user-defined types */
USTRUCT(BlueprintType)
struct FCustomAssetRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Rule")
    FString ClassName;      // Asset class name (e.g., "MyCustomDataAsset")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Rule")
    FString Prefix;         // Naming prefix (e.g., "CDA_")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Rule")
    FString TargetPath;     // Target folder path

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Rule")
    FString CategoryName;   // UI category (e.g., "Custom")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Rule")
    bool bEnabled = true;

    FCustomAssetRule()
        : ClassName(TEXT(""))
        , Prefix(TEXT(""))
        , TargetPath(TEXT("/Game/Custom"))
        , CategoryName(TEXT("Custom"))
        , bEnabled(true)
    {}
};

/** Move record for history/undo */
USTRUCT()
struct FMoveRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString AssetPath;      // Full path to asset (e.g., "/Game/OldFolder/MyAsset.MyAsset")

    UPROPERTY()
    FString AssetName;      // Asset name

    UPROPERTY()
    FString OldPath;        // Original folder path

    UPROPERTY()
    FString NewPath;        // New folder path

    UPROPERTY()
    FName AssetClass;       // Asset class

    UPROPERTY()
    FString Timestamp;      // ISO 8601 timestamp

    FMoveRecord()
    {}

    FMoveRecord(const FString& InPath, const FString& InName, const FString& InOld,
                const FString& InNew, const FName& InClass)
        : AssetPath(InPath)
        , AssetName(InName)
        , OldPath(InOld)
        , NewPath(InNew)
        , AssetClass(InClass)
        , Timestamp(FDateTime::Now().ToIso8601())
    {}
};

/** Level reference info - stores which assets a level references */
USTRUCT()
struct FLevelAssetReference
{
    GENERATED_BODY()

    UPROPERTY()
    FString LevelPath;              // Level package path (e.g., "/Game/Maps/MyLevel")

    UPROPERTY()
    TArray<FString> ReferencedAssets;   // Asset paths referenced by this level

    FLevelAssetReference()
    {}

    FLevelAssetReference(const FString& InLevelPath)
        : LevelPath(InLevelPath)
    {}
};

/** History entry for undo functionality */
USTRUCT()
struct FOrganizeHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;                     // Unique identifier (GUID)

    UPROPERTY()
    FString Timestamp;              // ISO 8601 timestamp

    UPROPERTY()
    FString Description;            // User description

    UPROPERTY()
    int32 TotalAssets = 0;

    UPROPERTY()
    int32 MovedAssets = 0;

    UPROPERTY()
    int32 FixedReferences = 0;

    UPROPERTY()
    TArray<FMoveRecord> Records;    // All move operations

    UPROPERTY()
    bool bCanRollback = true;       // Whether this entry can be rolled back

    UPROPERTY()
    TArray<FLevelAssetReference> LevelReferences;   // Snapshot of level references before organize

    FOrganizeHistoryEntry()
    {
        Id = FGuid::NewGuid().ToString();
        Timestamp = FDateTime::Now().ToIso8601();
    }
};

/** Root folder wrapper configuration */
USTRUCT(BlueprintType)
struct FRootFolderConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Folder")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Folder",
              meta = (EditCondition = "bEnabled"))
    FString FolderName = TEXT("Organized");

    FRootFolderConfig()
        : bEnabled(false)
        , FolderName(TEXT("Organized"))
    {}

    /** Get effective target path with root folder prefix */
    FString GetEffectivePath(const FString& OriginalPath) const
    {
        if (!bEnabled || FolderName.IsEmpty())
        {
            return OriginalPath;
        }

        // OriginalPath is like "/Game/Materials"
        // Result should be "/Game/Organized/Materials"
        if (OriginalPath.StartsWith(TEXT("/Game/")))
        {
            FString SubPath = OriginalPath.Mid(6); // Remove "/Game/"
            return FString::Printf(TEXT("/Game/%s/%s"), *FolderName, *SubPath);
        }
        return OriginalPath;
    }
};

/** Folder creation strategy */
UENUM(BlueprintType)
enum class EFolderCreationStrategy : uint8
{
    CreateAll,              // Create all target folders upfront
    LazyCreate,             // Create only folders that will be used
    OnDemand                // Create folder just before first asset move
};

/** Post-organize report — moved asset record */
USTRUCT()
struct FMovedAssetRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString OriginalPath;      // /Game/OldFolder/MyAsset

    UPROPERTY()
    FString DestinationPath;   // /Game/Blueprints/Characters/MyActor

    UPROPERTY()
    FName AssetType;           // e.g. "Blueprint"

    FMovedAssetRecord() {}
    FMovedAssetRecord(const FString& InOrig, const FString& InDest, const FName& InType)
        : OriginalPath(InOrig), DestinationPath(InDest), AssetType(InType) {}
};

/** Post-organize report — broken reference record */
USTRUCT()
struct FBrokenReferenceRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString AssetPath;          // asset that has the broken ref

    UPROPERTY()
    FString BrokenDependency;   // the missing/broken target

    UPROPERTY()
    bool bAutoFixed = false;    // whether redirector fixup resolved it

    FBrokenReferenceRecord() : bAutoFixed(false) {}
};

/** Post-organize summary report */
USTRUCT()
struct FOrganizeReport
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovedAssetRecord> MovedAssets;

    UPROPERTY()
    TArray<FBrokenReferenceRecord> BrokenReferences;

    UPROPERTY()
    TArray<FString> SkippedWhitelistedAssets;

    UPROPERTY()
    TArray<FString> RedirectorsFixed;

    UPROPERTY()
    int32 TotalScanned = 0;

    UPROPERTY()
    int32 TotalMoved = 0;

    UPROPERTY()
    int32 TotalSkipped = 0;

    UPROPERTY()
    int32 BrokenRefCount = 0;

    UPROPERTY()
    bool bVerificationRan = false;

    FOrganizeReport()
        : TotalScanned(0), TotalMoved(0), TotalSkipped(0)
        , BrokenRefCount(0), bVerificationRan(false)
    {}

    bool IsClean() const { return BrokenRefCount == 0 && !BrokenReferences.ContainsByPredicate(
        [](const FBrokenReferenceRecord& R){ return !R.bAutoFixed; }); }
};

/** Organization configuration */
USTRUCT(BlueprintType)
struct FOrganizeConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    FString RootPath = TEXT("/Game");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    bool bCreateFolders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    bool bFixReferences = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    bool bCleanEmptyFolders = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    bool bDryRun = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    EFolderCreationStrategy FolderStrategy = EFolderCreationStrategy::LazyCreate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Folder")
    FRootFolderConfig RootFolder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    TArray<FString> ExcludedFolders;

    /** Folders whose assets will NEVER be moved (whitelist).
     *  Stored as /Game/-relative paths, e.g. "/Game/Core".
     *  Prefix-matched: "/Game/Core" protects "/Game/Core/Sub/Asset" too. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whitelist")
    TArray<FString> WhitelistedFolders;

    /** If true, scan whitelisted-folder assets and fix their redirectors
     *  after moved assets leave redirectors at original paths. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Whitelist")
    bool bUpdateWhitelistedAssetReferences = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bSaveHistory = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bAutoRollbackOnCancel = true;

    // Type-specific folder overrides
    TMap<EAssetOrganizeType, FString> TypeToFolderMap;

    FOrganizeConfig()
        : RootPath(TEXT("/Game"))
        , bCreateFolders(true)
        , bFixReferences(true)
        , bCleanEmptyFolders(true)
        , bDryRun(false)
        , FolderStrategy(EFolderCreationStrategy::LazyCreate)
        , bUpdateWhitelistedAssetReferences(true)
        , bSaveHistory(true)
        , bAutoRollbackOnCancel(true)
    {}

    /** Get effective target path for a type */
    FString GetTargetPath(EAssetOrganizeType Type, const FString& DefaultPath) const
    {
        FString Path = DefaultPath;
        if (const FString* Override = TypeToFolderMap.Find(Type))
        {
            Path = *Override;
        }
        return RootFolder.GetEffectivePath(Path);
    }
};

/** Preview item for dry run */
USTRUCT(BlueprintType)
struct FMovePreviewItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FString AssetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FString AssetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FString FromPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FString ToPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    bool bWillConflict = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FString ConflictResolution;     // How conflict will be resolved (e.g., "Rename to Asset_2")

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    bool bIsNewFolder = false;      // Whether target folder will be created

    FMovePreviewItem()
        : bWillConflict(false)
        , bIsNewFolder(false)
    {}

    FMovePreviewItem(const FString& InName, const FString& InClass,
                     const FString& InFrom, const FString& InTo,
                     bool bConflict = false)
        : AssetName(InName)
        , AssetClass(InClass)
        , FromPath(InFrom)
        , ToPath(InTo)
        , bWillConflict(bConflict)
        , bIsNewFolder(false)
    {}
};

/** Organization result */
USTRUCT(BlueprintType)
struct FOrganizeResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 TotalAssets = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 MovedAssets = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 FixedReferences = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 RemovedEmptyFolders = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 CreatedFolders = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 SkippedAssets = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    TArray<FString> Errors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    TArray<FString> Warnings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    TArray<FString> DetailedLog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    TArray<FMovePreviewItem> PreviewItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    FString HistoryFilePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    bool bWasCancelled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    float DurationSeconds = 0.0f;

    FOrganizeResult()
        : TotalAssets(0)
        , MovedAssets(0)
        , FixedReferences(0)
        , RemovedEmptyFolders(0)
        , CreatedFolders(0)
        , SkippedAssets(0)
        , bWasCancelled(false)
        , DurationSeconds(0.0f)
    {}

    bool IsSuccess() const
    {
        return Errors.Num() == 0 && !bWasCancelled;
    }
};
