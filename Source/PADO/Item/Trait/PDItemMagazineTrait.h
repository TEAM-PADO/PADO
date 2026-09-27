#pragma once

#include "CoreMinimal.h"
#include "PADO/Item/Trait/PDItemTrait.h"
#include "PDItemMagazineTrait.generated.h"

class UAnimMontage;

/**
 * 탄창을 쓰는 아이템에 추가한다.
 *
 * Trait이 있으면 탄창 기능이 켜진 것이다. 별도의 사용 여부 플래그를 두지
 * 않는다. 현재 탄약과 재장전 상태는 `UPDWeaponMagazineComponent`가 소유한다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Magazine"))
class PADO_API UPDItemMagazineTrait : public UPDItemTrait
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;

	/** 탄창 하나에 들어가는 탄약 수. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "1"))
	int32 Capacity = 30;

	/**
	 * 재장전 몽타주다. 충전 시점은 몽타주에 찍은 Reload Complete 노티파이가
	 * 정하므로 재장전 시간을 따로 적지 않는다.
	 *
	 * 비워 두면 몽타주 없이 즉시 충전한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UAnimMontage> ReloadMontage;

	/** 재장전 몽타주 재생 속도다. 높을수록 빨리 장전한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "3.0"))
	float ReloadSpeed = 1.0f;

	/**
	 * 몽타주에서 충전이 일어나는 시각이다. 노티파이가 없으면 몽타주 전체
	 * 길이를 돌려준다. 몽타주가 없으면 0이다.
	 */
	float GetReloadCompleteTime() const;
};
