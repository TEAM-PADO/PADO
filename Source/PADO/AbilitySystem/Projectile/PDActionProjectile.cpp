#include "PADO/AbilitySystem/Projectile/PDActionProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDActionProjectile, Log, All);

APDActionProjectile::APDActionProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent =
		CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(18.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("PDProjectile"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(
		this,
		&APDActionProjectile::HandleBlockingHit);

	ProjectileMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMeshComponent->SetupAttachment(CollisionComponent);
	ProjectileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMeshComponent->SetGenerateOverlapEvents(false);
	ProjectileMeshComponent->SetCanEverAffectNavigation(false);

	ProjectileMovementComponent =
		CreateDefaultSubobject<UProjectileMovementComponent>(
			TEXT("ProjectileMovement"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Bounciness = 0.55f;
	ProjectileMovementComponent->Friction = 0.2f;
	ProjectileMovementComponent->ProjectileGravityScale = 1.0f;
	ProjectileMovementComponent->InitialSpeed = 0.0f;
	ProjectileMovementComponent->MaxSpeed = 0.0f;
	// 서버 업데이트 사이를 메꾸기 위한 기본값이다. BP 파생 클래스에서 조정할 수 있고,
	// 실제 보간 대상 연결은 클라이언트 BeginPlay에서 한다.
	ProjectileMovementComponent->bInterpMovement = true;
	ProjectileMovementComponent->bInterpRotation = true;
}

void APDActionProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 서버 위치 복제를 단일 진실로 사용하고 Simulated Proxy는 충돌 판정을 하지 않는다.
	if (!HasAuthority())
	{
		// 시뮬레이션만 멈춘다. 컴포넌트 틱까지 끄면 보간이 돌지 못해
		// 서버 업데이트가 도착할 때마다 위치가 그대로 튄다.
		ProjectileMovementComponent->bSimulationEnabled = false;
		ProjectileMovementComponent->SetInterpolatedComponent(ProjectileMeshComponent);
		RefreshSourceMovementIgnore(
			GetOwner(),
			GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
	}
}

void APDActionProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FuseTimerHandle);
	}
	ClearSourceMovementIgnore();
	Super::EndPlay(EndPlayReason);
}

void APDActionProjectile::OnRep_Owner()
{
	Super::OnRep_Owner();
	RefreshSourceMovementIgnore(
		GetOwner(),
		GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
}

void APDActionProjectile::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	RefreshSourceMovementIgnore(
		GetOwner(),
		GetInstigator() ? GetInstigator() : Cast<APawn>(GetOwner()));
}

void APDActionProjectile::PostNetReceiveLocationAndRotation()
{
	// 기본 구현은 SetActorLocationAndRotation()으로 즉시 대입하므로 업데이트마다 튄다.
	// 보간 대상이 연결된 클라이언트에서는 목표만 갱신하고 메시가 따라오게 한다.
	if (!ProjectileMovementComponent ||
		!ProjectileMovementComponent->bInterpMovement ||
		!ProjectileMovementComponent->GetInterpolatedComponent())
	{
		Super::PostNetReceiveLocationAndRotation();
		return;
	}

	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	ProjectileMovementComponent->MoveInterpolationTarget(
		FRepMovement::RebaseOntoLocalOrigin(LocalRepMovement.Location, this),
		LocalRepMovement.Rotation);
}

bool APDActionProjectile::InitializeProjectile(
	const FPDProjectileLaunchConfigStruct& LaunchConfig,
	const FPDProjectileExplosionConfigStruct& ExplosionConfig,
	const TArray<TObjectPtr<UPDActionFragment>>& ExplosionTargetFragments,
	const TArray<TObjectPtr<UPDActionFragment>>& ExplosionPresentationFragments,
	UAbilitySystemComponent* SourceAbilitySystem,
	UObject* EffectSourceObject,
	AActor* SourceActor,
	APawn* SourcePawn,
	const FVector& InitialVelocity)
{
	FString LaunchError;
	FString ExplosionError;
	if (!HasAuthority() || bInitialized || !SourceAbilitySystem ||
		!IsValid(SourceActor) || InitialVelocity.ContainsNaN() ||
		!LaunchConfig.Validate(LaunchError) ||
		!ExplosionConfig.Validate(ExplosionError))
	{
		return false;
	}

	FString FragmentsError;
	if (!InitializeExplosionFragments(
			ExplosionTargetFragments,
			EPDActionScope::Target,
			TEXT("ExplosionTargetFragments"),
			SourceAbilitySystem,
			EffectSourceObject,
			SourceActor,
			ActiveExplosionTargetFragments,
			FragmentsError) ||
		!InitializeExplosionFragments(
			ExplosionPresentationFragments,
			EPDActionScope::Source,
			TEXT("ExplosionPresentationFragments"),
			SourceAbilitySystem,
			EffectSourceObject,
			SourceActor,
			ActiveExplosionPresentationFragments,
			FragmentsError))
	{
		UE_LOG(
			LogPDActionProjectile,
			Warning,
			TEXT("투사체 폭발 Fragment 초기화에 실패했습니다: %s"),
			*FragmentsError);
		return false;
	}

	ActiveExplosionConfig = ExplosionConfig;
	ActiveEffectSourceObject = EffectSourceObject;
	ActiveSourceAbilitySystem = SourceAbilitySystem;
	ReplicatedProjectileMesh = LaunchConfig.ProjectileMesh;
	ReplicatedProjectileMeshScale = LaunchConfig.ProjectileMeshScale;
	ReplicatedCollisionRadius = LaunchConfig.CollisionRadius;
	ApplyPresentation();

	CollisionComponent->SetCollisionProfileName(TEXT("PDProjectile"));
	RefreshSourceMovementIgnore(SourceActor, SourcePawn);

	ProjectileMovementComponent->ProjectileGravityScale = LaunchConfig.GravityScale;
	ProjectileMovementComponent->bShouldBounce = LaunchConfig.bShouldBounce;
	ProjectileMovementComponent->Bounciness = LaunchConfig.Bounciness;
	ProjectileMovementComponent->Friction = LaunchConfig.Friction;
	ProjectileMovementComponent->Velocity = InitialVelocity;
	ProjectileMovementComponent->Activate(true);

	bInitialized = true;
	SetLifeSpan(ExplosionConfig.MaximumLifetime);
	if (ExplosionConfig.UsesFuse())
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&APDActionProjectile::HandleFuseExpired,
			ExplosionConfig.FuseDuration,
			false);
	}

	ForceNetUpdate();
	return true;
}

void APDActionProjectile::HandleFuseExpired()
{
	Detonate();
}

bool APDActionProjectile::Detonate()
{
	if (!HasAuthority() || !bInitialized || bHasDetonated)
	{
		return false;
	}

	bHasDetonated = true;
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	ProjectileMovementComponent->StopMovementImmediately();
	ProjectileMovementComponent->Deactivate();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TArray<AActor*> Targets;
	GatherExplosionTargets(Targets);
	for (AActor* Target : Targets)
	{
		ApplyExplosionToTarget(Target);
	}

	if (ActiveExplosionConfig.bDrawDebugExplosion)
	{
		DrawDebugExplosion(Targets);
	}

	PlayExplosionPresentation();
	Destroy();
	return true;
}

USphereComponent* APDActionProjectile::GetCollisionComponent() const
{
	return CollisionComponent;
}

UProjectileMovementComponent* APDActionProjectile::GetProjectileMovement() const
{
	return ProjectileMovementComponent;
}

AActor* APDActionProjectile::GetIgnoredSourceActor() const
{
	return IgnoredSourceActor.Get();
}

void APDActionProjectile::HandleBlockingHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || bHasDetonated ||
		OtherActor == IgnoredSourceActor.Get())
	{
		return;
	}

	if (ActiveExplosionConfig.ExplodesOnImpact())
	{
		Detonate();
	}
}

void APDActionProjectile::OnRep_Presentation()
{
	ApplyPresentation();
}

void APDActionProjectile::ApplyPresentation()
{
	ProjectileMeshComponent->SetStaticMesh(ReplicatedProjectileMesh);
	ProjectileMeshComponent->SetRelativeScale3D(ReplicatedProjectileMeshScale);
	CollisionComponent->SetSphereRadius(ReplicatedCollisionRadius, true);
}

void APDActionProjectile::GatherExplosionTargets(
	TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float RadiusSquared = FMath::Square(
		ActiveExplosionConfig.ExplosionRadius);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Target = *It;
		if (!IsValid(Target) ||
			FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) >
				RadiusSquared ||
			(!ActiveExplosionConfig.bAffectInstigator && Target == GetInstigator()) ||
			!HasLineOfSightTo(Target))
		{
			continue;
		}

		OutTargets.Add(Target);
		if (ActiveExplosionConfig.MaximumTargets > 0 &&
			OutTargets.Num() >= ActiveExplosionConfig.MaximumTargets)
		{
			break;
		}
	}
}

bool APDActionProjectile::HasLineOfSightTo(const AActor* TargetActor) const
{
	if (!ActiveExplosionConfig.bRequireLineOfSight || !TargetActor)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PDProjectileLineOfSight));
	QueryParams.AddIgnoredActor(this);
	if (const AActor* SourceActor = IgnoredSourceActor.Get())
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit,
		GetActorLocation(),
		TargetActor->GetActorLocation(),
		ECC_Visibility,
		QueryParams);
	return !bBlocked || Hit.GetActor() == TargetActor;
}

void APDActionProjectile::ApplyExplosionToTarget(AActor* TargetActor) const
{
	FPDActionExecutionContext Context;
	Context.SourceAbilitySystem = ActiveSourceAbilitySystem;
	Context.TargetAbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	// 지연 Fragment의 Source는 원본 Ability가 아니라 실제 효과 원점인 투사체다.
	Context.SourceActor = const_cast<APDActionProjectile*>(this);
	Context.TargetActor = TargetActor;
	Context.EffectSourceObject = ActiveEffectSourceObject;
	ExecuteExplosionFragments(ActiveExplosionTargetFragments, Context);
}

void APDActionProjectile::PlayExplosionPresentation() const
{
	// 연출도 투사체를 원점으로 삼아 폭발 지점에서 Cue가 재생되게 한다.
	// 클라이언트 전파는 Cue를 실행하는 소스 ASC가 맡는다.
	FPDActionExecutionContext Context;
	Context.SourceAbilitySystem = ActiveSourceAbilitySystem;
	Context.SourceActor = const_cast<APDActionProjectile*>(this);
	Context.EffectSourceObject = ActiveEffectSourceObject;
	ExecuteExplosionFragments(ActiveExplosionPresentationFragments, Context);
}

void APDActionProjectile::DrawDebugExplosion(
	const TArray<AActor*>& Targets) const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector ExplosionCenter = GetActorLocation();
	const float Duration = ActiveExplosionConfig.DebugDrawDuration;
	::DrawDebugSphere(
		World,
		ExplosionCenter,
		ActiveExplosionConfig.ExplosionRadius,
		24,
		Targets.IsEmpty() ? FColor::Silver : FColor::Green,
		false,
		Duration);

	for (const AActor* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		::DrawDebugSphere(
			World,
			Target->GetActorLocation(),
			24.0f,
			12,
			FColor::Red,
			false,
			Duration);
		::DrawDebugLine(
			World,
			ExplosionCenter,
			Target->GetActorLocation(),
			FColor::Red,
			false,
			Duration);
	}

	// 반경 안이면서 제외된 Pawn을 따로 칠한다. 시야 차단이나 대상 수 제한,
	// 투척자 제외 중 무엇 때문에 안 맞았는지 눈으로 가려내기 위한 것이다.
	const float RadiusSquared = FMath::Square(
		ActiveExplosionConfig.ExplosionRadius);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Candidate = *It;
		if (!IsValid(Candidate) || Targets.Contains(Candidate) ||
			FVector::DistSquared(ExplosionCenter, Candidate->GetActorLocation()) >
				RadiusSquared)
		{
			continue;
		}

		::DrawDebugSphere(
			World,
			Candidate->GetActorLocation(),
			24.0f,
			12,
			FColor::Yellow,
			false,
			Duration);
	}
#endif
}
bool APDActionProjectile::InitializeExplosionFragments(
	const TArray<TObjectPtr<UPDActionFragment>>& SourceFragments,
	EPDActionScope RequiredScope,
	const TCHAR* ListName,
	UAbilitySystemComponent* SourceAbilitySystem,
	UObject* EffectSourceObject,
	AActor* SourceActor,
	TArray<TObjectPtr<UPDActionFragment>>& OutRuntimeFragments,
	FString& OutError)
{
	OutError.Reset();
	OutRuntimeFragments.Reset();
	OutRuntimeFragments.Reserve(SourceFragments.Num());

	const TCHAR* ScopeName = RequiredScope == EPDActionScope::Source
		? TEXT("Source")
		: TEXT("Target");

	FPDActionExecutionContext PrepareContext;
	PrepareContext.SourceAbilitySystem = SourceAbilitySystem;
	PrepareContext.SourceActor = SourceActor;
	PrepareContext.EffectSourceObject = EffectSourceObject;

	for (int32 Index = 0; Index < SourceFragments.Num(); ++Index)
	{
		const UPDActionFragment* SourceFragment = SourceFragments[Index];
		if (!IsValid(SourceFragment))
		{
			OutError = FString::Printf(
				TEXT("%s[%d]가 비어 있습니다."),
				ListName,
				Index);
			return false;
		}

		if (SourceFragment->ApplicationScope != RequiredScope ||
			!SourceFragment->SupportsDeferredExecution())
		{
			OutError = FString::Printf(
				TEXT("%s[%d] '%s'는 %s Scope와 지연 실행을 지원해야 합니다."),
				ListName,
				Index,
				*GetNameSafe(SourceFragment),
				ScopeName);
			return false;
		}

		FString FragmentError;
		if (!SourceFragment->Validate(FragmentError))
		{
			OutError = FString::Printf(
				TEXT("%s[%d] '%s'가 유효하지 않습니다: %s"),
				ListName,
				Index,
				*GetNameSafe(SourceFragment),
				*FragmentError);
			return false;
		}

		UPDActionFragment* RuntimeFragment = DuplicateObject<UPDActionFragment>(
			SourceFragment,
			this);
		if (!RuntimeFragment)
		{
			OutError = FString::Printf(
				TEXT("%s[%d] '%s'의 Runtime 복제에 실패했습니다."),
				ListName,
				Index,
				*GetNameSafe(SourceFragment));
			return false;
		}

		if (!RuntimeFragment->PrepareDeferredExecution(
			PrepareContext,
			FragmentError))
		{
			OutError = FString::Printf(
				TEXT("%s[%d] '%s'의 지연 실행 준비에 실패했습니다: %s"),
				ListName,
				Index,
				*GetNameSafe(SourceFragment),
				*FragmentError);
			return false;
		}

		OutRuntimeFragments.Add(RuntimeFragment);
	}

	return true;
}
bool APDActionProjectile::ExecuteExplosionFragments(
	const TArray<TObjectPtr<UPDActionFragment>>& RuntimeFragments,
	const FPDActionExecutionContext& Context) const
{
	if (RuntimeFragments.IsEmpty())
	{
		return true;
	}

	TArray<const UPDActionFragment*, TInlineAllocator<8>> ExecutableFragments;
	for (const UPDActionFragment* Fragment : RuntimeFragments)
	{
		if (!IsValid(Fragment))
		{
			return false;
		}

		FString ExecutionError;
		if (Fragment->CanExecute(Context, ExecutionError))
		{
			ExecutableFragments.Add(Fragment);
		}
		else if (Fragment->bRequired)
		{
			UE_LOG(
				LogPDActionProjectile,
				Warning,
				TEXT("필수 폭발 Fragment '%s'가 '%s'에서 실행 조건을 만족하지 못했습니다: %s"),
				*GetNameSafe(Fragment),
				*GetNameSafe(Context.ResolveScopedActor(Fragment->ApplicationScope)),
				*ExecutionError);
			return false;
		}
	}

	for (const UPDActionFragment* Fragment : ExecutableFragments)
	{
		if (!Fragment->Execute(Context) && Fragment->bRequired)
		{
			UE_LOG(
				LogPDActionProjectile,
				Warning,
				TEXT("필수 폭발 Fragment '%s'가 '%s'에서 실행에 실패했습니다."),
				*GetNameSafe(Fragment),
				*GetNameSafe(Context.ResolveScopedActor(Fragment->ApplicationScope)));
			return false;
		}
	}

	return true;
}
void APDActionProjectile::ClearSourceMovementIgnore()
{
	if (AActor* SourceActor = IgnoredSourceActor.Get())
	{
		CollisionComponent->IgnoreActorWhenMoving(SourceActor, false);
	}
	if (APawn* SourcePawn = IgnoredSourcePawn.Get())
	{
		SourcePawn->MoveIgnoreActorRemove(this);
	}
	IgnoredSourcePawn.Reset();
	IgnoredSourceActor.Reset();
}

void APDActionProjectile::RefreshSourceMovementIgnore(
	AActor* SourceActor,
	APawn* SourcePawn)
{
	if (IgnoredSourceActor.Get() == SourceActor &&
		IgnoredSourcePawn.Get() == SourcePawn)
	{
		return;
	}

	ClearSourceMovementIgnore();
	if (!IsValid(SourceActor))
	{
		return;
	}

	IgnoredSourceActor = SourceActor;
	IgnoredSourcePawn = SourcePawn;
	CollisionComponent->IgnoreActorWhenMoving(SourceActor, true);
	if (SourcePawn)
	{
		// 투사체 Sweep와 투척자 이동 Sweep 양쪽에서 서로를 무시한다.
		SourcePawn->MoveIgnoreActorAdd(this);
	}
}

void APDActionProjectile::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDActionProjectile, ReplicatedProjectileMesh);
	DOREPLIFETIME(APDActionProjectile, ReplicatedProjectileMeshScale);
	DOREPLIFETIME(APDActionProjectile, ReplicatedCollisionRadius);
}
