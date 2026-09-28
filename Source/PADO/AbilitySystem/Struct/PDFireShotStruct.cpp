#include "PADO/AbilitySystem/Struct/PDFireShotStruct.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

FPDFireShotHitStruct FPDFireShotHitStruct::Make(
	AActor* InActor,
	const FHitResult* Hit)
{
	FPDFireShotHitStruct Result;
	Result.Actor = InActor;
	if (Hit)
	{
		Result.Component = Hit->GetComponent();
		Result.BoneName = Hit->BoneName;
		Result.ImpactPoint = Hit->ImpactPoint;
		Result.ImpactNormal = Hit->ImpactNormal;
		Result.PhysMaterial = Hit->PhysMaterial.Get();
		Result.bHasHitResult = true;
	}
	return Result;
}

FHitResult FPDFireShotHitStruct::ToHitResult(
	const FVector& TraceStart,
	const FVector& TraceEnd) const
{
	FHitResult Hit(Actor.Get(), Component.Get(), ImpactPoint, ImpactNormal);
	Hit.bBlockingHit = true;
	Hit.BoneName = BoneName;
	Hit.PhysMaterial = PhysMaterial.Get();
	Hit.TraceStart = TraceStart;
	Hit.TraceEnd = TraceEnd;
	Hit.Distance = FVector::Dist(TraceStart, ImpactPoint);
	return Hit;
}

void FPDFireShotStruct::SetShotResult(const FHitResult& ShotResult)
{
	TraceStart = ShotResult.TraceStart;
	ShotEnd = ShotResult.ImpactPoint;
	ImpactNormal = ShotResult.ImpactNormal;
	PhysMaterial = ShotResult.PhysMaterial.Get();
	bBlockingHit = ShotResult.bBlockingHit;
	bHasShotResult = true;
}

FHitResult FPDFireShotStruct::MakeShotResult() const
{
	FHitResult Shot(TraceStart, ShotEnd);
	Shot.Location = ShotEnd;
	Shot.ImpactPoint = ShotEnd;
	Shot.Normal = ImpactNormal;
	Shot.ImpactNormal = ImpactNormal;
	Shot.PhysMaterial = PhysMaterial.Get();
	Shot.bBlockingHit = bBlockingHit;
	Shot.Distance = FVector::Dist(TraceStart, ShotEnd);
	return Shot;
}
