#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PADO/Item/Struct/PDItemPresentationStruct.h"
#include "PADO/Item/Struct/PDItemMagazineConfig.h"
#include "PDItemDefinition.generated.h"

class UTexture2D;
class UPDAbilityDefinition;

/** 모든 월드 아이템의 불변 구성 데이터다. 기능은 선택 설정과 Action으로 조립한다. */
UCLASS(BlueprintType)
class PADO_API UPDItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		AssetRegistrySearchable,
		Category = "Item",
		meta = (Categories = "Item.Id"))
	FGameplayTag ItemId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Item",
		meta = (ShowOnlyInnerProperties))
	FPDItemPresentationStruct Presentation;

	/** 비어 있으면 사용 Ability가 없는 운반 전용 아이템이다. */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Item|Action")
	TObjectPtr<UPDAbilityDefinition> UseAction;

	/** 탄창을 사용하지 않는 아이템에서는 비활성화한다. 현재 탄약은 Actor의 Component에 둔다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Magazine",
		meta = (ShowOnlyInnerProperties))
	FPDItemMagazineConfig Magazine;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool IsUsable() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool HasMagazine() const { return Magazine.bEnabled; }

	virtual bool Validate(FString& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
};
