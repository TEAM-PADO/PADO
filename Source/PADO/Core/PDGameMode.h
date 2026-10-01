#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PDGameMode.generated.h"

/**
 * PADO의 기본 GameMode다.
 * 규칙과 기본 Pawn·Controller 지정은 파생 Blueprint에서 저작한다.
 */
UCLASS(Blueprintable)
class PADO_API APDGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	/**
	 * PADO 전용 GameState 클래스 등록
	 */
	APDGameMode();

	/** @return 일반·보스 배달 정의를 조회하는 코어 레지스트리입니다. 설정되지 않으면 nullptr입니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Delivery")
	class UPDDeliveryDefinitionSet* GetDeliveryDefinitionSet() const;

protected:
	/** 파생 GameMode Blueprint에서 지정하는 일반·보스 배달 정의 레지스트리입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	TObjectPtr<class UPDDeliveryDefinitionSet> DeliveryDefinitionSet;

	/** 서버 시작 시 배달 정의 데이터의 ID·필수 값 오류를 로그로 남깁니다. */
	virtual void BeginPlay() override;
};
