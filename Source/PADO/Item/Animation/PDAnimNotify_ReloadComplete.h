#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PDAnimNotify_ReloadComplete.generated.h"

/**
 * 재장전 몽타주에서 탄창이 실제로 채워지는 시점을 표시한다.
 *
 * 이 노티파이를 찍은 프레임이 곧 충전 시각이다. 재장전 시간을 숫자로 따로
 * 적지 않고 몽타주가 직접 정하게 하려는 것이므로, 무기마다 다른 시각을
 * 쓰려면 몽타주에서 이 노티파이를 옮기면 된다.
 *
 * 서버의 몽타주 인스턴스에서 발생한 것만 탄약을 바꾼다.
 */
UCLASS(meta = (DisplayName = "PD Reload Complete"))
class PADO_API UPDAnimNotify_ReloadComplete : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
