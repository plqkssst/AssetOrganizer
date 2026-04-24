// Copyright UEMaster. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

/**
 * Custom property editor for FCustomAssetRule.
 * Replaces the ClassName text box with a dropdown of all known engine asset classes.
 */
class FCustomAssetRuleCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    //~ Begin IPropertyTypeCustomization Interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    //~ End IPropertyTypeCustomization Interface

private:
    /** Rebuild the dropdown options from built-in types + AssetTools + AssetRegistry */
    void BuildClassOptions();

    TArray<TSharedPtr<FString>> ClassOptions;
    TSharedPtr<IPropertyHandle> ClassNameProperty;
};
