#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PADO/Character/Enum/PDLifeState.h"
#include "PDRespawnComponent.generated.h"

class UPDHealthComponent;

/**
 * 플레이어를 새 몸으로 다시 시작시킨다. 플레이어 컨트롤러에 붙고 서버에서만 동작한다.
 *
 * 리스폰 실행(RespawnAt)과 리스폰 조건을 나눈다. 지금 조건은 "빙의한 몸이 죽으면
 * RespawnDelay 뒤 죽은 자리"다(bRespawnOnDeath). 증원 호출이나 체크포인트처럼 조건이
 * 바뀌면 이 조건을 끄고 그쪽에서 RespawnAt을 부른다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDRespawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDRespawnComponent();

	/**
	 * 서버에서 이 플레이어를 SpawnTransform 위치의 새 몸으로 다시 시작한다. 이전 몸(시체)은
	 * 제거하고 플레이어 상태(체력, 걸려 있던 효과)를 처음으로 되돌린다. 그 자리에 스폰할 수
	 * 없으면 엔진 기본 시작 지점에서 시작한다. 살아 있는 몸이 있으면 거부한다.
	 * 반환값은 새 몸이 생겼는지다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Respawn")
	bool RespawnAt(const FTransform& SpawnTransform);

	/** 사망 뒤 리스폰을 기다리는 중인가. */
	UFUNCTION(BlueprintPure, Category = "PD|Respawn")
	bool IsRespawnPending() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 빙의한 몸이 죽으면 RespawnDelay 뒤 죽은 자리에서 다시 시작한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Respawn")
	bool bRespawnOnDeath = true;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "PD|Respawn",
		meta = (ClampMin = "0.0", Units = "s", EditCondition = "bRespawnOnDeath"))
	float RespawnDelay = 5.0f;

private:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleLifeStateChanged(EPDLifeState PreviousState, EPDLifeState NewState);

	void HandleRespawnTimer();
	void WatchPawn(APawn* Pawn);
	void StopWatchingPawn();

	/** 지금 빙의한 몸의 생명 상태다. */
	TWeakObjectPtr<UPDHealthComponent> WatchedHealth;

	/** 죽은 자리다. 탄 채로 죽었으면 하차 지점이다. */
	FTransform PendingSpawnTransform;

	FTimerHandle RespawnTimerHandle;
};
