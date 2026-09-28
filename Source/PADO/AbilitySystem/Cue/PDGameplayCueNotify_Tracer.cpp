#include "PADO/AbilitySystem/Cue/PDGameplayCueNotify_Tracer.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "TimerManager.h"

namespace PDTracerCue
{
	// 트레이서 Niagara System이 받는 User 파라미터다. User. 접두사는 컴포넌트가 붙인다.
	const FName TracerEndParameter(TEXT("TracerEnd"));
	const FName TracerSpeedParameter(TEXT("TracerSpeed"));
	const FName TracerLengthParameter(TEXT("TracerLength"));

	void SpawnImpact(
		UWorld* World,
		UNiagaraSystem* ImpactSystem,
		const FVector& Location,
		const FRotator& Rotation)
	{
		if (World && ImpactSystem)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				ImpactSystem,
				Location,
				Rotation,
				FVector::OneVector,
				/*bAutoDestroy=*/ true,
				/*bAutoActivate=*/ true,
				ENCPoolMethod::AutoRelease);
		}
	}
}

bool UPDGameplayCueNotify_Tracer::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters) const
{
	UWorld* World = MyTarget ? MyTarget->GetWorld() : nullptr;
	const FHitResult* ShotHit = Parameters.EffectContext.GetHitResult();
	if (!World || !ShotHit)
	{
		return false;
	}

	// Fragment는 빗나간 탄도 사거리 끝을 ImpactPoint에 담아 보낸다.
	const FVector End = ShotHit->ImpactPoint;
	const FVector Start = ResolveMuzzleLocation(Parameters, *ShotHit);
	const FVector ToEnd = End - Start;
	const double Distance = ToEnd.Size();

	if (TracerSystem && Distance > UE_KINDA_SMALL_NUMBER)
	{
		// 총구에 붙이지 않는다. 붙이면 날아가는 동안 출발점이 총을 따라 움직인다.
		UNiagaraComponent* Tracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			TracerSystem,
			Start,
			ToEnd.Rotation(),
			FVector::OneVector,
			/*bAutoDestroy=*/ true,
			/*bAutoActivate=*/ false,
			ENCPoolMethod::AutoRelease);
		if (Tracer)
		{
			// 비행 시간은 활성화할 때 한 번 계산하므로 값을 먼저 넣는다.
			Tracer->SetVariablePosition(PDTracerCue::TracerEndParameter, End);
			Tracer->SetVariableFloat(PDTracerCue::TracerSpeedParameter, TracerSpeed);
			Tracer->SetVariableFloat(PDTracerCue::TracerLengthParameter, TracerLength);
			Tracer->Activate(true);
		}
	}

	if (ImpactSystem && ShotHit->bBlockingHit)
	{
		// 탄 머리가 끝점에 닿는 시각이다. 트레이서와 같은 입력으로 계산할 뿐
		// 트레이서 입자에 의존하지 않는다.
		const float Delay = static_cast<float>(Distance / FMath::Max(TracerSpeed, 1.0f));
		const FRotator ImpactRotation = ShotHit->ImpactNormal.Rotation();
		if (Delay <= UE_KINDA_SMALL_NUMBER)
		{
			PDTracerCue::SpawnImpact(World, ImpactSystem, End, ImpactRotation);
		}
		else
		{
			FTimerHandle ImpactTimer;
			World->GetTimerManager().SetTimer(
				ImpactTimer,
				FTimerDelegate::CreateWeakLambda(
					World,
					[WeakWorld = TWeakObjectPtr<UWorld>(World),
						WeakImpact = TWeakObjectPtr<UNiagaraSystem>(ImpactSystem.Get()),
						End,
						ImpactRotation]()
					{
						PDTracerCue::SpawnImpact(
							WeakWorld.Get(),
							WeakImpact.Get(),
							End,
							ImpactRotation);
					}),
				Delay,
				false);
		}
	}

	return true;
}

FVector UPDGameplayCueNotify_Tracer::ResolveMuzzleLocation(
	const FGameplayCueParameters& Parameters,
	const FHitResult& ShotHit) const
{
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(Parameters.GetEffectCauser());
	const UMeshComponent* ItemMesh = Item ? Item->GetItemMesh() : nullptr;
	if (ItemMesh && !MuzzleSocketName.IsNone() && ItemMesh->DoesSocketExist(MuzzleSocketName))
	{
		return ItemMesh->GetSocketLocation(MuzzleSocketName);
	}

	// 판정 시작점은 서버 값이라 이 화면의 총구와 조금 어긋날 수 있다.
	// 끝점은 그대로이므로 탄은 맞은 자리에 떨어진다.
	return ShotHit.TraceStart;
}
