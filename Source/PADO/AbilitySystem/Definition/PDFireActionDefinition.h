#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PDFireActionDefinition.generated.h"

/** 방아쇠 입력을 발사로 해석하는 방식이다. */
UENUM(BlueprintType)
enum class EPDFireMode : uint8
{
	/** 누르는 동안 발 간격마다 쏜다. */
	Automatic UMETA(DisplayName = "Automatic"),

	/** 누를 때마다 한 발이다. 준비되기 전에 누른 입력은 버린다. */
	SemiAutomatic UMETA(DisplayName = "Semi Automatic"),

	/** 누를 때마다 정해진 발 수를 쏜다. 중간에 떼도 끝까지 쏜다. */
	Burst UMETA(DisplayName = "Burst")
};

/**
 * 방아쇠로 쏘는 무기의 Action Definition이다.
 *
 * 무기를 들고 있는 동안 활성 상태를 유지하고, 발사는 그 안에서 조종하는 머신이
 * 자기 시계로 한다. 발사가 활성화가 아니라서 연사력이 네트워크 지연에 좌우되지
 * 않는다. 설계는 docs/FireActionSystem.md에 있다.
 *
 * 기반 클래스의 ActionMontage와 ActionCooldown은 쓰지 않는다. 발 간격은
 * ShotInterval이 정하고, 발사 모션은 애님 블루프린트가 발사 신호로 재생한다.
 */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Fire Action"))
class PADO_API UPDFireActionDefinition : public UPDAbilityDefinition
{
	GENERATED_BODY()

public:
	/** 발 간격이 0이면 발사 일정이 한자리에서 끝없이 쏜다. 그 아래를 막는 값이다. */
	static constexpr float MinimumShotInterval = 0.001f;

	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	EPDFireMode FireMode = EPDFireMode::Automatic;

	/**
	 * 발과 발 사이의 최소 간격이다. 모든 모드에 적용된다. 0.1이면 초당 10발이다.
	 * 발사 일정이 이 값을 그대로 더해 가므로 연사력이 프레임레이트에 좌우되지 않는다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Fire",
		meta = (ClampMin = "0.001", UIMin = "0.02", UIMax = "2.0", Units = "s"))
	float ShotInterval = 0.1f;

	/** 점사 한 번에 나가는 발 수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Fire",
		meta = (
			EditCondition = "FireMode == EPDFireMode::Burst",
			EditConditionHides,
			ClampMin = "2",
			UIMax = "10"))
	int32 BurstCount = 3;

	/** 점사의 마지막 발 뒤 다음 점사까지 기다리는 시간이다. 발 간격보다 짧으면 발 간격을 쓴다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Fire",
		meta = (
			EditCondition = "FireMode == EPDFireMode::Burst",
			EditConditionHides,
			ClampMin = "0.0",
			UIMax = "2.0",
			Units = "s"))
	float BurstCooldown = 0.3f;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
