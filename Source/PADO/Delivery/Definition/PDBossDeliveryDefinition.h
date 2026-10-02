#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PADO/Delivery/Enum/PDBossRecipientInteractionType.h"
#include "PADO/Delivery/Struct/PDDeliveryCommonDefinitionData.h"
#include "PDBossDeliveryDefinition.generated.h"

/** 보스 한 종의 전투·배송 전환·수취인 상호작용을 정의하는 코어 소유 데이터 에셋입니다. */
UCLASS(BlueprintType)
class PADO_API UPDBossDeliveryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** 에디터 저장·검증 시 보스 배달 정의의 필수 항목을 확인합니다. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** 저장·복제·해금 규칙에 사용하는 코어 발급 보스 배달 정의 ID입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AssetRegistrySearchable, Category = "PADO|Delivery")
	FName DeliveryDefinitionId;

	/** 일반·보스 배달에 공통으로 적용하는 배송지, 해금, 기본 보상 데이터입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ShowOnlyInnerProperties))
	FPDDeliveryCommonDefinitionData CommonData;

	/** AI가 보스 Actor와 전투 상태를 연결할 때 사용하는 식별자입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	FName BossId;

	/** 보스 수락 뒤 처치 실패까지의 서버 기준 제한 시간(초)입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery", meta = (ClampMin = "1"))
	int32 CombatDeadlineSeconds = 1;

	/** 보스 처치 후 수취인 확인에 사용할 콘텐츠 흐름입니다. UI·레벨은 이 값을 표시·연출에만 사용합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	EPDBossRecipientInteractionType RecipientInteractionType = EPDBossRecipientInteractionType::None;

	/** @return 보스 배송 진행에 필요한 데이터가 유효하면 true입니다. */
	bool Validate(FString& OutError) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("BossDelivery"), GetFName());
	}
};
