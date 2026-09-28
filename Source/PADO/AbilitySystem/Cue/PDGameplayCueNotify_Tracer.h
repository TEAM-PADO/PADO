#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "PDGameplayCueNotify_Tracer.generated.h"

class UNiagaraSystem;

/**
 * 총구에서 이번 발이 멈춘 곳까지 날아가는 탄을 재생하고, 막힌 탄이면
 * 도착하는 시각에 끝점에서 탄착 연출을 재생한다.
 *
 * Execute Gameplay Cue Fragment의 Play At Shot End로 실행한다. 끝점과 막힘
 * 여부는 Cue의 HitResult가 알려 준다. 출발점은 재생하는 쪽이 자기 화면의
 * 아이템 소켓에서 직접 구한다. 서버가 구한 소켓 위치는 다른 클라이언트의
 * 애니메이션 포즈와 다를 수 있다.
 *
 * 연사 무기용이라 실행마다 액터를 스폰하지 않는 Static Notify다. 탄착 지연은
 * 월드 타이머로 건다. 트레이서 입자 상태에 묶지 않으므로 트레이서가 컬링돼도
 * 탄착은 제자리에 제시간에 나온다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "PD Tracer Cue Notify"))
class PADO_API UPDGameplayCueNotify_Tracer : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) const override;

	/**
	 * 날아가는 탄이다. User.TracerEnd(Position), User.TracerSpeed,
	 * User.TracerLength를 받아 스폰 위치에서 TracerEnd까지 날아가야 한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracer")
	TObjectPtr<UNiagaraSystem> TracerSystem;

	/** 탄이 출발하는 아이템 메시 소켓이다. 찾지 못하면 판정 시작점에서 출발한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracer")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 탄이 날아가는 속도다. 탄착 지연도 이 값으로 계산하므로 값은 여기 하나뿐이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Tracer",
		meta = (ClampMin = "1.0", UIMin = "1000.0", UIMax = "100000.0", Units = "cm/s"))
	float TracerSpeed = 20000.0f;

	/** 탄의 길이다. 머리가 끝점에서 멈춘 뒤 꼬리가 따라 들어오면 사라진다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Tracer",
		meta = (ClampMin = "0.0", UIMax = "1000.0", Units = "cm"))
	float TracerLength = 300.0f;

	/** 막힌 탄이 도착할 때 끝점에서 재생한다. 비우면 탄착 연출이 없다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	/** 출발점이다. 아이템 소켓을 찾지 못하면 판정 시작점으로 물러선다. */
	FVector ResolveMuzzleLocation(
		const FGameplayCueParameters& Parameters,
		const FHitResult& ShotHit) const;
};
