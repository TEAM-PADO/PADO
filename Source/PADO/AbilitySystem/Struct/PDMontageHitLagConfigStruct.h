#pragma once

#include "CoreMinimal.h"
#include "PDMontageHitLagConfigStruct.generated.h"

/** 이미 역경직 중일 때 새 명중이 들어온 경우의 처리 방식이다. */
UENUM(BlueprintType)
enum class EPDMontageHitLagRetriggerPolicy : uint8
{
	/** 현재 역경직이 끝날 때까지 새 요청을 무시한다. */
	IgnoreWhileActive UMETA(DisplayName = "Ignore While Active"),

	/** 새 요청의 세기와 지속시간으로 역경직을 다시 시작한다. */
	RestartDuration UMETA(DisplayName = "Restart Duration")
};

/** 공격 몽타주에 적용할 짧은 역경직 설정이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDMontageHitLagConfigStruct
{
	GENERATED_BODY()

	/** 원래 몽타주 재생률에 곱할 값이다. 1보다 작을수록 강하게 느려진다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.05", ClampMax = "0.95", UIMin = "0.05", UIMax = "0.95"))
	float PlayRateMultiplier = 0.2f;

	/** 몽타주 재생률을 낮게 유지할 실제 시간이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.001", ClampMax = "0.5", UIMin = "0.01", UIMax = "0.2", Units = "s"))
	float Duration = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	EPDMontageHitLagRetriggerPolicy RetriggerPolicy =
		EPDMontageHitLagRetriggerPolicy::IgnoreWhileActive;

	bool Validate(FString& OutError) const;
	float ResolvePlayRate(float BasePlayRate) const;
};
