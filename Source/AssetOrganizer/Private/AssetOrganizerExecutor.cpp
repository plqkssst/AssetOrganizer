// Copyright UEMaster. All Rights Reserved.

#include "AssetOrganizerExecutor.h"
#include "AssetOrganizerSettings.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "EditorAssetLibrary.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "EngineUtils.h"  // For TActorIterator

#define LOCTEXT_NAMESPACE "AssetOrganizer"

// NOTE: FName(TEXT("ObjectRedirector")) is already defined in engine (UnrealNames.inl)
// We use FName(TEXT("ObjectRedirector")) directly to avoid redefinition errors

// Static members
TAtomic<bool> FAssetOrganizerExecutor::bCancelRequested(false);
FCriticalSection FAssetOrganizerExecutor::AsyncLock;
TSharedPtr<FAsyncTask<FOrganizeAssetsTask>> FAssetOrganizerExecutor::CurrentAsyncTask(nullptr);

// ============================================================
// Type Detection (62+ types)
// ============================================================

EAssetOrganizeType FAssetOrganizerExecutor::DetermineAssetType(const FAssetData& AssetData, const TArray<FCustomAssetRule>* CustomRules)
{
    FName AssetClass = AssetData.AssetClass;

    // Check custom rules first (user-defined priority)
    if (CustomRules)
    {
        for (const FCustomAssetRule& Rule : *CustomRules)
        {
            if (Rule.bEnabled && FName(*Rule.ClassName) == AssetClass)
            {
                return EAssetOrganizeType::Custom;
            }
        }
    }

    // ========== MESHES ==========
    if (AssetClass == FName("StaticMesh"))        return EAssetOrganizeType::StaticMesh;
    if (AssetClass == FName("SkeletalMesh"))      return EAssetOrganizeType::SkeletalMesh;
    if (AssetClass == FName("Skeleton"))          return EAssetOrganizeType::Skeleton;
    if (AssetClass == FName("PhysicsAsset"))      return EAssetOrganizeType::PhysicsAsset;
    if (AssetClass == FName("DestructibleMesh"))  return EAssetOrganizeType::DestructibleMesh;

    // ========== MATERIALS ==========
    if (AssetClass == FName("Material"))                    return EAssetOrganizeType::Material;
    if (AssetClass == FName("MaterialInstanceConstant"))    return EAssetOrganizeType::MaterialInstanceConstant;
    if (AssetClass == FName("MaterialInstanceDynamic"))     return EAssetOrganizeType::MaterialInstanceDynamic;
    if (AssetClass == FName("MaterialFunction"))            return EAssetOrganizeType::MaterialFunction;
    if (AssetClass == FName("MaterialParameterCollection")) return EAssetOrganizeType::MaterialParameterCollection;
    if (AssetClass == FName("SubsurfaceProfile"))           return EAssetOrganizeType::SubsurfaceProfile;
    if (AssetClass == FName("PhysicalMaterial"))            return EAssetOrganizeType::PhysicalMaterial;

    // ========== TEXTURES ==========
    if (AssetClass == FName("Texture2D"))               return EAssetOrganizeType::Texture2D;
    if (AssetClass == FName("TextureCube"))             return EAssetOrganizeType::TextureCube;
    if (AssetClass == FName("TextureRenderTarget2D"))   return EAssetOrganizeType::TextureRenderTarget2D;
    if (AssetClass == FName("TextureRenderTargetCube")) return EAssetOrganizeType::TextureRenderTargetCube;
    if (AssetClass == FName("MediaTexture"))            return EAssetOrganizeType::MediaTexture;
    if (AssetClass == FName("TextureLightProfile"))     return EAssetOrganizeType::TextureLightProfile;
    if (AssetClass == FName("VolumeTexture"))           return EAssetOrganizeType::VolumeTexture;

    // ========== ANIMATION ==========
    if (AssetClass == FName("AnimSequence"))            return EAssetOrganizeType::AnimSequence;
    if (AssetClass == FName("AnimMontage"))             return EAssetOrganizeType::AnimMontage;
    if (AssetClass == FName("AnimBlueprint"))           return EAssetOrganizeType::AnimBlueprint;
    if (AssetClass == FName("AnimComposite"))           return EAssetOrganizeType::AnimComposite;
    if (AssetClass == FName("BlendSpace"))              return EAssetOrganizeType::BlendSpace;
    if (AssetClass == FName("BlendSpace1D"))            return EAssetOrganizeType::BlendSpace1D;
    if (AssetClass == FName("AimOffsetBlendSpace"))     return EAssetOrganizeType::AimOffsetBlendSpace;
    if (AssetClass == FName("AimOffsetBlendSpace1D"))   return EAssetOrganizeType::AimOffsetBlendSpace1D;
    if (AssetClass == FName("PoseAsset"))               return EAssetOrganizeType::PoseAsset;
    if (AssetClass == FName("LevelSequence"))           return EAssetOrganizeType::LevelSequence;

    // ========== BLUEPRINTS ==========
    if (AssetClass == FName("Blueprint"))
    {
        // Check if it's a widget blueprint
        FString GeneratedClass;
        AssetData.GetTagValue(FName("GeneratedClass"), GeneratedClass);
        if (GeneratedClass.Contains(TEXT("UUserWidget")) || GeneratedClass.Contains(TEXT("UserWidget")))
        {
            return EAssetOrganizeType::WidgetBlueprint;
        }
        return EAssetOrganizeType::Blueprint;
    }
    if (AssetClass == FName("WidgetBlueprint"))     return EAssetOrganizeType::WidgetBlueprint;
    if (AssetClass == FName("UserDefinedEnum"))     return EAssetOrganizeType::UserDefinedEnum;
    if (AssetClass == FName("UserDefinedStruct"))   return EAssetOrganizeType::UserDefinedStruct;

    // ========== AI ==========
    if (AssetClass == FName("BehaviorTree"))        return EAssetOrganizeType::BehaviorTree;
    if (AssetClass == FName("BlackboardData"))      return EAssetOrganizeType::BlackboardData;
    if (AssetClass == FName("EnvQuery"))            return EAssetOrganizeType::EnvQuery;

    // ========== DATA ==========
    if (AssetClass == FName("DataTable"))               return EAssetOrganizeType::DataTable;
    if (AssetClass == FName("DataAsset"))               return EAssetOrganizeType::DataAsset;
    if (AssetClass == FName("StringTable"))             return EAssetOrganizeType::StringTable;
    if (AssetClass == FName("CurveFloat"))              return EAssetOrganizeType::CurveFloat;
    if (AssetClass == FName("CurveVector"))             return EAssetOrganizeType::CurveVector;
    if (AssetClass == FName("CurveLinearColor"))        return EAssetOrganizeType::CurveLinearColor;
    if (AssetClass == FName("CurveTable"))              return EAssetOrganizeType::CurveTable;
    if (AssetClass == FName("CompositeCurveTable"))     return EAssetOrganizeType::CompositeCurveTable;

    // ========== AUDIO ==========
    if (AssetClass == FName("SoundWave"))               return EAssetOrganizeType::SoundWave;
    if (AssetClass == FName("SoundCue"))                return EAssetOrganizeType::SoundCue;
    if (AssetClass == FName("SoundAttenuation"))        return EAssetOrganizeType::SoundAttenuation;
    if (AssetClass == FName("SoundClass"))              return EAssetOrganizeType::SoundClass;
    if (AssetClass == FName("SoundMix"))                return EAssetOrganizeType::SoundMix;
    if (AssetClass == FName("SoundConcurrency"))        return EAssetOrganizeType::SoundConcurrency;
    if (AssetClass == FName("ReverbEffect"))            return EAssetOrganizeType::ReverbEffect;

    // ========== VFX ==========
    if (AssetClass == FName("ParticleSystem"))          return EAssetOrganizeType::ParticleSystem;
    if (AssetClass == FName("NiagaraSystem"))           return EAssetOrganizeType::NiagaraSystem;
    if (AssetClass == FName("NiagaraEmitter"))          return EAssetOrganizeType::NiagaraEmitter;
    if (AssetClass == FName("NiagaraParameterCollection")) return EAssetOrganizeType::NiagaraParameterCollection;

    // ========== ENVIRONMENT ==========
    if (AssetClass == FName("FoliageType"))                 return EAssetOrganizeType::FoliageType;
    if (AssetClass == FName("FoliageType_Actor"))           return EAssetOrganizeType::FoliageType;
    if (AssetClass == FName("LandscapeGrassType"))          return EAssetOrganizeType::LandscapeGrassType;
    if (AssetClass == FName("LandscapeLayerInfoObject"))    return EAssetOrganizeType::LandscapeLayerInfoObject;

    // ========== MEDIA ==========
    if (AssetClass == FName("MediaPlayer"))         return EAssetOrganizeType::MediaPlayer;
    if (AssetClass == FName("FileMediaSource"))     return EAssetOrganizeType::FileMediaSource;

    // ========== LEVELS ==========
    if (AssetClass == FName("World") || AssetClass == FName("Level"))
        return EAssetOrganizeType::World;

    return EAssetOrganizeType::Other;
}

FName FAssetOrganizerExecutor::GetClassNameFromType(EAssetOrganizeType Type)
{
    switch (Type)
    {
        // Meshes
        case EAssetOrganizeType::StaticMesh:        return FName("StaticMesh");
        case EAssetOrganizeType::SkeletalMesh:      return FName("SkeletalMesh");
        case EAssetOrganizeType::Skeleton:          return FName("Skeleton");
        case EAssetOrganizeType::PhysicsAsset:      return FName("PhysicsAsset");
        case EAssetOrganizeType::DestructibleMesh:  return FName("DestructibleMesh");

        // Materials
        case EAssetOrganizeType::Material:                  return FName("Material");
        case EAssetOrganizeType::MaterialInstanceConstant:  return FName("MaterialInstanceConstant");
        case EAssetOrganizeType::MaterialInstanceDynamic:   return FName("MaterialInstanceDynamic");
        case EAssetOrganizeType::MaterialFunction:          return FName("MaterialFunction");
        case EAssetOrganizeType::MaterialParameterCollection: return FName("MaterialParameterCollection");
        case EAssetOrganizeType::SubsurfaceProfile:         return FName("SubsurfaceProfile");
        case EAssetOrganizeType::PhysicalMaterial:          return FName("PhysicalMaterial");

        // Textures
        case EAssetOrganizeType::Texture2D:                 return FName("Texture2D");
        case EAssetOrganizeType::TextureCube:               return FName("TextureCube");
        case EAssetOrganizeType::TextureRenderTarget2D:     return FName("TextureRenderTarget2D");
        case EAssetOrganizeType::TextureRenderTargetCube:   return FName("TextureRenderTargetCube");
        case EAssetOrganizeType::MediaTexture:              return FName("MediaTexture");
        case EAssetOrganizeType::TextureLightProfile:       return FName("TextureLightProfile");
        case EAssetOrganizeType::VolumeTexture:             return FName("VolumeTexture");

        // Animation
        case EAssetOrganizeType::AnimSequence:          return FName("AnimSequence");
        case EAssetOrganizeType::AnimMontage:           return FName("AnimMontage");
        case EAssetOrganizeType::AnimBlueprint:         return FName("AnimBlueprint");
        case EAssetOrganizeType::AnimComposite:         return FName("AnimComposite");
        case EAssetOrganizeType::BlendSpace:            return FName("BlendSpace");
        case EAssetOrganizeType::BlendSpace1D:          return FName("BlendSpace1D");
        case EAssetOrganizeType::AimOffsetBlendSpace:   return FName("AimOffsetBlendSpace");
        case EAssetOrganizeType::AimOffsetBlendSpace1D: return FName("AimOffsetBlendSpace1D");
        case EAssetOrganizeType::PoseAsset:             return FName("PoseAsset");
        case EAssetOrganizeType::LevelSequence:         return FName("LevelSequence");

        // Blueprints
        case EAssetOrganizeType::Blueprint:             return FName("Blueprint");
        case EAssetOrganizeType::WidgetBlueprint:       return FName("WidgetBlueprint");
        case EAssetOrganizeType::UserDefinedEnum:       return FName("UserDefinedEnum");
        case EAssetOrganizeType::UserDefinedStruct:     return FName("UserDefinedStruct");

        // AI
        case EAssetOrganizeType::BehaviorTree:          return FName("BehaviorTree");
        case EAssetOrganizeType::BlackboardData:        return FName("BlackboardData");
        case EAssetOrganizeType::EnvQuery:              return FName("EnvQuery");

        // Data
        case EAssetOrganizeType::DataTable:             return FName("DataTable");
        case EAssetOrganizeType::DataAsset:             return FName("DataAsset");
        case EAssetOrganizeType::StringTable:           return FName("StringTable");
        case EAssetOrganizeType::CurveFloat:            return FName("CurveFloat");
        case EAssetOrganizeType::CurveVector:           return FName("CurveVector");
        case EAssetOrganizeType::CurveLinearColor:      return FName("CurveLinearColor");
        case EAssetOrganizeType::CurveTable:            return FName("CurveTable");
        case EAssetOrganizeType::CompositeCurveTable:   return FName("CompositeCurveTable");

        // Audio
        case EAssetOrganizeType::SoundWave:             return FName("SoundWave");
        case EAssetOrganizeType::SoundCue:              return FName("SoundCue");
        case EAssetOrganizeType::SoundAttenuation:      return FName("SoundAttenuation");
        case EAssetOrganizeType::SoundClass:            return FName("SoundClass");
        case EAssetOrganizeType::SoundMix:              return FName("SoundMix");
        case EAssetOrganizeType::SoundConcurrency:      return FName("SoundConcurrency");
        case EAssetOrganizeType::ReverbEffect:          return FName("ReverbEffect");

        // VFX
        case EAssetOrganizeType::ParticleSystem:        return FName("ParticleSystem");
        case EAssetOrganizeType::NiagaraSystem:         return FName("NiagaraSystem");
        case EAssetOrganizeType::NiagaraEmitter:        return FName("NiagaraEmitter");
        case EAssetOrganizeType::NiagaraParameterCollection: return FName("NiagaraParameterCollection");

        // Environment
        case EAssetOrganizeType::FoliageType:           return FName("FoliageType");
        case EAssetOrganizeType::LandscapeGrassType:    return FName("LandscapeGrassType");
        case EAssetOrganizeType::LandscapeLayerInfoObject: return FName("LandscapeLayerInfoObject");

        // Media
        case EAssetOrganizeType::MediaPlayer:           return FName("MediaPlayer");
        case EAssetOrganizeType::FileMediaSource:       return FName("FileMediaSource");

        // Levels
        case EAssetOrganizeType::World:                 return FName("World");

        default: return NAME_None;
    }
}

FAssetTypeInfo FAssetOrganizerExecutor::GetTypeInfo(EAssetOrganizeType Type)
{
    UAssetOrganizerSettings* Settings = GetMutableDefault<UAssetOrganizerSettings>();
    if (Settings)
    {
        for (const FAssetTypeInfo& Info : Settings->AssetTypeConfigs)
        {
            if (Info.Type == Type)
            {
                return Info;
            }
        }
    }
    return FAssetTypeInfo();
}

TArray<FAssetTypeInfo> FAssetOrganizerExecutor::GetAllTypeInfos()
{
    UAssetOrganizerSettings* Settings = GetMutableDefault<UAssetOrganizerSettings>();
    if (Settings)
    {
        return Settings->AssetTypeConfigs;
    }
    return TArray<FAssetTypeInfo>();
}

FString FAssetOrganizerExecutor::GetTargetFolder(EAssetOrganizeType Type, const FOrganizeConfig& Config)
{
    FAssetTypeInfo Info = GetTypeInfo(Type);
    if (!Info.TargetPath.IsEmpty())
    {
        return Config.GetTargetPath(Type, Info.TargetPath);
    }

    // Fallback defaults
    switch (Type)
    {
        case EAssetOrganizeType::StaticMesh:      return Config.GetTargetPath(Type, TEXT("/Game/Models/StaticMeshes"));
        case EAssetOrganizeType::SkeletalMesh:    return Config.GetTargetPath(Type, TEXT("/Game/Models/SkeletalMeshes"));
        case EAssetOrganizeType::Skeleton:        return Config.GetTargetPath(Type, TEXT("/Game/Models/Skeletons"));
        case EAssetOrganizeType::PhysicsAsset:    return Config.GetTargetPath(Type, TEXT("/Game/Models/Physics"));
        case EAssetOrganizeType::Material:        return Config.GetTargetPath(Type, TEXT("/Game/Materials"));
        case EAssetOrganizeType::MaterialInstanceConstant:
        case EAssetOrganizeType::MaterialInstanceDynamic:
                                                  return Config.GetTargetPath(Type, TEXT("/Game/Materials/Instances"));
        case EAssetOrganizeType::MaterialFunction:return Config.GetTargetPath(Type, TEXT("/Game/Materials/Functions"));
        case EAssetOrganizeType::Texture2D:
        case EAssetOrganizeType::TextureCube:
        case EAssetOrganizeType::TextureRenderTarget2D:
                                                  return Config.GetTargetPath(Type, TEXT("/Game/Textures"));
        case EAssetOrganizeType::Blueprint:       return Config.GetTargetPath(Type, TEXT("/Game/Blueprints"));
        case EAssetOrganizeType::WidgetBlueprint: return Config.GetTargetPath(Type, TEXT("/Game/UI/Widgets"));
        case EAssetOrganizeType::ParticleSystem:  return Config.GetTargetPath(Type, TEXT("/Game/Effects/Particles"));
        case EAssetOrganizeType::NiagaraSystem:   return Config.GetTargetPath(Type, TEXT("/Game/Effects/Niagara"));
        case EAssetOrganizeType::AnimSequence:    return Config.GetTargetPath(Type, TEXT("/Game/Animations/Sequences"));
        case EAssetOrganizeType::AnimMontage:     return Config.GetTargetPath(Type, TEXT("/Game/Animations/Montages"));
        case EAssetOrganizeType::AnimBlueprint:   return Config.GetTargetPath(Type, TEXT("/Game/Animations/Blueprints"));
        case EAssetOrganizeType::SoundWave:       return Config.GetTargetPath(Type, TEXT("/Game/Audio/Waves"));
        case EAssetOrganizeType::SoundCue:        return Config.GetTargetPath(Type, TEXT("/Game/Audio/Cues"));
        case EAssetOrganizeType::DataTable:       return Config.GetTargetPath(Type, TEXT("/Game/Data/Tables"));
        case EAssetOrganizeType::World:           return Config.GetTargetPath(Type, TEXT("/Game/Maps"));
        default:                                  return Config.GetTargetPath(Type, TEXT("/Game/Other"));
    }
}

int32 FAssetOrganizerExecutor::GetTypePriority(EAssetOrganizeType Type)
{
    FAssetTypeInfo Info = GetTypeInfo(Type);
    return Info.Priority;
}

// ============================================================
// Asset Operations
// ============================================================

TArray<FAssetData> FAssetOrganizerExecutor::GetAssetsInPath(const FString& Path, bool bRecursive, const TArray<FString>& ExcludedFolders)
{
    TArray<FAssetData> Assets;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Path));
    Filter.bRecursivePaths = bRecursive;

    AssetRegistry.GetAssets(Filter, Assets);

    // Filter out excluded folders
    if (ExcludedFolders.Num() > 0)
    {
        Assets.RemoveAll([&ExcludedFolders](const FAssetData& Asset)
        {
            FString PackagePath = Asset.PackagePath.ToString();
            for (const FString& Excluded : ExcludedFolders)
            {
                if (PackagePath.StartsWith(Excluded))
                {
                    return true;
                }
            }
            return false;
        });
    }

    return Assets;
}

TMap<EAssetOrganizeType, TArray<FAssetData>> FAssetOrganizerExecutor::GroupAssetsByType(
    const TArray<FAssetData>& Assets,
    const TArray<FCustomAssetRule>* CustomRules)
{
    TMap<EAssetOrganizeType, TArray<FAssetData>> Grouped;
    for (const FAssetData& Asset : Assets)
    {
        EAssetOrganizeType Type = DetermineAssetType(Asset, CustomRules);
        Grouped.FindOrAdd(Type).Add(Asset);
    }
    return Grouped;
}

bool FAssetOrganizerExecutor::IsInExcludedFolder(const FAssetData& Asset, const TArray<FString>& ExcludedFolders)
{
    FString PackagePath = Asset.PackagePath.ToString();
    for (const FString& Excluded : ExcludedFolders)
    {
        if (PackagePath.StartsWith(Excluded))
        {
            return true;
        }
    }
    return false;
}

// ============================================================
// Lazy Folder Creation (Phase 2)
// ============================================================

TArray<FString> FAssetOrganizerExecutor::CollectNeededFolders(
    const TArray<FAssetData>& Assets,
    const FOrganizeConfig& Config,
    const TArray<FCustomAssetRule>* CustomRules)
{
    TSet<FString> NeededFolders;
    TSet<FName> ActiveClasses;

    // Collect active asset classes from assets to organize
    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetClass != FName(TEXT("ObjectRedirector")))
        {
            ActiveClasses.Add(Asset.AssetClass);
        }
    }

    UAssetOrganizerSettings* Settings = GetMutableDefault<UAssetOrganizerSettings>();
    if (!Settings)
    {
        return TArray<FString>();
    }

    // Find matching type configs
    for (const FAssetTypeInfo& Info : Settings->AssetTypeConfigs)
    {
        if (Info.bEnabled && ActiveClasses.Contains(Info.ClassName))
        {
            FString EffectivePath = Config.GetTargetPath(Info.Type, Info.TargetPath);
            NeededFolders.Add(EffectivePath);
        }
    }

    // Add custom rules
    if (CustomRules)
    {
        for (const FCustomAssetRule& Rule : *CustomRules)
        {
            if (Rule.bEnabled && ActiveClasses.Contains(FName(*Rule.ClassName)))
            {
                FString EffectivePath = Config.RootFolder.GetEffectivePath(Rule.TargetPath);
                NeededFolders.Add(EffectivePath);
            }
        }
    }

    return NeededFolders.Array();
}

int32 FAssetOrganizerExecutor::CreateNeededFolders(
    const TArray<FString>& Folders,
    TArray<FString>& OutCreated,
    TArray<FString>& OutErrors)
{
    int32 CreatedCount = 0;

    for (const FString& Folder : Folders)
    {
        if (EnsureFolderExists(Folder))
        {
            if (!UEditorAssetLibrary::DoesDirectoryExist(Folder))
            {
                OutCreated.Add(Folder);
            }
            CreatedCount++;
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("Failed to create folder: %s"), *Folder));
        }
    }

    return CreatedCount;
}

// ============================================================
// Main Organization Logic
// ============================================================

FOrganizeResult FAssetOrganizerExecutor::OrganizeAssets(
    const FOrganizeConfig& Config,
    const FOnProgress& ProgressCallback,
    const FOnLog& LogCallback)
{
    FDateTime StartTime = FDateTime::Now();
    FOrganizeResult Result;
    bCancelRequested = false;

    auto Log = [&](const FString& Message)
    {
        if (LogCallback.IsBound())
        {
            LogCallback.Execute(Message);
        }
    };

    // ── Step 1: Scan ─────────────────────────────────────────────────────────
    if (ProgressCallback.IsBound())
        ProgressCallback.Execute(0.0f, LOCTEXT("Scanning", "Scanning assets..."));

    Log(TEXT("Starting asset organization..."));
    Log(FString::Printf(TEXT("Root path: %s"), *Config.RootPath));

    // Merge ExcludedFolders + WhitelistedFolders into a single exclusion list for the scan
    TArray<FString> AllExcluded = Config.ExcludedFolders;
    for (const FString& WL : Config.WhitelistedFolders)
    {
        AllExcluded.AddUnique(WL);
    }
    TArray<FAssetData> AllAssets = GetAssetsInPath(Config.RootPath, true, AllExcluded);
    Result.TotalAssets = AllAssets.Num();

    Log(FString::Printf(TEXT("Found %d assets to process"), Result.TotalAssets));

    if (AllAssets.Num() == 0)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(1.0f, LOCTEXT("NoAssets", "No assets found"));
        return Result;
    }

    // ── Step 2: Group by type ────────────────────────────────────────────────
    if (ProgressCallback.IsBound())
        ProgressCallback.Execute(0.05f, LOCTEXT("Grouping", "Grouping by type..."));

    UAssetOrganizerSettings* Settings = GetMutableDefault<UAssetOrganizerSettings>();
    TMap<EAssetOrganizeType, TArray<FAssetData>> GroupedAssets = GroupAssetsByType(AllAssets, &Settings->CustomRules);

    Log(FString::Printf(TEXT("Grouped into %d asset types"), GroupedAssets.Num()));

    // ── Step 3: Create target folders (Lazy Creation) ────────────────────────
    if (Config.bCreateFolders && Config.FolderStrategy != EFolderCreationStrategy::CreateAll)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.08f, LOCTEXT("PlanningFolders", "Planning folder creation..."));

        TArray<FString> NeededFolders = CollectNeededFolders(AllAssets, Config, &Settings->CustomRules);
        Log(FString::Printf(TEXT("Will create %d folders (lazy creation)"), NeededFolders.Num()));

        TArray<FString> CreatedFolders;
        TArray<FString> FolderErrors;
        Result.CreatedFolders = CreateNeededFolders(NeededFolders, CreatedFolders, FolderErrors);

        for (const FString& Created : CreatedFolders)
        {
            Log(FString::Printf(TEXT("Created folder: %s"), *Created));
        }
        for (const FString& Error : FolderErrors)
        {
            Log(Error);
            Result.Errors.Add(Error);
        }
    }
    else if (Config.bCreateFolders)
    {
        // Create all folders upfront (old behavior)
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.08f, LOCTEXT("CreatingFolders", "Creating target folders..."));

        for (const auto& Pair : GroupedAssets)
        {
            FString FolderPath = GetTargetFolder(Pair.Key, Config);
            EnsureFolderExists(FolderPath);
        }
    }

    // ── Step 4: Dry run / Preview ────────────────────────────────────────────
    if (Config.bDryRun)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.1f, LOCTEXT("BuildingPreview", "Building preview..."));

        Result.PreviewItems = GeneratePreview(AllAssets, Config, &Settings->CustomRules);

        for (const FMovePreviewItem& Item : Result.PreviewItems)
        {
            if (Item.FromPath != Item.ToPath)
            {
                Result.MovedAssets++;
            }
        }

        Result.TotalAssets = Result.PreviewItems.Num();

        Log(FString::Printf(TEXT("Preview complete. %d assets will be moved."), Result.MovedAssets));

        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(1.0f, LOCTEXT("PreviewComplete", "Preview complete"));

        return Result;
    }

    // ── Step 5: Build type priority order ────────────────────────────────────
    TArray<EAssetOrganizeType> TypeOrder;
    for (const auto& Pair : GroupedAssets)
    {
        TypeOrder.Add(Pair.Key);
    }

    // Sort by priority (lower = moved first)
    TypeOrder.Sort([&](EAssetOrganizeType A, EAssetOrganizeType B)
    {
        return GetTypePriority(A) < GetTypePriority(B);
    });

    // ── Step 6: Initialize history entry early ──────────────────────────────
    FOrganizeHistoryEntry HistoryEntry;
    HistoryEntry.Description = FString::Printf(TEXT("Asset organization on %s"), *Config.RootPath);
    HistoryEntry.TotalAssets = AllAssets.Num();

    // ── Step 7: Save level references snapshot (CRITICAL for rollback) ───────
    // Before moving any assets, save a snapshot of which levels reference which assets.
    // This allows us to properly restore references during rollback.
    if (Config.bSaveHistory && !Config.bDryRun)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.09f, LOCTEXT("SavingRefs", "Saving level references..."));

        HistoryEntry.LevelReferences = SaveLevelReferencesSnapshot(AllAssets, LogCallback);

        Log(FString::Printf(TEXT("Saved %d level reference snapshots"), HistoryEntry.LevelReferences.Num()));
    }

    // ── Step 8: Move assets ──────────────────────────────────────────────────
    FScopedSlowTask SlowTask((float)AllAssets.Num(), LOCTEXT("Organizing", "Organizing assets..."));
    SlowTask.MakeDialog();

    int32 ProcessedCount = 0;

    for (EAssetOrganizeType Type : TypeOrder)
    {
        if (bCancelRequested)
        {
            Result.bWasCancelled = true;
            Log(TEXT("Operation cancelled by user"));
            break;
        }

        TArray<FAssetData>* Group = GroupedAssets.Find(Type);
        if (!Group || Group->Num() == 0)
        {
            continue;
        }

        // Topological sort within type group
        TArray<FAssetData> Sorted = SortByDependency(*Group);
        FString TargetFolder = GetTargetFolder(Type, Config);

        for (const FAssetData& Asset : Sorted)
        {
            if (bCancelRequested)
            {
                Result.bWasCancelled = true;
                break;
            }

            SlowTask.EnterProgressFrame(1.0f, FText::Format(
                LOCTEXT("MovingAsset", "Moving: {0}"),
                FText::FromName(Asset.AssetName)
            ));

            FMoveRecord Record;
            if (MoveAsset(Asset, TargetFolder, Result.Errors, Result.Warnings, &Record))
            {
                Result.MovedAssets++;
                HistoryEntry.Records.Add(Record);
            }

            ProcessedCount++;
            if (ProgressCallback.IsBound())
            {
                float Pct = 0.1f + 0.7f * (float)ProcessedCount / (float)AllAssets.Num();
                ProgressCallback.Execute(Pct, FText::Format(
                    LOCTEXT("Progress", "Processing: {0}/{1}"),
                    FText::AsNumber(ProcessedCount), FText::AsNumber(AllAssets.Num())
                ));
            }
        }

        // ========== CRITICAL: Fix up redirectors after each type group ==========
        // According to logo.txt best practices, we must fix redirectors immediately
        // after moving each batch to avoid "redirector stacking" which causes reference loss
        if (!Result.bWasCancelled && Config.bFixReferences && Result.MovedAssets > 0)
        {
            // Get AssetRegistry for this scope
            FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

            // Force asset registry scan to ensure it knows about new locations
            AssetRegistry.ScanPathsSynchronous({Config.RootPath}, true);

            int32 FixedInThisBatch = 0;
            FixUpAllReferences(Config.RootPath, FixedInThisBatch, LogCallback);
            Result.FixedReferences += FixedInThisBatch;

            if (LogCallback.IsBound() && FixedInThisBatch > 0)
            {
                LogCallback.Execute(FString::Printf(TEXT("Fixed %d redirectors after moving %s assets"),
                    FixedInThisBatch, *GetClassNameFromType(Type).ToString()));
            }

            // Scan again after fixing to ensure clean state for next batch
            AssetRegistry.ScanPathsSynchronous({Config.RootPath}, true);
        }

        if (Result.bWasCancelled)
            break;
    }

    // ── Step 9: Save history ─────────────────────────────────────────────────
    if (Config.bSaveHistory && Result.MovedAssets > 0 && !Result.bWasCancelled)
    {
        HistoryEntry.MovedAssets = Result.MovedAssets;
        Result.HistoryFilePath = SaveHistory(HistoryEntry);
        Log(FString::Printf(TEXT("History saved to: %s"), *Result.HistoryFilePath));
    }

    // ── Step 9.5: Post-organize verification pass (NEW) ─────────────────────
    // Build FOrganizeReport from HistoryEntry records for verification
    FOrganizeReport OrganizeReport;
    OrganizeReport.TotalScanned  = Result.TotalAssets;
    OrganizeReport.TotalMoved    = Result.MovedAssets;
    OrganizeReport.TotalSkipped  = Result.SkippedAssets;

    // Populate moved-asset records from history
    for (const FMoveRecord& Rec : HistoryEntry.Records)
    {
        OrganizeReport.MovedAssets.Add(
            FMovedAssetRecord(Rec.OldPath, Rec.NewPath, Rec.AssetClass));
    }

    if (!Result.bWasCancelled && Result.MovedAssets > 0)
    {
        const UAssetOrganizerSettings* VerifySettings = GetDefault<UAssetOrganizerSettings>();
        if (VerifySettings && VerifySettings->bRunVerificationAfterOrganize)
        {
            if (ProgressCallback.IsBound())
                ProgressCallback.Execute(0.80f, LOCTEXT("Verifying", "Verifying references..."));

            Log(TEXT("Running post-organize reference verification..."));
            VerifyReferences(OrganizeReport.MovedAssets, OrganizeReport);

            OrganizeReport.bVerificationRan = true;

            if (OrganizeReport.BrokenRefCount > 0)
            {
                Log(FString::Printf(TEXT("[WARN] %d broken reference(s) detected after organize!"),
                    OrganizeReport.BrokenRefCount));
                Result.Warnings.Add(FString::Printf(
                    TEXT("%d broken reference(s) detected. Check the Organize Report for details."),
                    OrganizeReport.BrokenRefCount));
            }
            else
            {
                Log(TEXT("Reference verification passed — all references intact."));
            }
        }

        // Update whitelisted folder references if enabled
        if (Config.bUpdateWhitelistedAssetReferences && Config.WhitelistedFolders.Num() > 0)
        {
            Log(TEXT("Updating references in whitelisted folders..."));
            UpdateWhitelistedFolderReferences(Config, OrganizeReport, LogCallback);
        }
    }

    // ── Step 10: Additional reference checks and level save ──────────────────
    // NOTE: Redirectors are already fixed after each batch in Step 8
    // This step performs additional validation and CRITICAL: saves all levels
    if (!Result.bWasCancelled && Config.bFixReferences)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.82f, LOCTEXT("FixingReferences", "Validating references and saving levels..."));

        // Additional validation (material references, etc.)
        int32 FixedByRedirector, TrulyMissing;
        TArray<FString> DetailedLog;
        int32 AdditionalFixed = FixBrokenReferences(Config.RootPath, FixedByRedirector, TrulyMissing, DetailedLog);
        Result.FixedReferences += AdditionalFixed;

        for (const FString& LogEntry : DetailedLog)
        {
            Log(LogEntry);
        }

        // ========== CRITICAL: Save all loaded levels ==========
        // According to logo.txt: After fixing redirectors, must open and save levels
        // to ensure their serialized references are updated
        TArray<UPackage*> LevelPackages;
        for (TObjectIterator<UWorld> WorldIt; WorldIt; ++WorldIt)
        {
            UWorld* World = *WorldIt;
            if (World && World->GetOutermost() && World->GetOutermost()->GetName().StartsWith(TEXT("/Game/")))
            {
                World->GetOutermost()->MarkPackageDirty();
                LevelPackages.AddUnique(World->GetOutermost());
            }
        }

        if (LevelPackages.Num() > 0)
        {
            FEditorFileUtils::PromptForCheckoutAndSave(LevelPackages, true, false);
            Log(FString::Printf(TEXT("Saved %d levels to persist reference updates"), LevelPackages.Num()));
        }
    }

    // ── Step 11: Clean empty folders ──────────────────────────────────────────
    if (!Result.bWasCancelled && Config.bCleanEmptyFolders)
    {
        if (ProgressCallback.IsBound())
            ProgressCallback.Execute(0.95f, LOCTEXT("CleaningFolders", "Cleaning empty folders..."));

        TArray<FString> RemovedFolders;
        Result.RemovedEmptyFolders = CleanEmptyFolders(Config.RootPath, &RemovedFolders);

        for (const FString& Removed : RemovedFolders)
        {
            Log(FString::Printf(TEXT("Removed empty folder: %s"), *Removed));
        }
    }

    // ── Step 12: Completion ──────────────────────────────────────────────────
    FTimespan Duration = FDateTime::Now() - StartTime;
    Result.DurationSeconds = Duration.GetTotalSeconds();

    if (Result.bWasCancelled && Config.bAutoRollbackOnCancel && Result.MovedAssets > 0)
    {
        Log(TEXT("Auto-rollback requested after cancellation..."));
        // Note: Actual rollback would be handled by caller
    }

    Log(FString::Printf(TEXT("Organization complete. Moved: %d, Fixed: %d, Duration: %.2fs"),
        Result.MovedAssets, Result.FixedReferences, Result.DurationSeconds));

    if (ProgressCallback.IsBound())
        ProgressCallback.Execute(1.0f, LOCTEXT("Complete", "Organization complete"));

    return Result;
}

// ============================================================
// Dependency Sorting
// ============================================================

TArray<FAssetData> FAssetOrganizerExecutor::SortByDependency(const TArray<FAssetData>& Assets)
{
    if (Assets.Num() <= 1)
    {
        return Assets;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TMap<FName, int32> PackageToIndex;
    PackageToIndex.Reserve(Assets.Num());
    for (int32 i = 0; i < Assets.Num(); ++i)
    {
        PackageToIndex.Add(Assets[i].PackageName, i);
    }

    TArray<int32> InDegree;
    InDegree.Init(0, Assets.Num());

    TArray<TArray<int32>> Dependents;
    Dependents.SetNum(Assets.Num());

    for (int32 i = 0; i < Assets.Num(); ++i)
    {
        TArray<FName> Deps;
        AssetRegistry.GetDependencies(Assets[i].PackageName, Deps, UE::AssetRegistry::EDependencyCategory::Package);

        for (const FName& DepPackage : Deps)
        {
            int32* DepIdx = PackageToIndex.Find(DepPackage);
            if (DepIdx)
            {
                Dependents[*DepIdx].Add(i);
                InDegree[i]++;
            }
        }
    }

    TArray<int32> Queue;
    for (int32 i = 0; i < Assets.Num(); ++i)
    {
        if (InDegree[i] == 0)
        {
            Queue.Add(i);
        }
    }

    TArray<FAssetData> Sorted;
    Sorted.Reserve(Assets.Num());

    while (Queue.Num() > 0)
    {
        int32 Cur = Queue[0];
        Queue.RemoveAt(0);
        Sorted.Add(Assets[Cur]);

        for (int32 DepIdx : Dependents[Cur])
        {
            if (--InDegree[DepIdx] == 0)
            {
                Queue.Add(DepIdx);
            }
        }
    }

    // Handle circular dependencies - assets remaining in InDegree have circular dependencies
    if (Sorted.Num() < Assets.Num())
    {
        TArray<FString> CircularDepLogs;
        for (int32 i = 0; i < Assets.Num(); ++i)
        {
            if (InDegree[i] > 0)
            {
                // This asset has circular dependencies - log for debugging
                CircularDepLogs.Add(FString::Printf(TEXT("Circular dependency detected: %s (depends on %d remaining assets)"),
                    *Assets[i].AssetName.ToString(), InDegree[i]));

                // Add to sorted anyway to ensure all assets are processed
                Sorted.Add(Assets[i]);
            }
        }

        // Log circular dependency warnings if any
        if (CircularDepLogs.Num() > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("AssetOrganizer: %d assets with circular dependencies detected:"), CircularDepLogs.Num());
            for (const FString& Log : CircularDepLogs)
            {
                UE_LOG(LogTemp, Warning, TEXT("  %s"), *Log);
            }
        }
    }

    return Sorted;
}

bool FAssetOrganizerExecutor::DoesAssetDependOn(const FAssetData& AssetA, const FAssetData& AssetB)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FName> Dependencies;
    AssetRegistry.GetDependencies(AssetA.PackageName, Dependencies);
    return Dependencies.Contains(AssetB.PackageName);
}

// ============================================================
// Asset Moving
// ============================================================

bool FAssetOrganizerExecutor::MoveAsset(
    const FAssetData& Asset,
    const FString& NewPath,
    TArray<FString>& OutErrors,
    TArray<FString>& OutWarnings,
    FMoveRecord* OutRecord)
{
    if (Asset.AssetClass == FName(TEXT("ObjectRedirector")))
    {
        return true;
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Resolve naming conflicts
    FString BaseAssetName = Asset.AssetName.ToString();
    FString TargetAssetName = BaseAssetName;
    FString NewObjectPath = NewPath / TargetAssetName + TEXT(".") + TargetAssetName;

    int32 SuffixIndex = 1;
    bool bUsedGuid = false;

    while (true)
    {
        FAssetData ExistingAsset = AssetRegistry.GetAssetByObjectPath(FName(*NewObjectPath));

        if (!ExistingAsset.IsValid())
        {
            break;
        }

        if (ExistingAsset.ObjectPath == Asset.ObjectPath)
        {
            return true;
        }

        UObject* ExistingObject = ExistingAsset.GetAsset();
        bool bIsValidExisting = IsValid(ExistingObject) &&
            ExistingAsset.AssetClass != FName(TEXT("ObjectRedirector"));

        if (!bIsValidExisting)
        {
            break;
        }

        SuffixIndex++;
        TargetAssetName = FString::Printf(TEXT("%s_%d"), *BaseAssetName, SuffixIndex);
        NewObjectPath = NewPath / TargetAssetName + TEXT(".") + TargetAssetName;

        if (SuffixIndex > 999)
        {
            FGuid Guid = FGuid::NewGuid();
            TargetAssetName = FString::Printf(TEXT("%s_%s"), *BaseAssetName, *Guid.ToString(EGuidFormats::Digits).Left(8));
            NewObjectPath = NewPath / TargetAssetName + TEXT(".") + TargetAssetName;
            bUsedGuid = true;

            OutWarnings.Add(FString::Printf(TEXT("Naming conflict resolved with GUID suffix: %s -> %s"),
                *Asset.AssetName.ToString(), *TargetAssetName));
            break;
        }
    }

    UObject* AssetObject = Asset.GetAsset();
    if (!AssetObject)
    {
        // Skip assets that cannot be loaded (e.g., MapBuildDataRegistry like *_BuiltData)
        // These are internal engine assets that don't need to be moved
        FString AssetNameStr = Asset.AssetName.ToString();
        if (AssetNameStr.EndsWith(TEXT("_BuiltData")))
        {
            OutWarnings.Add(FString::Printf(TEXT("Skipped internal build data: %s"), *Asset.ObjectPath.ToString()));
            return true; // Not an error, just skip
        }
        OutErrors.Add(FString::Printf(TEXT("Could not load asset: %s"), *Asset.ObjectPath.ToString()));
        return false;
    }

    // Save before move
    UPackage* Package = AssetObject->GetOutermost();
    if (Package && Package->IsDirty())
    {
        TArray<UPackage*> PackagesToSave;
        PackagesToSave.Add(Package);
        FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
    }

    // Perform rename with retry logic for race condition protection
    const int32 MaxRetries = 3;
    int32 RetryCount = 0;
    bool bMoveSuccess = false;

    while (!bMoveSuccess && RetryCount < MaxRetries)
    {
        // Re-verify the target path is still available (race condition check)
        if (RetryCount > 0)
        {
            FAssetData RecheckAsset = AssetRegistry.GetAssetByObjectPath(FName(*NewObjectPath));
            if (RecheckAsset.IsValid() && RecheckAsset.ObjectPath != Asset.ObjectPath)
            {
                // Another process created an asset here - regenerate name
                SuffixIndex++;
                TargetAssetName = FString::Printf(TEXT("%s_%d"), *BaseAssetName, SuffixIndex);
                NewObjectPath = NewPath / TargetAssetName + TEXT(".") + TargetAssetName;
                RetryCount++;
                continue;
            }
        }

        TArray<FAssetRenameData> RenameData;
        RenameData.AddDefaulted();
        FAssetRenameData& Data = RenameData.Last();
        Data.Asset = AssetObject;
        Data.NewPackagePath = NewPath;
        Data.NewName = TargetAssetName;

        bMoveSuccess = AssetTools.RenameAssets(RenameData);

        if (!bMoveSuccess)
        {
            RetryCount++;
            if (RetryCount < MaxRetries)
            {
                // Wait a small amount before retry
                FPlatformProcess::Sleep(0.1f * RetryCount);
            }
        }
    }

    if (!bMoveSuccess)
    {
        OutErrors.Add(FString::Printf(TEXT("RenameAssets failed for: %s (tried %d times)"), *Asset.ObjectPath.ToString(), MaxRetries));
        return false;
    }

    // Save after move
    UPackage* NewPackage = AssetObject->GetOutermost();
    if (NewPackage)
    {
        TArray<UPackage*> PackagesToSave;
        PackagesToSave.Add(NewPackage);
        FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
    }

    // Record for history
    if (OutRecord)
    {
        *OutRecord = FMoveRecord(
            Asset.ObjectPath.ToString(),
            Asset.AssetName.ToString(),
            Asset.PackagePath.ToString(),
            NewPath,
            Asset.AssetClass
        );
    }

    return true;
}

// ============================================================
// Reference Fixing
// ============================================================

void FAssetOrganizerExecutor::FixUpAllReferences(const FString& RootPath, int32& OutFixedCount, const FOnLog& LogCallback)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.ClassNames.Add(FName(TEXT("ObjectRedirector")));
    Filter.PackagePaths.Add(FName(*RootPath));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> RedirectorAssets;
    AssetRegistry.GetAssets(Filter, RedirectorAssets);

    if (RedirectorAssets.Num() == 0)
    {
        return;
    }

    if (LogCallback.IsBound())
    {
        LogCallback.Execute(FString::Printf(TEXT("Found %d redirectors to fix"), RedirectorAssets.Num()));
    }

    TArray<UObjectRedirector*> Redirectors;
    Redirectors.Reserve(RedirectorAssets.Num());
    for (const FAssetData& AssetData : RedirectorAssets)
    {
        if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(AssetData.GetAsset()))
        {
            Redirectors.Add(Redirector);
        }
    }

    if (Redirectors.Num() == 0)
    {
        return;
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    AssetToolsModule.Get().FixupReferencers(Redirectors);

    OutFixedCount = Redirectors.Num();

    // ========== CRITICAL: Load all World assets to ensure their references are fixed ==========
    // When FixupReferencers updates references, World/Level assets need to be explicitly loaded
    // and marked dirty so the reference fixes are persisted when saved.
    // Without this step, levels may show missing references after organization.
    TArray<FAssetData> WorldAssets;
    FARFilter WorldFilter;
    WorldFilter.ClassNames.Add(FName("World"));
    WorldFilter.PackagePaths.Add(FName(*RootPath));
    WorldFilter.bRecursivePaths = true;
    AssetRegistry.GetAssets(WorldFilter, WorldAssets);

    int32 LoadedWorlds = 0;
    for (const FAssetData& WorldAsset : WorldAssets)
    {
        // Load the World asset - this ensures FixupReferencers can update its references
        if (UWorld* World = Cast<UWorld>(WorldAsset.GetAsset()))
        {
            // Mark the package dirty so it will be saved
            if (World->GetOutermost())
            {
                World->GetOutermost()->MarkPackageDirty();
                LoadedWorlds++;
            }
        }
    }

    if (LogCallback.IsBound() && LoadedWorlds > 0)
    {
        LogCallback.Execute(FString::Printf(TEXT("Loaded and marked %d levels for reference fixup"), LoadedWorlds));
    }

    // ========== CRITICAL: Save all dirty packages to persist reference fixes ==========
    // According to logo.txt: After fixing redirectors, must save to persist changes
    TArray<UPackage*> DirtyPackages;
    for (TObjectIterator<UPackage> PackageIt; PackageIt; ++PackageIt)
    {
        UPackage* Pkg = *PackageIt;
        if (Pkg && Pkg->IsDirty())
        {
            FString PkgName = Pkg->GetName();
            if (PkgName.StartsWith(RootPath))
            {
                DirtyPackages.Add(Pkg);
            }
        }
    }

    if (DirtyPackages.Num() > 0)
    {
        FEditorFileUtils::PromptForCheckoutAndSave(DirtyPackages, true, false);
    }
}

int32 FAssetOrganizerExecutor::FixBrokenReferences(
    const FString& RootPath,
    int32& OutFixedByRedirector,
    int32& OutTrulyMissing,
    TArray<FString>& OutDetailedLog)
{
    int32 FixedCount = 0;
    OutFixedByRedirector = 0;
    OutTrulyMissing = 0;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Find Materials and MaterialInstances
    FARFilter MaterialFilter;
    MaterialFilter.ClassNames.Add(FName("Material"));
    MaterialFilter.ClassNames.Add(FName("MaterialInstanceConstant"));
    MaterialFilter.PackagePaths.Add(FName(*RootPath));
    MaterialFilter.bRecursivePaths = true;

    TArray<FAssetData> Materials;
    AssetRegistry.GetAssets(MaterialFilter, Materials);

    // Check each material for broken references
    for (const FAssetData& MaterialData : Materials)
    {
        TArray<FName> Deps;
        AssetRegistry.GetDependencies(MaterialData.PackageName, Deps, UE::AssetRegistry::EDependencyCategory::Package);

        for (const FName& DepPackage : Deps)
        {
            // Check if dependency exists by looking in the package path
            // Note: Object path is not always "PackageName.PackageName"
            // We should check for any asset in the package
            TArray<FAssetData> DepAssets;
            FARFilter DepFilter;
            DepFilter.PackagePaths.Add(DepPackage);
            DepFilter.bRecursivePaths = false;
            AssetRegistry.GetAssets(DepFilter, DepAssets);

            // Filter out ObjectRedirectors - we care about actual assets
            DepAssets.RemoveAll([](const FAssetData& Asset)
            {
                return Asset.AssetClass == FName(TEXT("ObjectRedirector"));
            });

            if (DepAssets.Num() == 0)
            {
                // No valid assets found in this package - check for redirectors
                FARFilter RedirectorFilter;
                RedirectorFilter.PackagePaths.Add(DepPackage);
                RedirectorFilter.ClassNames.Add(FName(TEXT("ObjectRedirector")));
                TArray<FAssetData> Redirectors;
                AssetRegistry.GetAssets(RedirectorFilter, Redirectors);

                if (Redirectors.Num() > 0)
                {
                    OutFixedByRedirector++;
                    OutDetailedLog.Add(FString::Printf(TEXT("[Redirector Fixed] %s -> %s"),
                        *MaterialData.AssetName.ToString(), *DepPackage.ToString()));
                }
                else
                {
                    OutTrulyMissing++;
                    OutDetailedLog.Add(FString::Printf(TEXT("[Truly Missing] %s depends on %s"),
                        *MaterialData.AssetName.ToString(), *DepPackage.ToString()));
                }
                FixedCount++;
            }
        }
    }

    return FixedCount;
}

bool FAssetOrganizerExecutor::CheckAssetReferences(const FAssetData& Asset, TArray<FString>& OutBrokenRefs)
{
    bool bHasBrokenRefs = false;

    if (Asset.AssetClass == FName("Material") ||
        Asset.AssetClass == FName("MaterialInstanceConstant"))
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FName> Deps;
        AssetRegistry.GetDependencies(Asset.PackageName, Deps, UE::AssetRegistry::EDependencyCategory::Package);

        for (const FName& DepPackage : Deps)
        {
            FAssetData DepAsset = AssetRegistry.GetAssetByObjectPath(DepPackage);
            if (!DepAsset.IsValid())
            {
                TArray<FAssetData> DepAssets;
                FARFilter DepFilter;
                DepFilter.PackagePaths.Add(DepPackage);
                DepFilter.bRecursivePaths = false;
                AssetRegistry.GetAssets(DepFilter, DepAssets);

                if (DepAssets.Num() == 0)
                {
                    OutBrokenRefs.Add(FString::Printf(TEXT("Missing dependency: %s"), *DepPackage.ToString()));
                    bHasBrokenRefs = true;
                }
            }
        }
    }

    if (Asset.AssetClass == FName("StaticMesh"))
    {
        if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
        {
            const TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
            for (int32 i = 0; i < Materials.Num(); i++)
            {
                UMaterialInterface* Material = Mesh->GetMaterial(i);
                if (!Material)
                {
                    OutBrokenRefs.Add(FString::Printf(TEXT("Material slot %d is empty"), i));
                    bHasBrokenRefs = true;
                }
            }
        }
    }

    return bHasBrokenRefs;
}

// ============================================================
// Folder Management
// ============================================================

bool FAssetOrganizerExecutor::EnsureFolderExists(const FString& FolderPath)
{
    if (UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
    {
        return true;
    }
    return UEditorAssetLibrary::MakeDirectory(FolderPath);
}

int32 FAssetOrganizerExecutor::CleanEmptyFolders(const FString& RootPath, TArray<FString>* OutRemovedFolders)
{
    int32 RemovedCount = 0;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FString> SubPaths;
    AssetRegistry.GetSubPaths(RootPath, SubPaths, true);

    for (int32 i = SubPaths.Num() - 1; i >= 0; --i)
    {
        const FString& FolderPath = SubPaths[i];

        TArray<FAssetData> AssetsInFolder;
        FARFilter Filter;
        Filter.PackagePaths.Add(FName(*FolderPath));
        Filter.bRecursivePaths = false;
        AssetRegistry.GetAssets(Filter, AssetsInFolder);

        if (AssetsInFolder.Num() == 0)
        {
            TArray<FString> SubFolders;
            AssetRegistry.GetSubPaths(FolderPath, SubFolders, false);

            if (SubFolders.Num() == 0)
            {
                if (UEditorAssetLibrary::DeleteDirectory(FolderPath))
                {
                    RemovedCount++;
                    if (OutRemovedFolders)
                    {
                        OutRemovedFolders->Add(FolderPath);
                    }
                }
            }
        }
    }

    return RemovedCount;
}

// ============================================================
// History / Undo
// ============================================================

FString FAssetOrganizerExecutor::GetHistoryDirectory()
{
    FString HistoryDir = FPaths::ProjectSavedDir() / TEXT("AssetOrganizer");
    if (!FPaths::DirectoryExists(HistoryDir))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*HistoryDir);
    }
    return HistoryDir;
}

FString FAssetOrganizerExecutor::SaveHistory(const FOrganizeHistoryEntry& Entry)
{
    FString HistoryDir = GetHistoryDirectory();
    FString FileName = FString::Printf(TEXT("history_%s.json"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    FString FilePath = HistoryDir / FileName;

    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetStringField(TEXT("Id"), Entry.Id);
    RootObject->SetStringField(TEXT("Timestamp"), Entry.Timestamp);
    RootObject->SetStringField(TEXT("Description"), Entry.Description);
    RootObject->SetNumberField(TEXT("TotalAssets"), Entry.TotalAssets);
    RootObject->SetNumberField(TEXT("MovedAssets"), Entry.MovedAssets);
    RootObject->SetNumberField(TEXT("FixedReferences"), Entry.FixedReferences);
    RootObject->SetBoolField(TEXT("bCanRollback"), Entry.bCanRollback);

    TArray<TSharedPtr<FJsonValue>> RecordsArray;
    for (const FMoveRecord& Record : Entry.Records)
    {
        TSharedPtr<FJsonObject> RecordObj = MakeShareable(new FJsonObject);
        RecordObj->SetStringField(TEXT("AssetPath"), Record.AssetPath);
        RecordObj->SetStringField(TEXT("AssetName"), Record.AssetName);
        RecordObj->SetStringField(TEXT("OldPath"), Record.OldPath);
        RecordObj->SetStringField(TEXT("NewPath"), Record.NewPath);
        RecordObj->SetStringField(TEXT("AssetClass"), Record.AssetClass.ToString());
        RecordObj->SetStringField(TEXT("Timestamp"), Record.Timestamp);
        RecordsArray.Add(MakeShareable(new FJsonValueObject(RecordObj)));
    }
    RootObject->SetArrayField(TEXT("Records"), RecordsArray);

    // Save level references snapshot
    TArray<TSharedPtr<FJsonValue>> LevelRefsArray;
    for (const FLevelAssetReference& LevelRef : Entry.LevelReferences)
    {
        TSharedPtr<FJsonObject> LevelRefObj = MakeShareable(new FJsonObject);
        LevelRefObj->SetStringField(TEXT("LevelPath"), LevelRef.LevelPath);

        TArray<TSharedPtr<FJsonValue>> ReferencedAssetsArray;
        for (const FString& AssetPath : LevelRef.ReferencedAssets)
        {
            ReferencedAssetsArray.Add(MakeShareable(new FJsonValueString(AssetPath)));
        }
        LevelRefObj->SetArrayField(TEXT("ReferencedAssets"), ReferencedAssetsArray);

        LevelRefsArray.Add(MakeShareable(new FJsonValueObject(LevelRefObj)));
    }
    RootObject->SetArrayField(TEXT("LevelReferences"), LevelRefsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(OutputString, *FilePath);

    return FilePath;
}

bool FAssetOrganizerExecutor::LoadHistory(const FString& FilePath, FOrganizeHistoryEntry& OutEntry)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, RootObject))
    {
        return false;
    }

    // Use TryGet methods to safely extract fields (handles missing fields gracefully)
    if (!RootObject->TryGetStringField(TEXT("Id"), OutEntry.Id)) OutEntry.Id = FGuid::NewGuid().ToString();
    if (!RootObject->TryGetStringField(TEXT("Timestamp"), OutEntry.Timestamp)) OutEntry.Timestamp = FDateTime::Now().ToIso8601();
    if (!RootObject->TryGetStringField(TEXT("Description"), OutEntry.Description)) OutEntry.Description = TEXT("");
    if (!RootObject->TryGetNumberField(TEXT("TotalAssets"), OutEntry.TotalAssets)) OutEntry.TotalAssets = 0;
    if (!RootObject->TryGetNumberField(TEXT("MovedAssets"), OutEntry.MovedAssets)) OutEntry.MovedAssets = 0;
    if (!RootObject->TryGetNumberField(TEXT("FixedReferences"), OutEntry.FixedReferences)) OutEntry.FixedReferences = 0;
    if (!RootObject->TryGetBoolField(TEXT("bCanRollback"), OutEntry.bCanRollback)) OutEntry.bCanRollback = false;

    const TArray<TSharedPtr<FJsonValue>>* RecordsArray;
    if (RootObject->TryGetArrayField(TEXT("Records"), RecordsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *RecordsArray)
        {
            TSharedPtr<FJsonObject> RecordObj = Value->AsObject();
            if (RecordObj.IsValid())
            {
                FMoveRecord Record;
                RecordObj->TryGetStringField(TEXT("AssetPath"), Record.AssetPath);
                RecordObj->TryGetStringField(TEXT("AssetName"), Record.AssetName);
                RecordObj->TryGetStringField(TEXT("OldPath"), Record.OldPath);
                RecordObj->TryGetStringField(TEXT("NewPath"), Record.NewPath);

                FString AssetClassStr;
                if (RecordObj->TryGetStringField(TEXT("AssetClass"), AssetClassStr))
                {
                    Record.AssetClass = FName(*AssetClassStr);
                }

                RecordObj->TryGetStringField(TEXT("Timestamp"), Record.Timestamp);
                OutEntry.Records.Add(Record);
            }
        }
    }

    // Load level references snapshot
    const TArray<TSharedPtr<FJsonValue>>* LevelRefsArray;
    if (RootObject->TryGetArrayField(TEXT("LevelReferences"), LevelRefsArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *LevelRefsArray)
        {
            TSharedPtr<FJsonObject> LevelRefObj = Value->AsObject();
            if (LevelRefObj.IsValid())
            {
                FLevelAssetReference LevelRef;
                LevelRefObj->TryGetStringField(TEXT("LevelPath"), LevelRef.LevelPath);

                const TArray<TSharedPtr<FJsonValue>>* ReferencedAssetsArray;
                if (LevelRefObj->TryGetArrayField(TEXT("ReferencedAssets"), ReferencedAssetsArray))
                {
                    for (const TSharedPtr<FJsonValue>& AssetValue : *ReferencedAssetsArray)
                    {
                        if (AssetValue.IsValid())
                        {
                            LevelRef.ReferencedAssets.Add(AssetValue->AsString());
                        }
                    }
                }

                OutEntry.LevelReferences.Add(LevelRef);
            }
        }
    }

    return true;
}

TArray<FString> FAssetOrganizerExecutor::GetHistoryFiles(int32 MaxCount)
{
    TArray<FString> HistoryFiles;
    FString HistoryDir = GetHistoryDirectory();

    if (!FPaths::DirectoryExists(HistoryDir))
    {
        return HistoryFiles;
    }

    TArray<FString> AllFiles;
    FPlatformFileManager::Get().GetPlatformFile().FindFiles(AllFiles, *HistoryDir, TEXT(".json"));

    // Sort by filename (which includes timestamp) in descending order
    AllFiles.Sort([](const FString& A, const FString& B) { return A > B; });

    for (int32 i = 0; i < FMath::Min(MaxCount, AllFiles.Num()); ++i)
    {
        HistoryFiles.Add(AllFiles[i]);
    }

    return HistoryFiles;
}

FOrganizeResult FAssetOrganizerExecutor::RollbackOrganize(const FString& HistoryFilePath, const FOnProgress& ProgressCallback, const FOnLog& LogCallback)
{
    FOrganizeResult Result;

    FOrganizeHistoryEntry Entry;
    if (!LoadHistory(HistoryFilePath, Entry))
    {
        Result.Errors.Add(TEXT("Failed to load history file"));
        return Result;
    }

    if (!Entry.bCanRollback)
    {
        Result.Errors.Add(TEXT("This operation cannot be rolled back"));
        return Result;
    }

    FScopedSlowTask SlowTask((float)Entry.Records.Num(), LOCTEXT("RollingBack", "Rolling back organization..."));
    SlowTask.MakeDialog();

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    int32 ProcessedCount = 0;

    // Process in reverse order
    for (int32 i = Entry.Records.Num() - 1; i >= 0; --i)
    {
        const FMoveRecord& Record = Entry.Records[i];

        SlowTask.EnterProgressFrame(1.0f, FText::Format(
            LOCTEXT("RollingBackAsset", "Rolling back: {0}"),
            FText::FromString(Record.AssetName)
        ));

        // Construct the current path (where the asset was moved to)
        // Record.NewPath is the destination folder, AssetName is the asset name
        // Note: Asset might have been renamed due to naming conflicts during organization
        FString CurrentObjectPath = Record.NewPath / Record.AssetName + TEXT(".") + Record.AssetName;

        // Check if asset exists at new location
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FName(*CurrentObjectPath));

        // If not found with exact name, try to find by package path (handles naming conflicts)
        if (!AssetData.IsValid())
        {
            TArray<FAssetData> AssetsInPackage;
            FARFilter Filter;
            Filter.PackagePaths.Add(FName(*Record.NewPath));
            Filter.bRecursivePaths = false;
            AssetRegistry.GetAssets(Filter, AssetsInPackage);

            // Look for an asset with similar name (original name might have suffix)
            for (const FAssetData& Candidate : AssetsInPackage)
            {
                if (Candidate.AssetClass == Record.AssetClass &&
                    Candidate.AssetName.ToString().StartsWith(Record.AssetName))
                {
                    AssetData = Candidate;
                    if (LogCallback.IsBound())
                    {
                        LogCallback.Execute(FString::Printf(TEXT("Found asset with different name: %s -> %s"),
                            *Record.AssetName, *Candidate.AssetName.ToString()));
                    }
                    break;
                }
            }
        }

        if (!AssetData.IsValid())
        {
            Result.Warnings.Add(FString::Printf(TEXT("Asset not found at new location: %s (looked for: %s)"), *Record.AssetName, *CurrentObjectPath));
            continue;
        }

        UObject* AssetObject = AssetData.GetAsset();
        if (!AssetObject)
        {
            Result.Errors.Add(FString::Printf(TEXT("Could not load asset: %s"), *Record.AssetName));
            continue;
        }

        // Move back to old path
        TArray<FAssetRenameData> RenameData;
        RenameData.AddDefaulted();
        FAssetRenameData& Data = RenameData.Last();
        Data.Asset = AssetObject;
        Data.NewPackagePath = Record.OldPath;
        Data.NewName = Record.AssetName;

        if (AssetTools.RenameAssets(RenameData))
        {
            Result.MovedAssets++;
        }
        else
        {
            Result.Errors.Add(FString::Printf(TEXT("Failed to rollback: %s"), *Record.AssetName));
        }

        ProcessedCount++;
        if (ProgressCallback.IsBound())
        {
            float Pct = (float)ProcessedCount / (float)Entry.Records.Num();
            ProgressCallback.Execute(Pct, FText::Format(
                LOCTEXT("RollbackProgress", "Rolling back: {0}/{1}"),
                ProcessedCount, Entry.Records.Num()
            ));
        }
    }

    Entry.bCanRollback = false;
    SaveHistory(Entry);

    // ========== CRITICAL: Delete stale ObjectRedirectors after rollback ==========
    // After rollback, assets are back at their original locations (OldPath).
    // But ObjectRedirectors at OldPath still point to NewPath (which is now empty).
    // These stale redirectors cause "asset not found" errors when loading levels.
    if (Result.MovedAssets > 0 && Entry.Records.Num() > 0)
    {
        SlowTask.EnterProgressFrame(0.0f, LOCTEXT("CleaningRedirectors", "Cleaning up stale redirectors..."));

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        int32 DeletedRedirectors = 0;
        TArray<FString> FailedDeletions;

        for (const FMoveRecord& Record : Entry.Records)
        {
            // Construct the original path where the redirector might be
            FString OldPackagePath = Record.OldPath / Record.AssetName;
            FName OldPackageFName(*OldPackagePath);

            // Check if there's an ObjectRedirector at the old path
            FARFilter RedirectorFilter;
            RedirectorFilter.PackagePaths.Add(OldPackageFName);
            RedirectorFilter.ClassNames.Add(FName(TEXT("ObjectRedirector")));
            RedirectorFilter.bRecursivePaths = false;

            TArray<FAssetData> Redirectors;
            AssetRegistry.GetAssets(RedirectorFilter, Redirectors);

            for (const FAssetData& RedirectorData : Redirectors)
            {
                // Delete the redirector - asset is now back at original location
                if (UEditorAssetLibrary::DoesAssetExist(RedirectorData.ObjectPath.ToString()))
                {
                    if (UEditorAssetLibrary::DeleteAsset(RedirectorData.ObjectPath.ToString()))
                    {
                        DeletedRedirectors++;
                        if (LogCallback.IsBound())
                        {
                            LogCallback.Execute(FString::Printf(TEXT("Deleted stale redirector: %s"),
                                *RedirectorData.ObjectPath.ToString()));
                        }
                    }
                    else
                    {
                        FailedDeletions.Add(RedirectorData.ObjectPath.ToString());
                    }
                }
            }
        }

        if (LogCallback.IsBound())
        {
            LogCallback.Execute(FString::Printf(TEXT("Deleted %d stale redirectors"), DeletedRedirectors));
        }
    }

    // ========== CRITICAL: Restore level references after rollback ==========
    // After assets are moved back, we need to restore the original references
    // in levels. This prevents models from disappearing when opening levels.
    if (Result.MovedAssets > 0 && Entry.LevelReferences.Num() > 0)
    {
        SlowTask.EnterProgressFrame(0.0f, LOCTEXT("RestoringRefs", "Restoring level references..."));

        RestoreLevelReferencesAfterRollback(Entry.LevelReferences, LogCallback);
    }

    // ========== CRITICAL: Fix up references after rollback ==========
    // When assets are moved back to their original locations, all referencers
    // (levels, blueprints, materials, etc.) need to be updated to point to
    // the old paths. Without this step, models will appear missing in levels.
    if (Result.MovedAssets > 0 && Entry.Records.Num() > 0)
    {
        SlowTask.EnterProgressFrame(0.0f, LOCTEXT("FixingReferences", "Fixing up references after rollback..."));

        // Use /Game as root path to search for ALL redirectors in the project
        // This ensures we catch redirectors created in any folder during rollback
        FString RootPath = TEXT("/Game");

        int32 FixedCount = 0;
        FixUpAllReferences(RootPath, FixedCount, LogCallback);
        Result.FixedReferences = FixedCount;

        if (LogCallback.IsBound())
        {
            LogCallback.Execute(FString::Printf(TEXT("Rollback fixed %d references"), FixedCount));
        }
    }

    // ========== CRITICAL: Clean up empty folders created during organization ==========
    // After rollback, the target folders created during organization are now empty.
    // We need to remove these empty folders to restore the project to its pre-organization state.
    {
        SlowTask.EnterProgressFrame(0.0f, LOCTEXT("CleaningEmptyFolders", "Cleaning up empty folders..."));

        // Collect all target paths that were used during organization
        TSet<FString> TargetPathsUsed;
        for (const FMoveRecord& Record : Entry.Records)
        {
            TargetPathsUsed.Add(Record.NewPath);
        }

        // Try to clean each unique target folder
        TArray<FString> RemovedFolders;
        for (const FString& TargetPath : TargetPathsUsed)
        {
            // Clean empty folders under each target path
            // Use recursive cleaning to handle nested folder structures
            int32 Removed = CleanEmptyFolders(TargetPath, &RemovedFolders);
            Result.RemovedEmptyFolders += Removed;
        }

        // Also clean the root path to catch any empty parent folders
        Result.RemovedEmptyFolders += CleanEmptyFolders(TEXT("/Game"), &RemovedFolders);

        if (LogCallback.IsBound() && RemovedFolders.Num() > 0)
        {
            LogCallback.Execute(FString::Printf(TEXT("Removed %d empty folders after rollback"), RemovedFolders.Num()));
            for (const FString& Folder : RemovedFolders)
            {
                LogCallback.Execute(FString::Printf(TEXT("  - Removed: %s"), *Folder));
            }
        }
    }

    return Result;
}

bool FAssetOrganizerExecutor::DeleteHistory(const FString& FilePath)
{
    return FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*FilePath);
}

int32 FAssetOrganizerExecutor::ClearAllHistory(TArray<FString>* OutDeletedFiles)
{
    int32 DeletedCount = 0;
    TArray<FString> AllFiles = GetHistoryFiles(1000); // Get all history files

    for (const FString& FilePath : AllFiles)
    {
        if (DeleteHistory(FilePath))
        {
            DeletedCount++;
            if (OutDeletedFiles)
            {
                OutDeletedFiles->Add(FilePath);
            }
        }
    }

    return DeletedCount;
}

// ============================================================
// Preview Generation
// ============================================================

TArray<FMovePreviewItem> FAssetOrganizerExecutor::GeneratePreview(
    const TArray<FAssetData>& Assets,
    const FOrganizeConfig& Config,
    const TArray<FCustomAssetRule>* CustomRules)
{
    TArray<FMovePreviewItem> PreviewItems;

    TMap<EAssetOrganizeType, TArray<FAssetData>> Grouped = GroupAssetsByType(Assets, CustomRules);

    for (const auto& Pair : Grouped)
    {
        EAssetOrganizeType Type = Pair.Key;
        const TArray<FAssetData>& TypeAssets = Pair.Value;
        FString TargetFolder = GetTargetFolder(Type, Config);

        for (const FAssetData& Asset : TypeAssets)
        {
            FString CurrentPath = Asset.PackagePath.ToString();
            bool bWillConflict = (CurrentPath != TargetFolder) &&
                !Asset.AssetClass.ToString().Equals(TEXT("ObjectRedirector"));

            FMovePreviewItem PreviewItem(
                Asset.AssetName.ToString(),
                Asset.AssetClass.ToString(),
                CurrentPath,
                TargetFolder,
                bWillConflict
            );

            PreviewItem.bIsNewFolder = !UEditorAssetLibrary::DoesDirectoryExist(TargetFolder);

            PreviewItems.Add(PreviewItem);
        }
    }

    return PreviewItems;
}

// ============================================================
// Async Execution
// ============================================================

TSharedPtr<FAsyncTask<FOrganizeAssetsTask>> FAssetOrganizerExecutor::OrganizeAssetsAsync(
    const FOrganizeConfig& Config,
    const FOnProgress& ProgressCallback,
    const FOnLog& LogCallback,
    const FOnCancelled& CancelledCallback)
{
    FScopeLock Lock(&AsyncLock);

    if (CurrentAsyncTask.IsValid() && !CurrentAsyncTask->IsDone())
    {
        return nullptr;
    }

    bCancelRequested = false;

    CurrentAsyncTask = MakeShared<FAsyncTask<FOrganizeAssetsTask>>(
        Config, ProgressCallback, LogCallback);

    // IMPORTANT: Asset operations must run on GameThread because GetAsset()
    // and UObject operations are not thread-safe. Using StartSynchronousTask()
    // ensures execution on the calling thread (expected to be GameThread).
    CurrentAsyncTask->StartSynchronousTask();

    return CurrentAsyncTask;
}

void FAssetOrganizerExecutor::CancelAsyncOperation()
{
    bCancelRequested = true;
}

bool FAssetOrganizerExecutor::IsAsyncOperationRunning()
{
    FScopeLock Lock(&AsyncLock);
    return CurrentAsyncTask.IsValid() && !CurrentAsyncTask->IsDone();
}

TSharedPtr<FAsyncTask<FOrganizeAssetsTask>> FAssetOrganizerExecutor::GetCurrentAsyncTask()
{
    FScopeLock Lock(&AsyncLock);
    return CurrentAsyncTask;
}

// ============================================================
// Async Task Implementation
// ============================================================

FOrganizeAssetsTask::FOrganizeAssetsTask(
    const FOrganizeConfig& InConfig,
    const FAssetOrganizerExecutor::FOnProgress& InProgressCallback,
    const FAssetOrganizerExecutor::FOnLog& InLogCallback)
    : Config(InConfig)
    , ProgressCallback(InProgressCallback)
    , LogCallback(InLogCallback)
{
}

void FOrganizeAssetsTask::DoWork()
{
    Result = FAssetOrganizerExecutor::OrganizeAssets(Config, ProgressCallback, LogCallback);
}

// ============================================================
// Level Reference Snapshot (Critical for Rollback)
// ============================================================

TArray<FLevelAssetReference> FAssetOrganizerExecutor::SaveLevelReferencesSnapshot(
    const TArray<FAssetData>& AssetsToMove,
    const FOnLog& LogCallback)
{
    TArray<FLevelAssetReference> Snapshots;

    auto Log = [&](const FString& Message)
    {
        if (LogCallback.IsBound())
        {
            LogCallback.Execute(Message);
        }
    };

    // Get all levels in the project
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Build a set of asset paths that will be moved (for quick lookup)
    TSet<FString> AssetsToMovePaths;
    for (const FAssetData& Asset : AssetsToMove)
    {
        AssetsToMovePaths.Add(Asset.ObjectPath.ToString());
        // Also add the package path for matching
        AssetsToMovePaths.Add(Asset.PackageName.ToString());
    }

    // Find all World/Level assets
    FARFilter LevelFilter;
    LevelFilter.ClassNames.Add(FName("World"));
    LevelFilter.bRecursivePaths = true;

    TArray<FAssetData> Levels;
    AssetRegistry.GetAssets(LevelFilter, Levels);

    Log(FString::Printf(TEXT("Found %d levels to scan for references"), Levels.Num()));

    // For each level, find which of our target assets it references
    for (const FAssetData& LevelData : Levels)
    {
        TArray<FName> Dependencies;
        AssetRegistry.GetDependencies(LevelData.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);

        TArray<FString> ReferencedAssets;
        for (const FName& DepPackage : Dependencies)
        {
            FString DepPackageStr = DepPackage.ToString();
            // Check if this dependency is one of the assets we're moving
            if (AssetsToMovePaths.Contains(DepPackageStr))
            {
                ReferencedAssets.Add(DepPackageStr);
            }
        }

        if (ReferencedAssets.Num() > 0)
        {
            FLevelAssetReference Snapshot(LevelData.ObjectPath.ToString());
            Snapshot.ReferencedAssets = ReferencedAssets;
            Snapshots.Add(Snapshot);

            Log(FString::Printf(TEXT("Level %s references %d assets that will be moved"),
                *LevelData.AssetName.ToString(), ReferencedAssets.Num()));
        }
    }

    Log(FString::Printf(TEXT("Saved reference snapshots for %d levels"), Snapshots.Num()));
    return Snapshots;
}

void FAssetOrganizerExecutor::RestoreLevelReferencesAfterRollback(
    const TArray<FLevelAssetReference>& LevelSnapshots,
    const FOnLog& LogCallback)
{
    auto Log = [&](const FString& Message)
    {
        if (LogCallback.IsBound())
        {
            LogCallback.Execute(Message);
        }
    };

    if (LevelSnapshots.Num() == 0)
    {
        return;
    }

    Log(FString::Printf(TEXT("Restoring references for %d levels..."), LevelSnapshots.Num()));

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    int32 FixedLevels = 0;
    int32 TotalFixedRefs = 0;

    // Build a map of old (organized) paths to current (original) paths
    // After rollback, assets are back at their original locations
    // But levels might still have stale references pointing to the organized locations
    TMap<UObject*, UObject*> ReplacementMap;
    TArray<UObject*> ObjectsToCheck;

    // Load each level that had references to our moved assets
    for (const FLevelAssetReference& Snapshot : LevelSnapshots)
    {
        // Load the level package
        UWorld* World = LoadObject<UWorld>(nullptr, *Snapshot.LevelPath);
        if (!World)
        {
            Log(FString::Printf(TEXT("Failed to load level: %s"), *Snapshot.LevelPath));
            continue;
        }

        // Collect all objects in the level that might need reference fixing
        TArray<UObject*> LevelObjects;
        for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
        {
            AActor* Actor = *ActorIt;
            if (!Actor)
            {
                continue;
            }

            LevelObjects.Add(Actor);

            // Collect all components
            TArray<UActorComponent*> Components;
            Actor->GetComponents(Components);
            for (UActorComponent* Component : Components)
            {
                if (Component)
                {
                    LevelObjects.Add(Component);
                }
            }
        }

        bool bLevelModified = false;

        // For each referenced asset, check if we need to fix references
        for (const FString& AssetPath : Snapshot.ReferencedAssets)
        {
            // Get the current asset data (should be at original location after rollback)
            FAssetData CurrentAssetData = AssetRegistry.GetAssetByObjectPath(FName(*AssetPath));

            if (!CurrentAssetData.IsValid())
            {
                // Try to find the asset by package name (without object name)
                int32 DotIndex = AssetPath.Find(TEXT("."));
                FString PackagePath = (DotIndex != INDEX_NONE) ? AssetPath.Left(DotIndex) : AssetPath;

                TArray<FAssetData> AssetsInPackage;
                FARFilter Filter;
                Filter.PackagePaths.Add(FName(*PackagePath));
                Filter.bRecursivePaths = false;
                AssetRegistry.GetAssets(Filter, AssetsInPackage);

                if (AssetsInPackage.Num() > 0)
                {
                    CurrentAssetData = AssetsInPackage[0];
                }
            }

            if (CurrentAssetData.IsValid())
            {
                UObject* AssetObject = CurrentAssetData.GetAsset();
                if (AssetObject)
                {
                    // Find and fix references to this asset in the level
                    for (UObject* LevelObject : LevelObjects)
                    {
                        if (!LevelObject)
                        {
                            continue;
                        }

                        // Check all properties for references that need fixing
                        for (TFieldIterator<FProperty> PropIt(LevelObject->GetClass()); PropIt; ++PropIt)
                        {
                            FProperty* Property = *PropIt;
                            if (!Property)
                            {
                                continue;
                            }

                            // Check object properties
                            if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
                            {
                                UObject* ObjValue = ObjectProp->GetPropertyValue_InContainer(LevelObject);
                                if (ObjValue && ObjValue->GetClass()->IsChildOf(UObjectRedirector::StaticClass()))
                                {
                                    // Found a redirector reference - replace with actual asset
                                    ObjectProp->SetPropertyValue_InContainer(LevelObject, AssetObject);
                                    bLevelModified = true;
                                    TotalFixedRefs++;
                                }
                            }
                            // Check array properties
                            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
                            {
                                if (FObjectProperty* InnerObjProp = CastField<FObjectProperty>(ArrayProp->Inner))
                                {
                                    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(LevelObject));
                                    for (int32 i = 0; i < ArrayHelper.Num(); i++)
                                    {
                                        UObject* ObjValue = InnerObjProp->GetPropertyValue(ArrayHelper.GetRawPtr(i));
                                        if (ObjValue && ObjValue->GetClass()->IsChildOf(UObjectRedirector::StaticClass()))
                                        {
                                            InnerObjProp->SetPropertyValue(ArrayHelper.GetRawPtr(i), AssetObject);
                                            bLevelModified = true;
                                            TotalFixedRefs++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                Log(FString::Printf(TEXT("Warning: Could not find asset at %s after rollback"), *AssetPath));
            }
        }

        if (bLevelModified)
        {
            // Mark the level package as dirty so it will be saved with fixed references
            World->MarkPackageDirty();
            FixedLevels++;
        }
    }

    // Now save all dirty packages
    TArray<UPackage*> DirtyPackages;
    for (TObjectIterator<UPackage> PackageIt; PackageIt; ++PackageIt)
    {
        UPackage* Pkg = *PackageIt;
        if (Pkg && Pkg->IsDirty() && Pkg->GetName().StartsWith(TEXT("/Game/")))
        {
            DirtyPackages.Add(Pkg);
        }
    }

    if (DirtyPackages.Num() > 0)
    {
        FEditorFileUtils::PromptForCheckoutAndSave(DirtyPackages, true, false);
        Log(FString::Printf(TEXT("Saved %d levels with restored references"), DirtyPackages.Num()));
    }

    // Force asset registry refresh to pick up changes
    AssetRegistry.ScanPathsSynchronous({TEXT("/Game")});

    Log(FString::Printf(TEXT("Reference restoration complete. Fixed %d levels, %d references."), FixedLevels, TotalFixedRefs));
}


// ============================================================
// Whitelist Check
// ============================================================

bool FAssetOrganizerExecutor::IsAssetWhitelisted(const FAssetData& Asset, const FOrganizeConfig& Config)
{
    if (Config.WhitelistedFolders.Num() == 0)
        return false;

    FString PackagePath = Asset.PackagePath.ToString();
    for (const FString& WL : Config.WhitelistedFolders)
    {
        if (!WL.IsEmpty() && PackagePath.StartsWith(WL))
            return true;
    }
    return false;
}

// ============================================================
// Post-Organize Reference Verification
// ============================================================

void FAssetOrganizerExecutor::VerifyReferences(
    const TArray<FMovedAssetRecord>& MovedAssets,
    FOrganizeReport& OutReport)
{
    FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = ARModule.Get();

    // Force sync so registry reflects post-move state
    AssetRegistry.SearchAllAssets(/*bSynchronousSearch=*/true);

    for (const FMovedAssetRecord& Record : MovedAssets)
    {
        FName PackageName(*FPackageName::ObjectPathToPackageName(Record.DestinationPath));

        TArray<FName> Dependencies;
        AssetRegistry.GetDependencies(PackageName, Dependencies,
            UE::AssetRegistry::EDependencyCategory::Package);

        for (const FName& Dep : Dependencies)
        {
            FAssetData DepAsset = AssetRegistry.GetAssetByObjectPath(Dep);
            if (!DepAsset.IsValid())
            {
                FBrokenReferenceRecord Broken;
                Broken.AssetPath        = Record.DestinationPath;
                Broken.BrokenDependency = Dep.ToString();
                Broken.bAutoFixed       = false;
                OutReport.BrokenReferences.Add(Broken);
                OutReport.BrokenRefCount++;
            }
        }
    }

    // Attempt auto-fix if configured
    const UAssetOrganizerSettings* Settings = GetDefault<UAssetOrganizerSettings>();
    if (Settings && Settings->bAutoFixBrokenReferences && OutReport.BrokenRefCount > 0)
    {
        AttemptAutoFixBrokenReferences(OutReport);
    }
}

void FAssetOrganizerExecutor::AttemptAutoFixBrokenReferences(FOrganizeReport& OutReport)
{
    FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = ARModule.Get();

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Collect all redirectors under /Game
    TArray<FAssetData> RedirectorAssets;
    FARFilter Filter;
    Filter.ClassNames.Add(FName(TEXT("ObjectRedirector")));
    Filter.PackagePaths.Add(FName(TEXT("/Game")));
    Filter.bRecursivePaths = true;
    AssetRegistry.GetAssets(Filter, RedirectorAssets);

    TArray<UObjectRedirector*> Redirectors;
    for (const FAssetData& RedirData : RedirectorAssets)
    {
        if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(RedirData.GetAsset()))
        {
            Redirectors.Add(Redirector);
        }
    }

    if (Redirectors.Num() > 0)
    {
        AssetTools.FixupReferencers(Redirectors);
    }

    // Re-check which broken refs are now resolved
    for (FBrokenReferenceRecord& Record : OutReport.BrokenReferences)
    {
        if (!Record.bAutoFixed)
        {
            FAssetData Check = AssetRegistry.GetAssetByObjectPath(FName(*Record.BrokenDependency));
            if (Check.IsValid())
            {
                Record.bAutoFixed = true;
                OutReport.BrokenRefCount = FMath::Max(0, OutReport.BrokenRefCount - 1);
            }
        }
    }
}

// ============================================================
// Update Whitelisted Folder References
// ============================================================

void FAssetOrganizerExecutor::UpdateWhitelistedFolderReferences(
    const FOrganizeConfig& Config,
    const FOrganizeReport& Report,
    const FOnLog& LogCallback)
{
    if (!Config.bUpdateWhitelistedAssetReferences)
        return;
    if (Config.WhitelistedFolders.Num() == 0)
        return;
    if (Report.MovedAssets.Num() == 0)
        return;

    FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = ARModule.Get();

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Build a set of original paths that were moved
    TSet<FString> MovedOriginalPaths;
    for (const FMovedAssetRecord& Rec : Report.MovedAssets)
    {
        MovedOriginalPaths.Add(Rec.OriginalPath);
    }

    TArray<UObjectRedirector*> RedirectorsToFix;

    for (const FString& WLFolder : Config.WhitelistedFolders)
    {
        TArray<FAssetData> WLAssets;
        ARModule.Get().GetAssetsByPath(FName(*WLFolder), WLAssets, /*bRecursive=*/true);

        for (const FAssetData& Asset : WLAssets)
        {
            TArray<FName> Deps;
            AssetRegistry.GetDependencies(Asset.PackageName, Deps,
                UE::AssetRegistry::EDependencyCategory::Package);

            for (const FName& Dep : Deps)
            {
                FString DepStr = Dep.ToString();
                if (MovedOriginalPaths.Contains(DepStr))
                {
                    // Load redirector at original path and collect for fixup
                    if (UObjectRedirector* Redir = LoadObject<UObjectRedirector>(nullptr, *DepStr))
                    {
                        RedirectorsToFix.AddUnique(Redir);
                    }
                }
            }
        }
    }

    if (RedirectorsToFix.Num() > 0)
    {
        AssetTools.FixupReferencers(RedirectorsToFix);
        if (LogCallback.IsBound())
        {
            LogCallback.Execute(FString::Printf(
                TEXT("Fixed %d redirector(s) for whitelisted folder assets"),
                RedirectorsToFix.Num()));
        }
    }
}

#undef LOCTEXT_NAMESPACE
