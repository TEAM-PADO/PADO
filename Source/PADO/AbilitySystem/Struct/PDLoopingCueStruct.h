#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDLoopingCueStruct.generated.h"

/**
 * 실행 구간 내내 유지되는 표현용 GameplayCue다.
 *
 * 일회성 Cue와 달리 시작과 끝이 반드시 짝을 이뤄야 하므로 Fragment가 소유할 수 없다.
 * Fragment는 Execute 하나뿐이라 "끝낼 때"가 없고, Hook은 취소·중단 경로에서 실행되지
 * 않아 이펙트가 남는다. 그래서 GA가 수명을 쥐고 EndAbility에서 반드시 뗀다.
 *
 * Cue Notify는 AGameplayCueNotify_Looping(GCN Looping)을 쓴다. 같은 태그로 일회성
 * Cue를 실행하면 그 Notify의 Recurring 구간이 재생되므로 타격 연출을 함께 담을 수 있다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDLoopingCueStruct
{
	GENERATED_BODY()

	/** 실행 구간 동안 유지할 Cue다. 비워 두면 Looping Cue를 쓰지 않는다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Looping Cue",
		meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	/**
	 * true면 Cue를 소스 아이템 메시에 붙인다. false면 Avatar가 기준이 된다.
	 * 실제 소켓과 부착 규칙은 Cue Notify의 Placement Info가 정한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Looping Cue")
	bool bAttachToSourceItem = true;

	bool IsEnabled() const;
	bool Validate(FString& OutError) const;
};
