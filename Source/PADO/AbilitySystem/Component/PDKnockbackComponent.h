#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "PDKnockbackComponent.generated.h"

class UPrimitiveComponent;

/** 소스 종류와 무관한 서버 권위 넉백 요청이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDKnockbackRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "PD|Knockback")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Knockback")
	float HorizontalSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Knockback")
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Knockback")
	bool bOverrideHorizontalVelocity = true;

	UPROPERTY(BlueprintReadWrite, Category = "PD|Knockback")
	bool bOverrideVerticalVelocity = true;

};

/** Character Launch와 물리 Velocity Change를 한 계약으로 제공하는 대상 측 Adapter다. */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDKnockbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PD|Knockback")
	bool CanApplyKnockback(const FPDKnockbackRequest& Request) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Knockback")
	bool ApplyKnockback(const FPDKnockbackRequest& Request);

protected:
	/** 비어 있으면 Owner의 Root Primitive를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Knockback")
	FComponentReference PhysicsComponent;

private:
	UPrimitiveComponent* ResolvePhysicsComponent() const;
};
