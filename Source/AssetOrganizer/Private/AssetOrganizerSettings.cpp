// Copyright UEMaster. All Rights Reserved.

#include "AssetOrganizerSettings.h"

#define LOCTEXT_NAMESPACE "AssetOrganizer"

FOnCustomRulesChanged UAssetOrganizerSettings::OnCustomRulesChanged;

FName UAssetOrganizerSettings::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}

FName UAssetOrganizerSettings::GetSectionName() const
{
    return FName(TEXT("Asset Organizer"));
}

UAssetOrganizerSettings::UAssetOrganizerSettings()
    : RootPath(TEXT("/Game"))
    , bCreateFolders(true)
    , bFixReferences(true)
    , bCleanEmptyFolders(true)
    , FolderStrategy(EFolderCreationStrategy::LazyCreate)
    , bRunVerificationAfterOrganize(true)
    , bAutoFixBrokenReferences(false)
    , MaxHistoryEntries(20)
    , bEnableHistory(true)
    , bAutoSaveHistory(true)
{
    // Initialize default asset types if empty
    if (AssetTypeConfigs.Num() == 0)
    {
        InitializeDefaultAssetTypes();
    }
}

void UAssetOrganizerSettings::PostInitProperties()
{
    Super::PostInitProperties();

    // Ensure asset types are initialized
    if (AssetTypeConfigs.Num() == 0)
    {
        InitializeDefaultAssetTypes();
    }

    MigrateFromLegacyExcludedFolders();
}

void UAssetOrganizerSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SaveConfig();

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UAssetOrganizerSettings, CustomRules))
    {
        OnCustomRulesChanged.Broadcast();
    }
}

void UAssetOrganizerSettings::ResetToDefault()
{
    RootPath = TEXT("/Game");
    bCreateFolders = true;
    bFixReferences = true;
    bCleanEmptyFolders = true;
    FolderStrategy = EFolderCreationStrategy::LazyCreate;
    RootFolderConfig = FRootFolderConfig();
    ExcludedFolders.Empty();
    WhitelistedFolders.Empty();
    bRunVerificationAfterOrganize = true;
    bAutoFixBrokenReferences = false;
    MaxHistoryEntries = 20;
    bEnableHistory = true;
    bAutoSaveHistory = true;
    CustomRules.Empty();

    InitializeDefaultAssetTypes();
    SaveConfig();
}

void UAssetOrganizerSettings::InitializeDefaultAssetTypes()
{
    AssetTypeConfigs.Empty();

    // =====================================================
    // MATERIALS (7 types) - Priority 10
    // NOTE: Materials must be moved BEFORE Meshes because Meshes reference Materials
    // Following the "Inside-Out" principle: Textures -> Materials -> Meshes -> Blueprints
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::Material, FName("Material"), TEXT("M_"),
        TEXT("/Game/Materials"), FName("Materials"),
        LOCTEXT("Material", "Material"), 10));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MaterialInstanceConstant, FName("MaterialInstanceConstant"), TEXT("MI_"),
        TEXT("/Game/Materials/Instances"), FName("Materials"),
        LOCTEXT("MaterialInstance", "Material Instance"), 11));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MaterialInstanceDynamic, FName("MaterialInstanceDynamic"), TEXT("MID_"),
        TEXT("/Game/Materials/Dynamic"), FName("Materials"),
        LOCTEXT("MaterialInstanceDynamic", "Material Instance Dynamic"), 11));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MaterialFunction, FName("MaterialFunction"), TEXT("MF_"),
        TEXT("/Game/Materials/Functions"), FName("Materials"),
        LOCTEXT("MaterialFunction", "Material Function"), 12));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MaterialParameterCollection, FName("MaterialParameterCollection"), TEXT("MPC_"),
        TEXT("/Game/Materials/ParameterCollections"), FName("Materials"),
        LOCTEXT("MaterialParameterCollection", "Material Parameter Collection"), 13));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SubsurfaceProfile, FName("SubsurfaceProfile"), TEXT("SP_"),
        TEXT("/Game/Materials"), FName("Materials"),
        LOCTEXT("SubsurfaceProfile", "Subsurface Profile"), 14));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::PhysicalMaterial, FName("PhysicalMaterial"), TEXT("PM_"),
        TEXT("/Game/Physics/Materials"), FName("Materials"),
        LOCTEXT("PhysicalMaterial", "Physical Material"), 15));

    // =====================================================
    // MESHES (5 types) - Priority 20
    // NOTE: Meshes must be moved AFTER Materials because they reference Materials
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::StaticMesh, FName("StaticMesh"), TEXT("SM_"),
        TEXT("/Game/Models/StaticMeshes"), FName("Meshes"),
        LOCTEXT("StaticMesh", "Static Mesh"), 20));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SkeletalMesh, FName("SkeletalMesh"), TEXT("SK_"),
        TEXT("/Game/Models/SkeletalMeshes"), FName("Meshes"),
        LOCTEXT("SkeletalMesh", "Skeletal Mesh"), 20));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::Skeleton, FName("Skeleton"), TEXT("SKEL_"),
        TEXT("/Game/Models/Skeletons"), FName("Meshes"),
        LOCTEXT("Skeleton", "Skeleton"), 20));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::PhysicsAsset, FName("PhysicsAsset"), TEXT("PHYS_"),
        TEXT("/Game/Models/Physics"), FName("Meshes"),
        LOCTEXT("PhysicsAsset", "Physics Asset"), 20));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::DestructibleMesh, FName("DestructibleMesh"), TEXT("DM_"),
        TEXT("/Game/Models/Destructibles"), FName("Meshes"),
        LOCTEXT("DestructibleMesh", "Destructible Mesh"), 20));

    // OLD MATERIALS SECTION - REORDERED ABOVE (Materials now at P10, Meshes at P20)

    // =====================================================
    // TEXTURES (7 types) - Priority 5 (moved first)
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::Texture2D, FName("Texture2D"), TEXT("T_"),
        TEXT("/Game/Textures"), FName("Textures"),
        LOCTEXT("Texture2D", "Texture 2D"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::TextureCube, FName("TextureCube"), TEXT("TC_"),
        TEXT("/Game/Textures/Cubemaps"), FName("Textures"),
        LOCTEXT("TextureCube", "Texture Cube"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::TextureRenderTarget2D, FName("TextureRenderTarget2D"), TEXT("RT_"),
        TEXT("/Game/Textures/RenderTargets"), FName("Textures"),
        LOCTEXT("TextureRenderTarget", "Render Target"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::TextureRenderTargetCube, FName("TextureRenderTargetCube"), TEXT("RTC_"),
        TEXT("/Game/Textures/RenderTargets"), FName("Textures"),
        LOCTEXT("TextureRenderTargetCube", "Render Target Cube"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MediaTexture, FName("MediaTexture"), TEXT("MT_"),
        TEXT("/Game/Media/Textures"), FName("Textures"),
        LOCTEXT("MediaTexture", "Media Texture"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::TextureLightProfile, FName("TextureLightProfile"), TEXT("TLP_"),
        TEXT("/Game/Textures/LightProfiles"), FName("Textures"),
        LOCTEXT("TextureLightProfile", "Light Profile"), 5));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::VolumeTexture, FName("VolumeTexture"), TEXT("VT_"),
        TEXT("/Game/Textures/Volume"), FName("Textures"),
        LOCTEXT("VolumeTexture", "Volume Texture"), 5));

    // =====================================================
    // ANIMATION (11 types) - Priority 30
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AnimSequence, FName("AnimSequence"), TEXT("A_"),
        TEXT("/Game/Animations/Sequences"), FName("Animation"),
        LOCTEXT("AnimSequence", "Animation Sequence"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AnimMontage, FName("AnimMontage"), TEXT("AM_"),
        TEXT("/Game/Animations/Montages"), FName("Animation"),
        LOCTEXT("AnimMontage", "Animation Montage"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AnimBlueprint, FName("AnimBlueprint"), TEXT("ABP_"),
        TEXT("/Game/Animations/Blueprints"), FName("Animation"),
        LOCTEXT("AnimBlueprint", "Animation Blueprint"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AnimComposite, FName("AnimComposite"), TEXT("AC_"),
        TEXT("/Game/Animations/Composites"), FName("Animation"),
        LOCTEXT("AnimComposite", "Animation Composite"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::BlendSpace, FName("BlendSpace"), TEXT("BS_"),
        TEXT("/Game/Animations/BlendSpaces"), FName("Animation"),
        LOCTEXT("BlendSpace", "Blend Space"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::BlendSpace1D, FName("BlendSpace1D"), TEXT("BS_"),
        TEXT("/Game/Animations/BlendSpaces"), FName("Animation"),
        LOCTEXT("BlendSpace1D", "Blend Space 1D"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AimOffsetBlendSpace, FName("AimOffsetBlendSpace"), TEXT("AO_"),
        TEXT("/Game/Animations/AimOffsets"), FName("Animation"),
        LOCTEXT("AimOffset", "Aim Offset"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::AimOffsetBlendSpace1D, FName("AimOffsetBlendSpace1D"), TEXT("AO_"),
        TEXT("/Game/Animations/AimOffsets"), FName("Animation"),
        LOCTEXT("AimOffset1D", "Aim Offset 1D"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::PoseAsset, FName("PoseAsset"), TEXT("Pose_"),
        TEXT("/Game/Animations/Poses"), FName("Animation"),
        LOCTEXT("PoseAsset", "Pose Asset"), 30));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::LevelSequence, FName("LevelSequence"), TEXT("LS_"),
        TEXT("/Game/Cinematics/Sequences"), FName("Animation"),
        LOCTEXT("LevelSequence", "Level Sequence"), 40));

    // =====================================================
    // BLUEPRINTS (4 types) - Priority 50
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::Blueprint, FName("Blueprint"), TEXT("BP_"),
        TEXT("/Game/Blueprints"), FName("Blueprints"),
        LOCTEXT("Blueprint", "Blueprint"), 50));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::WidgetBlueprint, FName("WidgetBlueprint"), TEXT("WBP_"),
        TEXT("/Game/UI/Widgets"), FName("Blueprints"),
        LOCTEXT("WidgetBlueprint", "Widget Blueprint"), 50));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::UserDefinedEnum, FName("UserDefinedEnum"), TEXT("E_"),
        TEXT("/Game/Data/Enums"), FName("Blueprints"),
        LOCTEXT("UserDefinedEnum", "User Defined Enum"), 50));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::UserDefinedStruct, FName("UserDefinedStruct"), TEXT("S_"),
        TEXT("/Game/Data/Structs"), FName("Blueprints"),
        LOCTEXT("UserDefinedStruct", "User Defined Struct"), 50));

    // =====================================================
    // AI (3 types) - Priority 45
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::BehaviorTree, FName("BehaviorTree"), TEXT("BT_"),
        TEXT("/Game/AI/BehaviorTrees"), FName("AI"),
        LOCTEXT("BehaviorTree", "Behavior Tree"), 45));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::BlackboardData, FName("BlackboardData"), TEXT("BB_"),
        TEXT("/Game/AI/Blackboards"), FName("AI"),
        LOCTEXT("Blackboard", "Blackboard"), 45));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::EnvQuery, FName("EnvQuery"), TEXT("EQS_"),
        TEXT("/Game/AI/EQS"), FName("AI"),
        LOCTEXT("EnvQuery", "Environment Query"), 45));

    // =====================================================
    // DATA (8 types) - Priority 55
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::DataTable, FName("DataTable"), TEXT("DT_"),
        TEXT("/Game/Data/Tables"), FName("Data"),
        LOCTEXT("DataTable", "Data Table"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::DataAsset, FName("DataAsset"), TEXT("DA_"),
        TEXT("/Game/Data/Assets"), FName("Data"),
        LOCTEXT("DataAsset", "Data Asset"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::StringTable, FName("StringTable"), TEXT("ST_"),
        TEXT("/Game/Data/StringTables"), FName("Data"),
        LOCTEXT("StringTable", "String Table"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::CurveFloat, FName("CurveFloat"), TEXT("Curve_"),
        TEXT("/Game/Data/Curves"), FName("Data"),
        LOCTEXT("CurveFloat", "Float Curve"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::CurveVector, FName("CurveVector"), TEXT("Curve_"),
        TEXT("/Game/Data/Curves"), FName("Data"),
        LOCTEXT("CurveVector", "Vector Curve"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::CurveLinearColor, FName("CurveLinearColor"), TEXT("Curve_"),
        TEXT("/Game/Data/Curves"), FName("Data"),
        LOCTEXT("CurveColor", "Color Curve"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::CurveTable, FName("CurveTable"), TEXT("CT_"),
        TEXT("/Game/Data/CurveTables"), FName("Data"),
        LOCTEXT("CurveTable", "Curve Table"), 55));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::CompositeCurveTable, FName("CompositeCurveTable"), TEXT("CCT_"),
        TEXT("/Game/Data/CurveTables"), FName("Data"),
        LOCTEXT("CompositeCurveTable", "Composite Curve Table"), 55));

    // =====================================================
    // AUDIO (8 types) - Priority 35
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundWave, FName("SoundWave"), TEXT("SW_"),
        TEXT("/Game/Audio/Waves"), FName("Audio"),
        LOCTEXT("SoundWave", "Sound Wave"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundCue, FName("SoundCue"), TEXT("SC_"),
        TEXT("/Game/Audio/Cues"), FName("Audio"),
        LOCTEXT("SoundCue", "Sound Cue"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundAttenuation, FName("SoundAttenuation"), TEXT("ATT_"),
        TEXT("/Game/Audio/Attenuations"), FName("Audio"),
        LOCTEXT("SoundAttenuation", "Sound Attenuation"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundClass, FName("SoundClass"), TEXT(""),
        TEXT("/Game/Audio/SoundClasses"), FName("Audio"),
        LOCTEXT("SoundClass", "Sound Class"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundMix, FName("SoundMix"), TEXT("Mix_"),
        TEXT("/Game/Audio/Mixes"), FName("Audio"),
        LOCTEXT("SoundMix", "Sound Mix"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::SoundConcurrency, FName("SoundConcurrency"), TEXT("CON_"),
        TEXT("/Game/Audio/Concurrencies"), FName("Audio"),
        LOCTEXT("SoundConcurrency", "Sound Concurrency"), 35));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::ReverbEffect, FName("ReverbEffect"), TEXT("Reverb_"),
        TEXT("/Game/Audio/Reverbs"), FName("Audio"),
        LOCTEXT("ReverbEffect", "Reverb Effect"), 35));

    // =====================================================
    // VFX (4 types) - Priority 32
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::ParticleSystem, FName("ParticleSystem"), TEXT("PS_"),
        TEXT("/Game/Effects/Particles"), FName("VFX"),
        LOCTEXT("ParticleSystem", "Particle System (Cascade)"), 32));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::NiagaraSystem, FName("NiagaraSystem"), TEXT("NS_"),
        TEXT("/Game/Effects/Niagara"), FName("VFX"),
        LOCTEXT("NiagaraSystem", "Niagara System"), 32));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::NiagaraEmitter, FName("NiagaraEmitter"), TEXT("NE_"),
        TEXT("/Game/Effects/Niagara/Emitters"), FName("VFX"),
        LOCTEXT("NiagaraEmitter", "Niagara Emitter"), 32));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::NiagaraParameterCollection, FName("NiagaraParameterCollection"), TEXT("NPC_"),
        TEXT("/Game/Effects/Niagara/ParameterCollections"), FName("VFX"),
        LOCTEXT("NiagaraParameterCollection", "Niagara Parameter Collection"), 31));

    // =====================================================
    // ENVIRONMENT (3 types) - Priority 60
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::FoliageType, FName("FoliageType"), TEXT("FT_"),
        TEXT("/Game/Environment/Foliage"), FName("Environment"),
        LOCTEXT("FoliageType", "Foliage Type"), 60));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::LandscapeGrassType, FName("LandscapeGrassType"), TEXT("LG_"),
        TEXT("/Game/Environment/Landscape"), FName("Environment"),
        LOCTEXT("LandscapeGrass", "Landscape Grass Type"), 60));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::LandscapeLayerInfoObject, FName("LandscapeLayerInfoObject"), TEXT("LL_"),
        TEXT("/Game/Environment/Landscape"), FName("Environment"),
        LOCTEXT("LandscapeLayer", "Landscape Layer Info"), 60));

    // =====================================================
    // MEDIA (2 types) - Priority 65
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::MediaPlayer, FName("MediaPlayer"), TEXT("MP_"),
        TEXT("/Game/Media/Players"), FName("Media"),
        LOCTEXT("MediaPlayer", "Media Player"), 65));

    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::FileMediaSource, FName("FileMediaSource"), TEXT("FMS_"),
        TEXT("/Game/Media/Sources"), FName("Media"),
        LOCTEXT("FileMediaSource", "File Media Source"), 65));

    // =====================================================
    // LEVELS (1 type) - Priority 100 (last)
    // NOTE: World/Level assets are disabled by default to prevent reference loss
    // Levels contain complex actor references and should generally not be moved
    // =====================================================
    FAssetTypeInfo WorldType(
        EAssetOrganizeType::World, FName("World"), TEXT(""),
        TEXT("/Game/Maps"), FName("Levels"),
        LOCTEXT("World", "Level/World"), 1);
    WorldType.bEnabled = false;  // Disabled by default - enable with caution
    AssetTypeConfigs.Add(WorldType);

    // =====================================================
    // OTHER (1 type) - Priority 999 (last)
    // =====================================================
    AssetTypeConfigs.Add(FAssetTypeInfo(
        EAssetOrganizeType::Other, FName(""), TEXT("*"),
        TEXT("/Game/Other"), FName("Uncategorized"),
        LOCTEXT("Other", "Other/Uncategorized"), 999));
}

FAssetTypeInfo UAssetOrganizerSettings::GetTypeInfo(const FName& AssetClass) const
{
    for (const FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.ClassName == AssetClass)
        {
            return Info;
        }
    }
    return FAssetTypeInfo();
}

void UAssetOrganizerSettings::UpdateTypeInfo(const FAssetTypeInfo& TypeInfo)
{
    for (FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.Type == TypeInfo.Type)
        {
            Info = TypeInfo;
            SaveConfig();
            return;
        }
    }
}

void UAssetOrganizerSettings::SetTypeEnabled(EAssetOrganizeType Type, bool bEnabled)
{
    for (FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.Type == Type)
        {
            Info.bEnabled = bEnabled;
            SaveConfig();
            return;
        }
    }
}

void UAssetOrganizerSettings::SetTypeTargetPath(EAssetOrganizeType Type, const FString& NewPath)
{
    for (FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.Type == Type)
        {
            Info.TargetPath = NewPath;
            SaveConfig();
            return;
        }
    }
}

void UAssetOrganizerSettings::AddCustomRule(const FCustomAssetRule& Rule)
{
    CustomRules.Add(Rule);
    SaveConfig();
}

void UAssetOrganizerSettings::RemoveCustomRule(int32 Index)
{
    if (CustomRules.IsValidIndex(Index))
    {
        CustomRules.RemoveAt(Index);
        SaveConfig();
    }
}

void UAssetOrganizerSettings::SaveSettings()
{
    SaveConfig();
}

TArray<FAssetTypeInfo> UAssetOrganizerSettings::GetActiveTypes() const
{
    TArray<FAssetTypeInfo> ActiveTypes;
    for (const FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.bEnabled && !Info.ClassName.IsNone())
        {
            ActiveTypes.Add(Info);
        }
    }
    return ActiveTypes;
}

TArray<FString> UAssetOrganizerSettings::GetNeededFolders(const TArray<FAssetData>& AssetsToOrganize) const
{
    TSet<FString> NeededFolders;
    TSet<FName> ActiveClasses;

    // Collect active asset classes from the assets
    for (const FAssetData& Asset : AssetsToOrganize)
    {
        ActiveClasses.Add(Asset.AssetClass);
    }

    // Find matching type configs
    for (const FAssetTypeInfo& Info : AssetTypeConfigs)
    {
        if (Info.bEnabled && ActiveClasses.Contains(Info.ClassName))
        {
            FString EffectivePath = RootFolderConfig.GetEffectivePath(Info.TargetPath);
            NeededFolders.Add(EffectivePath);
        }
    }

    // Add custom rules
    for (const FCustomAssetRule& Rule : CustomRules)
    {
        if (Rule.bEnabled && ActiveClasses.Contains(FName(*Rule.ClassName)))
        {
            FString EffectivePath = RootFolderConfig.GetEffectivePath(Rule.TargetPath);
            NeededFolders.Add(EffectivePath);
        }
    }

    return NeededFolders.Array();
}

bool UAssetOrganizerSettings::IsFolderWhitelisted(const FString& FolderPath) const
{
    for (const FString& WL : WhitelistedFolders)
    {
        if (!WL.IsEmpty() && FolderPath.StartsWith(WL))
        {
            return true;
        }
    }
    return false;
}

void UAssetOrganizerSettings::MigrateFromLegacyExcludedFolders()
{
    // One-time migration: if ExcludedFolders has values but WhitelistedFolders is empty,
    // copy them over and save
    if (ExcludedFolders.Num() > 0 && WhitelistedFolders.Num() == 0)
    {
        WhitelistedFolders = ExcludedFolders;
        SaveConfig();
    }
}

#undef LOCTEXT_NAMESPACE
