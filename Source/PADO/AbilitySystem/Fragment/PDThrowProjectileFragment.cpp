#include "PADO/AbilitySystem/Fragment/PDThrowProjectileFragment.h"

#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Projectile/PDActionProjectile.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/PDWorldItemActor.h"

namespace
{
	bool ValidateExplosionFragments(
		const TArray<TObjectPtr<UPDActionFragment>>& Fragments,
		EPDActionScope RequiredScope,
		const TCHAR* ListName,
		FString& OutError)
	{
		const TCHAR* ScopeName = RequiredScope == EPDActionScope::Source
			? TEXT("Source")
			: TEXT("Target");

		for (int32 Index = 0; Index < Fragments.Num(); ++Index)
		{
			const UPDActionFragment* Fragment = Fragments[Index];
			if (!IsValid(Fragment))
			{
				OutError = FString::Printf(
					TEXT("%s[%d]가 비어 있습니다."),
					ListName,
					Index);
				return false;
			}

			if (Fragment->ApplicationScope != RequiredScope)
			{
				OutError = FString::Printf(
					TEXT("%s[%d] '%s'는 %s Scope여야 합니다."),
					ListName,
					Index,
					*GetNameSafe(Fragment),
					ScopeName);
				return false;
			}

			if (!Fragment->SupportsDeferredExecution())
			{
				OutError = FString::Printf(
					TEXT("%s[%d] '%s'는 지연 실행을 지원하지 않습니다."),
					ListName,
					Index,
					*GetNameSafe(Fragment));
				return false;
			}

			FString FragmentError;
			if (!Fragment->Validate(FragmentError))
			{
				OutError = FString::Printf(
					TEXT("%s[%d] '%s'가 유효하지 않습니다: %s"),
					ListName,
					Index,
					*GetNameSafe(Fragment),
					*FragmentError);
				return false;
			}
		}

		return true;
	}
}

UPDThrowProjectileFragment::UPDThrowProjectileFragment()
{
	ApplicationScope = EPDActionScope::Source;
	bRequired = true;
	LaunchConfig.ProjectileClass = APDActionProjectile::StaticClass();
}

bool UPDThrowProjectileFragment::Validate(FString& OutError) const
{
	FString LaunchError;
	if (!LaunchConfig.Validate(LaunchError))
	{
		OutError = FString::Printf(
			TEXT("LaunchConfig가 유효하지 않습니다: %s"),
			*LaunchError);
		return false;
	}

	FString ExplosionError;
	if (!ExplosionConfig.Validate(ExplosionError))
	{
		OutError = FString::Printf(
			TEXT("ExplosionConfig가 유효하지 않습니다: %s"),
			*ExplosionError);
		return false;
	}

	return ValidateExplosionFragments(
			ExplosionTargetFragments,
			EPDActionScope::Target,
			TEXT("ExplosionTargetFragments"),
			OutError) &&
		ValidateExplosionFragments(
			ExplosionPresentationFragments,
			EPDActionScope::Source,
			TEXT("ExplosionPresentationFragments"),
			OutError);
}
bool UPDThrowProjectileFragment::DeclaresSetByCallerTag(
	FGameplayTag DataTag) const
{
	const auto DeclaresTag = [DataTag](const UPDActionFragment* Fragment)
	{
		return IsValid(Fragment) && Fragment->DeclaresSetByCallerTag(DataTag);
	};

	return ExplosionTargetFragments.ContainsByPredicate(DeclaresTag) ||
		ExplosionPresentationFragments.ContainsByPredicate(DeclaresTag);
}
void UPDThrowProjectileFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
	for (const UPDActionFragment* Fragment : ExplosionTargetFragments)
	{
		if (IsValid(Fragment))
		{
			Fragment->AppendDeclaredSetByCallerTags(OutTags);
		}
	}

	for (const UPDActionFragment* Fragment : ExplosionPresentationFragments)
	{
		if (IsValid(Fragment))
		{
			Fragment->AppendDeclaredSetByCallerTags(OutTags);
		}
	}
}
bool UPDThrowProjectileFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	AActor* LaunchOrigin = ResolveLaunchOrigin(Context);
	if (!Context.IsAuthoritative() || !Context.SourceAbilitySystem ||
		!IsValid(Context.SourceActor) || !IsValid(LaunchOrigin) ||
		!LaunchOrigin->GetWorld())
	{
		OutError = TEXT("투사체를 생성할 서버 권한, Source ASC 또는 생성 원점이 없습니다.");
		return false;
	}

	return true;
}

bool UPDThrowProjectileFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	FString ExecutionError;
	if (!CanExecute(Context, ExecutionError))
	{
		return false;
	}

	AActor* LaunchOrigin = ResolveLaunchOrigin(Context);
	FTransform SpawnTransform;
	FVector InitialVelocity;
	if (!BuildLaunchSolution(
		Context.SourceActor,
		LaunchOrigin,
		Context.InputChargeAlpha,
		SpawnTransform,
		InitialVelocity))
	{
		return false;
	}

	UWorld* World = LaunchOrigin->GetWorld();
	APDActionProjectile* Projectile =
		World->SpawnActorDeferred<APDActionProjectile>(
			LaunchConfig.ProjectileClass,
			SpawnTransform,
			Context.SourceActor,
			Cast<APawn>(Context.SourceActor),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return false;
	}

	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	if (!Projectile->InitializeProjectile(
		LaunchConfig,
		ExplosionConfig,
		ExplosionTargetFragments,
		ExplosionPresentationFragments,
		Context.SourceAbilitySystem,
		ResolveEffectSourceObject(Context, LaunchOrigin),
		Context.SourceActor,
		Cast<APawn>(Context.SourceActor),
		InitialVelocity))
	{
		Projectile->Destroy();
		return false;
	}

	return true;
}

bool UPDThrowProjectileFragment::BuildLaunchSolution(
	const AActor* SourceActor,
	const AActor* LaunchOrigin,
	float ChargeAlpha,
	FTransform& OutSpawnTransform,
	FVector& OutInitialVelocity) const
{
	OutSpawnTransform = FTransform::Identity;
	OutInitialVelocity = FVector::ZeroVector;
	if (!IsValid(SourceActor) || !IsValid(LaunchOrigin))
	{
		return false;
	}

	FVector Forward = SourceActor->GetActorForwardVector();
	if (LaunchConfig.bUseSourceAim)
	{
		if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
		{
			Forward = SourcePawn->GetBaseAimRotation().Vector();
		}
	}
	if (!Forward.Normalize())
	{
		return false;
	}

	FVector LaunchLocation = LaunchOrigin->GetActorLocation();
	if (!LaunchConfig.LaunchSocketName.IsNone())
	{
		const APDWorldItemActor* SourceItem =
			Cast<APDWorldItemActor>(LaunchOrigin);
		const UMeshComponent* ItemMesh = SourceItem
			? SourceItem->GetItemMesh()
			: nullptr;
		if (ItemMesh && ItemMesh->DoesSocketExist(LaunchConfig.LaunchSocketName))
		{
			LaunchLocation =
				ItemMesh->GetSocketLocation(LaunchConfig.LaunchSocketName);
		}
	}

	const FVector SpawnLocation = LaunchLocation +
		Forward * LaunchConfig.SpawnForwardOffset +
		FVector::UpVector * LaunchConfig.SpawnUpOffset;
	OutSpawnTransform = FTransform(Forward.Rotation(), SpawnLocation);
	OutInitialVelocity =
		Forward * LaunchConfig.ResolveForwardSpeed(ChargeAlpha) +
		FVector::UpVector * LaunchConfig.UpwardSpeed;
	return !OutInitialVelocity.ContainsNaN();
}

AActor* UPDThrowProjectileFragment::ResolveLaunchOrigin(
	const FPDActionExecutionContext& Context) const
{
	UObject* SourceObject = Context.Ability
		? Context.Ability->GetCurrentSourceObject()
		: nullptr;
	if (const UActorComponent* SourceComponent =
		Cast<UActorComponent>(SourceObject))
	{
		if (AActor* ComponentOwner = SourceComponent->GetOwner())
		{
			return ComponentOwner;
		}
	}
	if (AActor* SourceObjectActor = Cast<AActor>(SourceObject))
	{
		return SourceObjectActor;
	}

	return Context.SourceActor;
}

UObject* UPDThrowProjectileFragment::ResolveEffectSourceObject(
	const FPDActionExecutionContext& Context,
	AActor* LaunchOrigin) const
{
	if (const APDWorldItemActor* SourceItem =
		Cast<APDWorldItemActor>(LaunchOrigin))
	{
		if (UObject* ItemDefinition = SourceItem->GetItemDefinition())
		{
			return ItemDefinition;
		}
	}

	if (Context.Ability && Context.Ability->GetCurrentSourceObject())
	{
		return Context.Ability->GetCurrentSourceObject();
	}
	return LaunchOrigin;
}
