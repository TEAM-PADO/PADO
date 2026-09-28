#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "PDMovementAttributeSet.generated.h"

#define PD_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 이동 속도의 외부 효과 계층이다.
 *
 * 슬로우와 헤이스트처럼 남이 걸어 주는 효과만 여기에 붙는다.
 * 견착·조준·스프린트처럼 본인 입력이 만드는 자세 배율은
 * UPDCharacterMovementComponent가 이동 예측과 함께 처리한다.
 */
UCLASS()
class PADO_API UPDMovementAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPDMovementAttributeSet();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) override;
	virtual void PreAttributeBaseChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) const override;

	/** 자세 배율을 적용하기 전의 걷기 속도다. */
	UPROPERTY(
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_MoveSpeed,
		Category = "PD|Movement")
	FGameplayAttributeData MoveSpeed;
	PD_ATTRIBUTE_ACCESSORS(UPDMovementAttributeSet, MoveSpeed)

protected:
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
};
