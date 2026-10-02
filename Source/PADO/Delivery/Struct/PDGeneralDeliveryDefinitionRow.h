#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PADO/Delivery/Struct/PDDeliveryCommonDefinitionData.h"
#include "PDGeneralDeliveryDefinitionRow.generated.h"

/** 일반 배송 DataTable의 한 행입니다. 행 이름 자체가 코어가 발급한 DeliveryDefinitionId입니다. */
USTRUCT(BlueprintType)
struct PADO_API FPDGeneralDeliveryDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 일반·보스 배달에 공통으로 적용하는 배송지, 해금, 기본 보상 데이터입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ShowOnlyInnerProperties))
	FPDDeliveryCommonDefinitionData CommonData;

	/** 수락 뒤 최종 실패까지의 서버 기준 제한 시간(초)입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "1"))
	int32 DeadlineSeconds = 1;

	/** 보상을 감액하는 서버 기준 시점(수락 후 초)입니다. 최종 제한 시간보다 작아야 합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "1"))
	int32 RewardReductionTimeSeconds = 1;

	/** 감액 뒤 완료 보상입니다. 기본 보상보다 클 수 없습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "0"))
	int32 ReducedReward = 0;

	/** @return 일반 배송 진행에 필요한 데이터가 유효하면 true입니다. */
	bool Validate(FString& OutError) const;
};
