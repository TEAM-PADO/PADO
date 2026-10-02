#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PADO/Save/Struct/PDRoomPersistentState.h"
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

	/**
	 * 서버의 현재 방 공용 상태를 영속 가능한 값 스냅샷으로 복사합니다.
	 * 진행 중 배달 같은 런타임 상태의 복원 정책은 이후 기획 확정 시 이 함수에서 확장합니다.
	 */
	bool CaptureRoomPersistentState(FPDRoomPersistentState& OutPersistentState, FString& OutError) const;

	/**
	 * 불러온 방 공용 상태를 서버에 적용합니다.
	 * 일반 저장 재개 정책에 따라 활성 배달은 초기화하며, 플레이어 배치는 별도 복원 흐름이 담당합니다.
	 */
	bool RestoreRoomPersistentState(const FPDRoomPersistentState& PersistentState, FString& OutError);

	/**
	 * 현재 서버의 공용 상태를 캡처해 선택된 방 저장본에 기록하도록 요청합니다.
	 * 실제 저장 완료 여부는 UPDRoomSaveSubsystem의 OnRoomSaveCompleted 이벤트에서 확인해야 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool RequestRoomSave();

protected:
	/** 파생 GameMode Blueprint에서 지정하는 일반·보스 배달 정의 레지스트리입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Delivery")
	TObjectPtr<class UPDDeliveryDefinitionSet> DeliveryDefinitionSet;

	/** 서버 시작 시 배달 정의 데이터의 ID·필수 값 오류를 로그로 남깁니다. */
	virtual void BeginPlay() override;
};
