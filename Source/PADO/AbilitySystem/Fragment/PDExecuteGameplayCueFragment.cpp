#include "PADO/AbilitySystem/Fragment/PDExecuteGameplayCueFragment.h"

#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/Item/PDWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDGameplayCueFragment, Log, All);

bool UPDExecuteGameplayCueFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!CueTag.IsValid())
	{
		OutError = TEXT("CueTag가 비어 있습니다.");
		return false;
	}

	const FGameplayTag GameplayCueRoot =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayCue"), false);
	if (!GameplayCueRoot.IsValid() || !CueTag.MatchesTag(GameplayCueRoot))
	{
		OutError = TEXT("CueTag는 GameplayCue 하위 태그여야 합니다.");
		return false;
	}

	// 대상은 서버가 정한다. 클라이언트가 미리 고른 대상에 연출을 붙이면
	// 서버가 다른 대상을 고른 경우 엉뚱한 곳에서 터진다.
	if (bPredictOnOwningClient && ApplicationScope == EPDActionScope::Target)
	{
		OutError = TEXT(
			"Target Scope Cue는 예측 재생할 수 없습니다. 대상은 서버가 정합니다.");
		return false;
	}

	// 소켓 위치로 덮으면 이번 발이 멈춘 곳이 사라진다.
	if (bPlayAtShotEnd && !ItemSocketName.IsNone())
	{
		OutError = TEXT(
			"이번 발이 멈춘 곳에서 재생하는 Cue는 ItemSocketName을 쓸 수 없습니다.");
		return false;
	}

	return true;
}

bool UPDExecuteGameplayCueFragment::SupportsDeferredExecution() const
{
	return true;
}

bool UPDExecuteGameplayCueFragment::SupportsLocalPrediction() const
{
	return bPredictOnOwningClient;
}

bool UPDExecuteGameplayCueFragment::RequiresShotResult() const
{
	return bPlayAtShotEnd;
}

bool UPDExecuteGameplayCueFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.IsAuthoritativeOrPredicting() ||
		!Context.ResolveScopedAbilitySystem(ApplicationScope) ||
		!IsValid(Context.ResolveScopedActor(ApplicationScope)))
	{
		OutError = TEXT("GameplayCue를 실행할 권한, ASC 또는 대상 Actor가 없습니다.");
		return false;
	}

	if (bPlayAtShotEnd && !Context.bHasShotResult)
	{
		OutError = TEXT("이번 발 결과가 없어 멈춘 곳에서 재생할 수 없습니다.");
		return false;
	}

	return true;
}

bool UPDExecuteGameplayCueFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	UAbilitySystemComponent* ScopedAbilitySystem =
		Context.ResolveScopedAbilitySystem(ApplicationScope);
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritativeOrPredicting() || !ScopedAbilitySystem ||
		!IsValid(ScopedActor) || !CueTag.IsValid())
	{
		return false;
	}

	const FHitResult* CueHit = ResolveCueHit(Context);
	if (bPlayAtShotEnd && !CueHit)
	{
		return false;
	}

	// 빗나간 탄은 허공에서 멈췄다. 탄착 연출을 거르는 것은 실패가 아니다.
	if (bPlayAtShotEnd && bOnlyWhenShotBlocked && !CueHit->bBlockingHit)
	{
		return true;
	}

	UObject* SourceObject = Context.Ability
		? Context.Ability->GetCurrentSourceObject()
		: Context.EffectSourceObject;
	AActor* EffectCauser = Cast<AActor>(SourceObject);
	if (!EffectCauser)
	{
		if (const UActorComponent* SourceComponent =
			Cast<UActorComponent>(SourceObject))
		{
			EffectCauser = SourceComponent->GetOwner();
		}
	}
	if (!EffectCauser)
	{
		EffectCauser = Context.SourceActor;
	}

	/**
	 * Cue Notify는 HitResult가 있으면 그 ImpactNormal을, 없으면 CueParameters.Normal을
	 * 회전으로 쓴다. 방향을 지정했다면 두 경로가 어긋나지 않도록 둘 다 덮어쓴다.
	 */
	const FVector CueDirection = ResolveDirection(Context);
	const bool bHasCueDirection = !CueDirection.IsNearlyZero();

	/** 위치도 같은 이유로 HitResult와 CueParameters.Location을 함께 소켓으로 맞춘다. */
	FVector ItemSocketLocation = FVector::ZeroVector;
	const bool bHasItemSocketLocation =
		ResolveItemSocketLocation(EffectCauser, ItemSocketLocation);

	FHitResult CueHitResult = CueHit ? *CueHit : FHitResult();
	if (bHasCueDirection)
	{
		CueHitResult.Normal = CueDirection;
		CueHitResult.ImpactNormal = CueDirection;
	}
	if (bHasItemSocketLocation)
	{
		CueHitResult.Location = ItemSocketLocation;
		CueHitResult.ImpactPoint = ItemSocketLocation;
	}

	FGameplayEffectContextHandle EffectContext =
		Context.SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(Context.SourceActor, EffectCauser);
	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}
	if (CueHit)
	{
		EffectContext.AddHitResult(CueHitResult, true);
	}

	FGameplayCueParameters CueParameters(EffectContext);
	CueParameters.Instigator = Context.SourceActor;
	CueParameters.EffectCauser = EffectCauser;
	CueParameters.SourceObject = SourceObject;
	CueParameters.AbilityLevel = Context.Ability
		? Context.Ability->GetAbilityLevel()
		: 1;
	CueParameters.bReplicateLocationWhenUsingMinimalRepProxy = true;

	if (CueHit)
	{
		CueParameters.Location = CueHitResult.ImpactPoint;
		CueParameters.Normal = CueHitResult.ImpactNormal;
		CueParameters.PhysicalMaterial = CueHitResult.PhysMaterial.Get();
	}
	else
	{
		CueParameters.Location = bHasItemSocketLocation
			? ItemSocketLocation
			: ScopedActor->GetActorLocation();
		if (bHasCueDirection)
		{
			CueParameters.Normal = CueDirection;
		}
	}

	ScopedAbilitySystem->ExecuteGameplayCue(CueTag, CueParameters);
	return true;
}

FVector UPDExecuteGameplayCueFragment::ResolveDirection(
	const FPDActionExecutionContext& Context) const
{
	FVector Direction = FVector::ZeroVector;
	switch (DirectionMode)
	{
	case EPDGameplayCueDirectionMode::SourceForward:
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EPDGameplayCueDirectionMode::SourceAim:
		if (const APawn* SourcePawn = Cast<APawn>(Context.SourceActor))
		{
			Direction = SourcePawn->GetBaseAimRotation().Vector();
		}
		else if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EPDGameplayCueDirectionMode::SourceToTarget:
		if (Context.SourceActor && Context.TargetActor)
		{
			Direction = Context.TargetActor->GetActorLocation() -
				Context.SourceActor->GetActorLocation();
		}
		break;

	case EPDGameplayCueDirectionMode::InverseHitNormal:
		if (const FHitResult* CueHit = ResolveCueHit(Context))
		{
			Direction = -CueHit->ImpactNormal;
		}
		break;

	case EPDGameplayCueDirectionMode::FromContext:
	default:
		return FVector::ZeroVector;
	}

	if (bFlattenDirection)
	{
		Direction.Z = 0.0f;
	}

	if (!Direction.Normalize())
	{
		// 방향을 못 구했으면 소스 정면으로 물러선다.
		if (!Context.SourceActor)
		{
			return FVector::ZeroVector;
		}

		Direction = Context.SourceActor->GetActorForwardVector();
		if (bFlattenDirection)
		{
			Direction.Z = 0.0f;
		}
		if (!Direction.Normalize())
		{
			return FVector::ZeroVector;
		}
	}

	// 오프셋은 구한 방향의 로컬 프레임에서 돈다. 물러선 방향에도 똑같이 적용한다.
	return DirectionOffset.IsNearlyZero()
		? Direction
		: (Direction.ToOrientationQuat() * DirectionOffset.Quaternion())
			.GetForwardVector();
}

const FHitResult* UPDExecuteGameplayCueFragment::ResolveCueHit(
	const FPDActionExecutionContext& Context) const
{
	if (bPlayAtShotEnd)
	{
		return Context.bHasShotResult ? &Context.ShotResult : nullptr;
	}

	return Context.bHasHitResult ? &Context.HitResult : nullptr;
}

bool UPDExecuteGameplayCueFragment::ResolveItemSocketLocation(
	const AActor* EffectCauser,
	FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;
	if (ItemSocketName.IsNone())
	{
		return false;
	}

	const APDWorldItemActor* ItemActor = Cast<APDWorldItemActor>(EffectCauser);
	const UMeshComponent* ItemMesh = ItemActor
		? ItemActor->GetItemMesh()
		: nullptr;
	if (!IsValid(ItemMesh) || !ItemMesh->DoesSocketExist(ItemSocketName))
	{
		UE_LOG(
			LogPDGameplayCueFragment,
			Warning,
			TEXT("Execute Gameplay Cue [%s]: 원본 아이템 메시에서 소켓 '%s'을 찾지 못해 기본 위치에서 재생합니다."),
			*CueTag.ToString(),
			*ItemSocketName.ToString());
		return false;
	}

	OutLocation = ItemMesh->GetSocketLocation(ItemSocketName);
	return !OutLocation.ContainsNaN();
}
