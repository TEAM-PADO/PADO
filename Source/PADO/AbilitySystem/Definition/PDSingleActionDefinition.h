#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PDSingleActionDefinition.generated.h"

class UPDThrowProjectileFragment;

/** Press 한 번에 한 번 실행하고 종료하는 Action Definition이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Single Action"))
class PADO_API UPDSingleActionDefinition : public UPDAbilityDefinition
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const override;

	/** 충전 투척 Fragment가 있으면 Press가 아니라 Release에서 실행한다. */
	bool ExecutesOnInputRelease() const;
	const UPDThrowProjectileFragment* FindChargedThrowProjectileFragment() const;

	/**
	 * 누르고 있는 동안 실행을 반복한다.
	 *
	 * 반복은 활성화를 다시 여는 방식이다. 한 번이 곧 한 활성화라서 반복마다 활성화
	 * 왕복이 생긴다. 방아쇠로 쏘는 총기는 이것이 아니라 Fire Action을 쓴다.
	 *
	 * 반복 간격은 `ActionCooldown.Duration`이다. 조종하는 머신이 같은 값으로
	 * 반복을 돌리고 자기 시계로 쿨다운을 판정한다. 그래서 값이 하나뿐이고,
	 * 네트워크 지연이 반복 간격을 바꾸지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	bool bAutomatic = false;

	/** 조종하는 머신이 입력을 반복하는 간격이다. 자동이 아니면 0이다. */
	float GetAutomaticFireInterval() const;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
