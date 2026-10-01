#pragma once

#include "CoreMinimal.h"
#include "PDDeliveryCommonDefinitionData.generated.h"

/** 일반·보스 배달 정의가 공유하는 데이터입니다. */
USTRUCT(BlueprintType)
struct PADO_API FPDDeliveryCommonDefinitionData
{
	GENERATED_BODY()

	/** 레벨 배송지 Anchor와 연결하는 코어 발급 식별자입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	FName DeliveryLocationId;

	/** UI에 표시할 배달 이름입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	FText DisplayName;

	/** 이 배달을 수락하기 전에 완료해야 하는 일반 배달 횟수입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "0"))
	int32 RequiredCompletedGeneralDeliveryCount = 0;

	/** 이 배달을 수락하기 전에 완료해야 하는 보스 배달 정의 ID입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	TArray<FName> RequiredCompletedBossDeliveryIds;

	/** 서버가 완료 보상 자격 대상에게 지급하도록 판정할 기본 코인 보상입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "0"))
	int32 BaseReward = 0;

	/** @return 실행에 필요한 공통 데이터가 유효하면 true입니다. */
	bool Validate(FString& OutError) const;
};
