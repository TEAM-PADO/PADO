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
	 * 누르고 있는 동안 발사를 반복한다. 자동 소총이 이것이다.
	 *
	 * 반복은 활성화를 다시 여는 방식이다. 한 발이 곧 한 활성화이므로 발사마다
	 * 예측 ID가 생기고, 총구 화염과 반동을 그 단위로 걸 수 있다. 이어지는
	 * 채널(화염방사기 같은)은 이것이 아니라 Channel Action이다.
	 *
	 * 반복 간격은 `ActionCooldown.Duration`이다. 같은 값을 서버가 강제하고
	 * 클라이언트가 반복에 쓴다. 그래서 값이 하나뿐이고 어긋나지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	bool bAutomatic = false;

	/** 클라이언트가 입력을 반복하는 간격이다. 자동이 아니면 0이다. */
	float GetAutomaticFireInterval() const;

	/**
	 * 서버가 실제로 거는 쿨다운이다. 반복 간격보다 약간 짧다.
	 *
	 * 둘이 같으면 클라 타이머와 서버 쿨다운이 매 발 경합해서, 지터에 따라
	 * 발사가 수시로 거부된다. 연사가 끊겨 보이는 원인이 된다. 그래서 서버는
	 * 약간의 여유를 두고, 대신 그만큼만 빠른 연사를 허용한다.
	 */
	float GetEnforcedCooldownDuration() const;

	/** 서버가 허용하는 연사 속도 여유다. */
	static constexpr float AutomaticFireCooldownTolerance = 0.1f;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
