#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDActionCooldownStruct.generated.h"

/**
 * Action 하나의 재사용 대기시간이다.
 *
 * 몽타주나 Channel 수명주기가 발사 간격을 만들어 주지 않는 Action은 이 데이터가 없으면
 * 입력을 연타하는 만큼 실행된다. 쿨다운은 Definition이 소유하고 GA는 검사와 적용만 한다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDActionCooldownStruct
{
	GENERATED_BODY()

	/**
	 * 이 Action이 점유하는 쿨다운 슬롯이다.
	 * 같은 태그를 쓰는 Action끼리는 쿨다운을 공유한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Cooldown",
		meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	/** 0이면 쿨다운을 쓰지 않는다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Cooldown",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0", Units = "s"))
	float Duration = 0.0f;

	bool IsEnabled() const;
	bool Validate(FString& OutError) const;
};
