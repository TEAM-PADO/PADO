#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PADO/Item/Struct/PDItemPresentationStruct.h"
#include "PDItemDefinition.generated.h"

class UTexture2D;
class UPDAbilityDefinition;
class UPDItemTrait;

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

	/**
	 * 이 아이템이 갖는 선택 기능이다. 필요한 것만 추가한다.
	 *
	 * 넣지 않은 기능은 값 자체가 존재하지 않는다. 새 기능을 만들 때 이 클래스에
	 * 필드를 늘리지 말고 `UPDItemTrait` 파생 클래스를 추가한다.
	 * 같은 클래스를 두 개 넣으면 하나가 조용히 무시되므로 Validate가 막는다.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Item|Traits")
	TArray<TObjectPtr<UPDItemTrait>> Traits;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool IsUsable() const;

	/** 없으면 nullptr을 반환한다. 매 프레임 부르지 말고 호출부에서 캐시한다. */
	UFUNCTION(BlueprintPure, Category = "PD|Item", meta = (DeterminesOutputType = "TraitClass"))
	const UPDItemTrait* FindTrait(TSubclassOf<UPDItemTrait> TraitClass) const;

	template <typename TraitType>
	const TraitType* FindTrait() const
	{
		return Cast<TraitType>(FindTrait(TraitType::StaticClass()));
	}

	virtual bool Validate(FString& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
};
