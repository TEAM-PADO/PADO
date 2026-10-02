#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PADO/Interaction/Struct/PDInteractionContextStruct.h"
#include "PDInteractionComponent.generated.h"

class UPrimitiveComponent;
class UWorld;

/**
 * 상호작용하는 쪽의 컴포넌트다. 대상을 고르고 서버에 요청한다.
 *
 * 손 닿는 거리는 대상 종류와 무관하게 이 컴포넌트의 값 하나다. 거리는 몸의
 * 기준점에서 대상 충돌체의 가장 가까운 점까지 잰다. 무엇을 할지는 대상이
 * IPDInteractable로 정한다. 대상 액터는 클라이언트가 소유하지 않으므로
 * Server RPC는 언제나 이 컴포넌트가 보낸다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDInteractionComponent();

	/**
	 * 지금 상호작용할 대상을 골라 요청한다. 입력 계층에서 부른다.
	 * 서버 권한이면 바로 확정하고, 아니면 서버로 보낸다. 반환값은 요청했는지다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Interaction")
	bool TryInteract();

	/**
	 * 서버에서 상호작용을 확정한다.
	 *
	 * 거리는 다시 재지 않는다. 대상을 고른 머신이 자기 화면 기준으로 이미
	 * 판단했고, 서버 위치로 다시 재면 경계에서 누른 정상 요청이 지연만큼 거부된다.
	 * 대상이 지금 상호작용할 수 있는 상태인지만 본다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Interaction")
	bool InteractWithTarget(AActor* Target, UPrimitiveComponent* AimedComponent);

	/**
	 * 시점 Sweep으로 대상을 고른다. 시선에 걸린 것이 없으면 손 닿는 거리 안에서
	 * 가장 가까운 대상을 고른다. 고를 대상이 없으면 false다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Interaction")
	bool FindInteractionTarget(
		AActor*& OutTarget,
		UPrimitiveComponent*& OutAimedComponent) const;

	/**
	 * 몸의 기준점에서 대상 충돌체의 가장 가까운 점까지 거리다.
	 * 잴 수 있는 충돌체가 없으면 기준점끼리 잰다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Interaction")
	float GetDistanceToTarget(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "PD|Interaction")
	float GetInteractionReach() const { return InteractionReach; }

protected:
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Target, UPrimitiveComponent* AimedComponent);

private:
	/** 상호작용하는 쪽이 살아 있는가. 빈사·사망이면 아무것과도 상호작용하지 않는다. */
	bool IsOwnerAbleToInteract() const;

	FPDInteractionContextStruct MakeContext(
		UPrimitiveComponent* AimedComponent) const;
	bool CanInteractWith(
		const AActor* Target,
		const FPDInteractionContextStruct& Context) const;
	bool FindAimedTarget(
		const UWorld& World,
		const FPDInteractionContextStruct& BaseContext,
		AActor*& OutTarget,
		UPrimitiveComponent*& OutAimedComponent) const;
	bool FindNearestTarget(
		const UWorld& World,
		const FPDInteractionContextStruct& BaseContext,
		AActor*& OutTarget,
		UPrimitiveComponent*& OutAimedComponent) const;
	float GetDistanceToComponent(const UPrimitiveComponent& Component) const;

	/** 손이 닿는 거리다. 대상 종류와 무관하게 이 값 하나로 판단한다. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Interaction",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractionReach = 250.0f;

	/** 시점 Sweep의 굵기다. 크게 잡을수록 작은 대상을 조준하기 쉽다. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Interaction",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float TraceRadius = 24.0f;

	/**
	 * 시점에서 앞으로 탐색할 거리다. 3인칭 카메라가 캐릭터 뒤에 있으므로
	 * 손 닿는 거리보다 길게 잡는다. 걸린 대상은 손 닿는 거리로 다시 거른다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Interaction",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float TraceDistance = 900.0f;
};
