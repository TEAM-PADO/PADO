#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Struct/PDActionCooldownStruct.h"
#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"
#include "PADO/AbilitySystem/Struct/PDActionMontageConfigStruct.h"
#include "UObject/Object.h"
#include "PDAbilityDefinition.generated.h"

class UPDActionTargeting;
class UPDGA_Base;

/**
 * Action 수명주기와 GA의 고정 대응 관계가 공유하는 불변 데이터다.
 * 구체 Definition은 자기 수명주기에 맞는 GA 클래스를 코드로 고정한다.
 * HideDropDown은 기존 직렬화 인스턴스의 로드는 허용하면서 새 저작 선택지에서는 숨긴다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, HideDropDown)
class PADO_API UPDAbilityDefinition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Action",
		meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/** 결과를 누구에게 적용할지 정한다. 대상 수집 방식이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Action")
	TObjectPtr<UPDActionTargeting> ActionTargeting;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Action",
		meta = (ShowOnlyInnerProperties))
	FPDActionMontageConfigStruct ActionMontage;

	/**
	 * 재사용 대기시간이다. 비워 두면 입력이 들어오는 대로 실행한다.
	 * 몽타주도 Channel 수명주기도 없는 Single Action은 이 값이 유일한 실행 간격이다.
	 * Fire Action은 이 값을 쓰지 않는다. 발 간격은 ShotInterval이 정한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action")
	FPDActionCooldownStruct ActionCooldown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action")
	TArray<FPDActionHookStruct> ActionHooks;

	const FPDActionHookStruct* FindActionHook(FGameplayTag HookTag) const;
	bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const;
	bool Validate(FString& OutError) const;
	bool ValidateWithActionContract(FString& OutError) const;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const;
};
