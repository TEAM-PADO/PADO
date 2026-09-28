#pragma once

#include "CoreMinimal.h"
#include "PDActionMontageConfigStruct.generated.h"

class UAnimMontage;

/** Action Definition에 들어가는 몽타주 재생 데이터다. 실행 상태는 GA가 소유한다. */
USTRUCT(BlueprintType)
struct PADO_API FPDActionMontageConfigStruct
{
	GENERATED_BODY()

	/**
	 * 비어 있으면 Single Action은 즉시 실행한다.
	 * Channel Action은 Montage Event 방식일 때만 필수다. Fixed Interval은
	 * 서버 타이머가 실행 시점을 만들므로 몽타주 없이도 동작한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Playback",
		meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "3.0"))
	float PlayRate = 1.0f;

	/** 비어 있으면 몽타주 처음부터 재생한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	FName StartSection = NAME_None;

	bool Validate(FString& OutError) const;
};
