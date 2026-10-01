#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PADO/Delivery/Enum/PDDeliveryType.h"
#include "PADO/Delivery/Struct/PDDeliveryCommonDefinitionData.h"
#include "PADO/Delivery/Struct/PDGeneralDeliveryDefinitionRow.h"
#include "PDDeliveryDefinitionSet.generated.h"

class UDataTable;
class UPDBossDeliveryDefinition;

/** 일반 배송 DataTable과 보스 정의 에셋을 하나의 ID 조회 계약으로 묶는 코어 소유 레지스트리입니다. */
UCLASS(BlueprintType)
class PADO_API UPDDeliveryDefinitionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** 에디터 저장·검증 시 일반·보스 정의의 누락 및 ID 충돌을 확인합니다. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** 반복 추가되는 일반 배송 정의를 관리하는 DataTable입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	TObjectPtr<UDataTable> GeneralDeliveryDefinitionTable;

	/** 보스별 전용 규칙을 담는 데이터 에셋 목록입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	TArray<TObjectPtr<UPDBossDeliveryDefinition>> BossDeliveryDefinitions;

	/** 일반 배송 정의를 ID로 조회합니다. */
	bool FindGeneralDeliveryDefinition(FName DeliveryDefinitionId, FPDGeneralDeliveryDefinitionRow& OutDefinition) const;

	/** @return 보스 배송 정의 ID에 해당하는 에셋입니다. 없으면 nullptr을 반환합니다. */
	const UPDBossDeliveryDefinition* FindBossDeliveryDefinition(FName DeliveryDefinitionId) const;

	/** 공통 정의와 유형을 ID로 조회합니다. 개별 유형 전용 값은 각 조회 함수를 사용합니다. */
	bool FindCommonDefinition(FName DeliveryDefinitionId, FPDDeliveryCommonDefinitionData& OutCommonData, EPDDeliveryType& OutDeliveryType) const;

	/** @return 일반·보스 정의 ID와 데이터의 조합이 유효하면 true입니다. */
	bool Validate(FString& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("DeliveryDefinitionSet"), GetFName());
	}
};
