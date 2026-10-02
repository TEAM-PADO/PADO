#include "PADO/Vehicle/Component/PDVehicleImpactComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "PADO/AbilitySystem/Component/PDKnockbackComponent.h"
#include "PADO/AbilitySystem/Effect/PDGE_Damage.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "PADO/Vehicle/Interface/PDControllableVehicle.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDVehicleImpact, Log, All);

UPDVehicleImpactComponent::UPDVehicleImpactComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDVehicleImpactComponent::HandleCharacterContact(
	APDCharacterBase& Character,
	const FVector& ContactPoint)
{
	AActor* Vehicle = GetOwner();
	const UWorld* World = GetWorld();
	if (!Vehicle || !World || !Vehicle->HasAuthority() || Character.IsDead())
	{
		return false;
	}

	// 탄 사람은 충돌이 꺼져 닿지 않는다. 다른 감지 경로로 와도 치지 않는다.
	if (const UPDVehicleOccupantComponent* Occupant = Character.GetVehicleOccupantComponent();
		Occupant && Occupant->IsSeated())
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	for (auto It = LastImpactTimes.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid() || Now - It->Value >= RehitInterval)
		{
			It.RemoveCurrent();
		}
	}
	if (LastImpactTimes.Contains(&Character))
	{
		return false;
	}

	FVector Direction;
	const float ImpactSpeed = ResolveImpactSpeed(Character, ContactPoint, Direction);
	if (ImpactSpeed < MinImpactSpeed)
	{
		return false;
	}

	LastImpactTimes.Add(&Character, Now);

	// 넉백을 먼저 건다. 이 피해로 죽으면 사망 처리가 넉백 속도를 래그돌에 넘긴다.
	if (UPDKnockbackComponent* Knockback = Character.FindComponentByClass<UPDKnockbackComponent>())
	{
		FPDKnockbackRequest Request;
		Request.Direction = Direction;
		Request.HorizontalSpeed = ImpactSpeed * KnockbackSpeedScale;
		Request.VerticalSpeed = KnockbackUpSpeed;
		Knockback->ApplyKnockback(Request);
	}

	ApplyImpactSpeedLoss(Character, Direction, ImpactSpeed);

	const float Damage = FMath::GetMappedRangeValueClamped(
		FVector2f(MinDamageSpeed, MaxDamageSpeed),
		FVector2f(0.0f, MaxDamage),
		ImpactSpeed);
	UAbilitySystemComponent* SourceAbilitySystem = ResolveSourceAbilitySystem();
	UAbilitySystemComponent* TargetAbilitySystem = Character.GetAbilitySystemComponent();
	if (Damage > 0.0f && SourceAbilitySystem && TargetAbilitySystem)
	{
		UPDGE_Damage::ApplyDamage(*SourceAbilitySystem, *TargetAbilitySystem, Damage, Vehicle);
	}

	UE_LOG(
		LogPDVehicleImpact,
		Log,
		TEXT("%s가 %s를 쳤다. 속도 %.0fcm/s, 피해 %.1f"),
		*GetNameSafe(Vehicle),
		*GetNameSafe(&Character),
		ImpactSpeed,
		Damage);
	return true;
}

void UPDVehicleImpactComponent::BeginPlay()
{
	Super::BeginPlay();

	// 판정은 서버 접촉만 쓴다.
	UPrimitiveComponent* Body = GetBody();
	if (GetOwner() && GetOwner()->HasAuthority() && Body)
	{
		// 스켈레탈 메시 차체는 컴포넌트와 물리 에셋의 바디가 둘 다 피격 알림을 켜야
		// 접촉을 알린다. 물리 에셋 바디의 기본값은 꺼져 있어 차량마다 저작에 기대지
		// 않고 여기서 모든 바디에 켠다(2026-10-02 PIE에서 꺼진 채로는 알림이 오지 않음).
		Body->SetNotifyRigidBodyCollision(true);
		Body->OnComponentHit.AddDynamic(this, &UPDVehicleImpactComponent::HandleBodyHit);
		BoundBody = Body;
	}
}

void UPDVehicleImpactComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPrimitiveComponent* Body = BoundBody.Get())
	{
		Body->OnComponentHit.RemoveDynamic(this, &UPDVehicleImpactComponent::HandleBodyHit);
	}
	BoundBody.Reset();
	LastImpactTimes.Reset();

	Super::EndPlay(EndPlayReason);
}

void UPDVehicleImpactComponent::HandleBodyHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (APDCharacterBase* Character = Cast<APDCharacterBase>(OtherActor))
	{
		HandleCharacterContact(*Character, Hit.ImpactPoint);
	}
}

float UPDVehicleImpactComponent::ResolveImpactSpeed(
	const APDCharacterBase& Character,
	const FVector& ContactPoint,
	FVector& OutDirection) const
{
	const AActor* Vehicle = GetOwner();
	const UPrimitiveComponent* Body = GetBody();

	// 차체가 돌고 있으면 닿은 자리의 속도는 차체 중심의 속도와 다르다.
	const FVector VehicleVelocity = Body && Body->IsSimulatingPhysics()
		? Body->GetPhysicsLinearVelocityAtPoint(ContactPoint)
		: Vehicle->GetVelocity();

	// 닿은 곳에서 사람 쪽이다. 닿은 곳이 몸 중심과 겹치면 차체 중심에서 보고,
	// 그것도 겹치면 차가 가는 방향이다.
	OutDirection = Character.GetActorLocation() - ContactPoint;
	OutDirection.Z = 0.0f;
	if (!OutDirection.Normalize())
	{
		OutDirection = Character.GetActorLocation() - Vehicle->GetActorLocation();
		OutDirection.Z = 0.0f;
		if (!OutDirection.Normalize())
		{
			OutDirection = VehicleVelocity.GetSafeNormal2D();
		}
	}

	// 사람이 차 쪽으로 걸어오는 속도는 더하지 않는다. 멈춘 차에 걸어 들어가는 것은 치인
	// 것이 아니다. 차에서 멀어지는 속도는 빼서 피하던 사람은 덜 다친다.
	const float AwaySpeed =
		FMath::Max(0.0f, static_cast<float>(FVector::DotProduct(Character.GetVelocity(), OutDirection)));
	return static_cast<float>(FVector::DotProduct(VehicleVelocity, OutDirection)) - AwaySpeed;
}

UAbilitySystemComponent* UPDVehicleImpactComponent::ResolveSourceAbilitySystem() const
{
	const IPDControllableVehicle* Controllable = Cast<IPDControllableVehicle>(GetOwner());
	const AController* Driver = Controllable ? Controllable->GetVehicleController() : nullptr;
	if (UAbilitySystemComponent* DriverAbilitySystem = Driver
		? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Driver->GetPawn())
		: nullptr)
	{
		return DriverAbilitySystem;
	}

	// 운전자 없이 굴러가는 차다.
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

void UPDVehicleImpactComponent::ApplyImpactSpeedLoss(
	const APDCharacterBase& Character,
	const FVector& Direction,
	float ImpactSpeed) const
{
	UPrimitiveComponent* Body = GetBody();
	const UCharacterMovementComponent* TargetMovement = Character.GetCharacterMovement();
	if (ImpactSpeedLossScale <= 0.0f || !Body || !Body->IsSimulatingPhysics() || !TargetMovement)
	{
		return;
	}

	const float VehicleMass = Body->GetMass();
	if (VehicleMass <= 0.0f || TargetMovement->Mass <= 0.0f)
	{
		return;
	}

	// 사람을 밀어낸 반작용이다. 다가오던 속도보다 많이 잃어 뒤로 밀리지는 않는다. 차체 중심에
	// 걸어 옆을 스쳐 쳐도 차가 돌지 않게 한다. 서버에서만 걸고 운전자 화면은 예측 보정으로 따라온다.
	const float SpeedLoss = FMath::Min(
		ImpactSpeed,
		ImpactSpeed * TargetMovement->Mass / VehicleMass * ImpactSpeedLossScale);
	Body->AddImpulse(-Direction * SpeedLoss, NAME_None, true);
}

UPrimitiveComponent* UPDVehicleImpactComponent::GetBody() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Cast<UPrimitiveComponent>(Owner->GetRootComponent()) : nullptr;
}
