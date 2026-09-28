#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Struct/PDLoopingCueStruct.h"
#include "PDChannelActionDefinition.generated.h"

/**
 * Press부터 Release까지 유지되며 몽타주가 실행 시점을 정하는 Action Definition이다.
 *
 * 실행 시점은 몽타주의 Action Execute Event 또는 Trace Window가 준다. 방아쇠로
 * 반복해서 쏘는 무기는 이것이 아니라 Fire Action이다.
 */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Channel Action"))
class PADO_API UPDChannelActionDefinition : public UPDAbilityDefinition
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const override;

	/**
	 * Press부터 Release까지 유지할 표현 Cue다. 화염방사처럼 누르는 동안 이어지는
	 * 연출에 쓴다. GA가 실행 시작에 붙이고 종료에 뗀다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Presentation",
		meta = (ShowOnlyInnerProperties))
	FPDLoopingCueStruct LoopingCue;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
