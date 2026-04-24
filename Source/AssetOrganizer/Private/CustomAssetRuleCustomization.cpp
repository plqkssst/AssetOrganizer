// Copyright UEMaster. All Rights Reserved.

#include "CustomAssetRuleCustomization.h"
#include "AssetOrganizerTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"

#define LOCTEXT_NAMESPACE "AssetOrganizer"

TSharedRef<IPropertyTypeCustomization> FCustomAssetRuleCustomization::MakeInstance()
{
    return MakeShareable(new FCustomAssetRuleCustomization);
}

void FCustomAssetRuleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    HeaderRow.NameContent()
        [
            PropertyHandle->CreatePropertyNameWidget()
        ];
}

void FCustomAssetRuleCustomization::BuildClassOptions()
{
    ClassOptions.Empty();
    TSet<FString> UniqueNames;

    // 1. Built-in asset types from AssetTools registration
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

    // 2. Asset classes from AssetRegistry scan
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

    // Sort and populate
    TArray<FString> SortedNames = UniqueNames.Array();
    SortedNames.Sort();

    for (const FString& Name : SortedNames)
    {
        ClassOptions.Add(MakeShared<FString>(Name));
    }
}

void FCustomAssetRuleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    BuildClassOptions();

    ClassNameProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCustomAssetRule, ClassName));
    TSharedPtr<IPropertyHandle> PrefixProperty        = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCustomAssetRule, Prefix));
    TSharedPtr<IPropertyHandle> TargetPathProperty     = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCustomAssetRule, TargetPath));
    TSharedPtr<IPropertyHandle> CategoryNameProperty   = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCustomAssetRule, CategoryName));
    TSharedPtr<IPropertyHandle> bEnabledProperty       = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCustomAssetRule, bEnabled));

    // Class Name row — custom combo box
    ChildBuilder.AddCustomRow(LOCTEXT("ClassNameRow", "Class Name"))
        .NameContent()
        [
            ClassNameProperty.IsValid()
                ? ClassNameProperty->CreatePropertyNameWidget()
                : SNew(STextBlock).Text(LOCTEXT("ClassNameRow", "Class Name"))
        ]
        .ValueContent()
        .MinDesiredWidth(250.f)
        [
            SNew(SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&ClassOptions)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewSelection, ESelectInfo::Type)
            {
                if (NewSelection.IsValid() && ClassNameProperty.IsValid())
                {
                    ClassNameProperty->SetValue(*NewSelection);
                }
            })
            .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
            {
                return SNew(STextBlock)
                    .Text(FText::FromString(InOption.IsValid() ? *InOption : TEXT("")));
            })
            .Content()
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    if (ClassNameProperty.IsValid())
                    {
                        FString Value;
                        if (ClassNameProperty->GetValue(Value) == FPropertyAccess::Success)
                        {
                            return FText::FromString(Value);
                        }
                    }
                    return LOCTEXT("SelectClassName", "Select asset class...");
                })
            ]
        ];

    // Remaining fields — default property widgets
    if (PrefixProperty.IsValid())
    {
        ChildBuilder.AddProperty(PrefixProperty.ToSharedRef());
    }
    if (TargetPathProperty.IsValid())
    {
        ChildBuilder.AddProperty(TargetPathProperty.ToSharedRef());
    }
    if (CategoryNameProperty.IsValid())
    {
        ChildBuilder.AddProperty(CategoryNameProperty.ToSharedRef());
    }
    if (bEnabledProperty.IsValid())
    {
        ChildBuilder.AddProperty(bEnabledProperty.ToSharedRef());
    }
}

#undef LOCTEXT_NAMESPACE
