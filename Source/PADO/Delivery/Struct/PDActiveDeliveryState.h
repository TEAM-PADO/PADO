#pragma once

#include "CoreMinimal.h"
#include "PADO/Delivery/Enum/PDDeliveryPhase.h"
#include "PADO/Delivery/Enum/PDDeliveryType.h"
#include "PDActiveDeliveryState.generated.h"

/**
 * 모든 플레이어가 함께 보는 현재 배달 상태입니다.
 * 시간 값은 서버 시간 기준 절대 시각이며, UI는 GameState의 서버 시간을 이용해 남은 시간을 계산합니다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDActiveDeliveryState
{
	GENERATED_BODY()

	/** 배달 식별자입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	FName DeliveryId;

	/** 현재 배달의 목적지 식별자입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	FName DestinationId;

	/** 일반 배달과 보스 배달을 구분하는 큰 분류입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	EPDDeliveryType DeliveryType = EPDDeliveryType::General;

	/** 서버가 판정한 현재 배달 진행 단계입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	EPDDeliveryPhase Phase = EPDDeliveryPhase::None;

	/** 현재 단계가 시작된 서버 월드 시간의 절대 시각입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	double ServerPhaseStartTime = 0.0;

	/** 현재 단계 제한 시간의 서버 월드 시간 기준 절대 마감 시각입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	double ServerDeadlineTime = 0.0;

	/** 서버가 최종 확정한 현재 배달 보상입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	int32 CurrentReward = 0;

	/** 2단계 실패 규칙으로 보상이 이미 감소했는지 나타냅니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	bool bIsRewardReduced = false;

	/** 현재 상자를 보유한 플레이어의 온라인 식별자 문자열입니다. 보유자가 없으면 비어 있습니다. */
	UPROPERTY(BlueprintReadOnly, Category = "PADO|Delivery")
	FString PackageHolderPlayerId;

	/** @return 진행 중인 배달 단계이면 true, None이면 false입니다. */
	bool IsActive() const
	{
		return Phase != EPDDeliveryPhase::None;
	}
};
