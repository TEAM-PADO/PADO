#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PADO/Delivery/Struct/PDActiveDeliveryState.h"
#include "PDGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDActiveDeliveryStateChangedSignature, const FPDActiveDeliveryState&, NewActiveDeliveryState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDCompletedGeneralDeliveryCountChangedSignature, int32, NewCompletedGeneralDeliveryCount);

/**
 * 방 전체가 공유하는 서버 권위 게임 진행 상태입니다.
 */
UCLASS(Blueprintable)
class PADO_API APDGameState : public AGameState
{
	GENERATED_BODY()

public:
	/**
	 * 모든 클라이언트에 복제된 현재 배달 상태의 사본을 반환합니다.
	 * 시간 값은 서버 기준 절대 시각입니다.
	 */
	UFUNCTION(BlueprintPure, Category = "PADO|Delivery")
	FPDActiveDeliveryState GetActiveDeliveryState() const;

	/**
	 * 완료한 일반 배달 횟수를 반환합니다.
	 * 이 값은 방 전체가 공유하며 모든 클라이언트에 복제됩니다.
	 */
	UFUNCTION(BlueprintPure, Category = "PADO|Delivery")
	int32 GetCompletedGeneralDeliveryCount() const;

	/**
	 * 서버 권위로 현재 배달 상태를 설정하고 변경 이벤트를 발생시킵니다.
	 * 클라이언트 호출은 무시됩니다.
	 *
	 * @param NewActiveDeliveryState 적용할 새 공유 배달 상태입니다.
	 */
	void SetActiveDeliveryState(const FPDActiveDeliveryState& NewActiveDeliveryState);

	/**
	 * 서버 권위로 현재 배달 상태를 None 상태로 초기화합니다.
	 * 클라이언트 호출은 무시됩니다.
	 */
	void ClearActiveDeliveryState();

	/**
	 * 서버 권위로 완료 일반 배달 횟수를 설정합니다.
	 * 음수 값은 0으로 보정하며, 클라이언트 호출은 무시됩니다.
	 *
	 * @param NewCompletedGeneralDeliveryCount 설정할 완료 횟수입니다.
	 */
	void SetCompletedGeneralDeliveryCount(int32 NewCompletedGeneralDeliveryCount);

	/**
	 * ActiveDeliveryState가 변경되었을 때 서버와 각 클라이언트에서 발생합니다.
	 * UI는 이 델리게이트를 구독해 배달 정보를 갱신합니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Delivery")
	FPDActiveDeliveryStateChangedSignature OnActiveDeliveryStateChanged;

	/** 완료 일반 배달 횟수가 변경되었을 때 발생하는 UI 갱신용 이벤트입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Delivery")
	FPDCompletedGeneralDeliveryCountChangedSignature OnCompletedGeneralDeliveryCountChanged;

protected:
	/** GameState의 공유 상태를 모든 접속 플레이어에게 복제하도록 등록합니다. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** ActiveDeliveryState 복제 수신 또는 서버 변경 직후 UI 갱신 이벤트를 발생시킵니다. */
	UFUNCTION()
	void OnRep_ActiveDeliveryState();

	/** 완료 일반 배달 횟수 복제 수신 또는 서버 변경 직후 변경 이벤트를 발생시킵니다. */
	UFUNCTION()
	void OnRep_CompletedGeneralDeliveryCount(int32 PreviousCompletedGeneralDeliveryCount);

private:
	/** 서버만 변경하며, 방의 모든 플레이어에게 복제하는 현재 배달 진행 상태입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveDeliveryState)
	FPDActiveDeliveryState ActiveDeliveryState;

	/** 서버만 변경하며, 방의 모든 플레이어에게 복제하는 완료 일반 배달 수입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_CompletedGeneralDeliveryCount)
	int32 CompletedGeneralDeliveryCount = 0;
};
