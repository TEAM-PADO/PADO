#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDEventTargeting.generated.h"

/**
 * 활성화 이벤트가 지정한 대상을 그대로 사용한다.
 * 함정처럼 소스가 대상을 이미 알고 있는 경우에 쓴다.
 */
UCLASS(meta = (DisplayName = "Event Targeting"))
class PADO_API UPDEventTargeting : public UPDInstantActionTargeting
{
	GENERATED_BODY()

public:
	virtual bool RequiresActivationTarget() const override { return true; }

	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		FPDActionTargetingResult& OutResult) const override;
};
