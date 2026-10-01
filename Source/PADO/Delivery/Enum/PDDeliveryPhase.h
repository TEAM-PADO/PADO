#pragma once

#include "CoreMinimal.h"
#include "PDDeliveryPhase.generated.h"

/**
 * 배달 및 보스 전투의 서버 권위 진행 단계입니다.
 */
UENUM(BlueprintType)
enum class EPDDeliveryPhase : uint8
{
	/** 수락된 팀 배달이 없는 대기 상태입니다. 우체국에서 다음 후보를 수락할 수 있습니다. */
	None UMETA(DisplayName = "None"),

	/** 일반 배달을 수락한 직후부터 첫 보상 감소 시각 전까지의 진행 상태입니다. 제한 시간은 계속 흐릅니다. */
	GeneralInProgress UMETA(DisplayName = "General In Progress"),

	/**
	 * 일반 배달이 첫 제한 시각을 지나 보상이 한 번 감소한 진행 상태입니다.
	 * 아직 최종 마감 전이므로 배송을 완료할 수 있으며, 최종 마감 시각을 넘기면 서버가 배달을 실패 처리합니다.
	 */
	GeneralRewardReduced UMETA(DisplayName = "General Reward Reduced"),

	/**
	 * 보스 배달을 수락하여 보스 처치 제한시간이 흐르는 전투 상태입니다.
	 * 보스를 제한시간 안에 처치해야 현관 배송 단계로 전환할 수 있으며, 시간 초과 시 해당 배달은 실패합니다.
	 */
	BossCombatInProgress UMETA(DisplayName = "Boss Combat In Progress"),

	/**
	 * 보스를 제한시간 안에 처치한 뒤 현관 배송을 진행하는 상태입니다.
	 * 이 단계에서는 배송 제한시간이 없으며, 마지막 보스 배달은 택배 소지자의 문 앞 도착 시 별도 체크포인트를 저장합니다.
	 */
	BossDeliveryReady UMETA(DisplayName = "Boss Delivery Ready")
};
