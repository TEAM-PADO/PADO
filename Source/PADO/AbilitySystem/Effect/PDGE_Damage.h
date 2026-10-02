#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PDGE_Damage.generated.h"

class UAbilitySystemComponent;

/**
 * 공용 피해 GE다. 즉시 적용되고, 피해량을 SetByCaller `Data.Damage`로 받는다.
 *
 * 체력을 직접 깎지 않고 메타 속성 Damage에 더한다. 체력으로 옮기는 것과 0 아래로
 * 내려가지 않게 자르는 것은 UPDHealthAttributeSet이 서버에서 한다. 아군과 자기
 * 자신도 거르지 않는다. 막을 대상은 판정이나 대상의 상태가 정한다.
 */
UCLASS(meta = (DisplayName = "PDGE_Damage"))
class PADO_API UPDGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPDGE_Damage();

	/**
	 * 서버에서 Source ASC가 Target ASC에 피해를 준다. 아이템 행동이 아닌 피해(들이받기,
	 * 하차 착지)가 쓴다. Instigator는 Source ASC의 소유자(논리적 주체), EffectCauser는
	 * 피해를 일으킨 물체다. 반환값은 Effect를 실행했는지다. 죽은 대상(State.Dead)의
	 * 체력은 Attribute Set이 깎지 않는다.
	 */
	static bool ApplyDamage(
		UAbilitySystemComponent& Source,
		UAbilitySystemComponent& Target,
		float Damage,
		AActor* EffectCauser);
};
