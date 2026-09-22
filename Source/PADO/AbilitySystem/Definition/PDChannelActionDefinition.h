#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Struct/PDLoopingCueStruct.h"
#include "PDChannelActionDefinition.generated.h"

/** Channel Action이 반복 결과를 만드는 기준이다. */
UENUM(BlueprintType)
enum class EPDChannelExecutionMode : uint8
{
	/** Montage의 Action Execute Event 또는 Trace Window가 실행 시점을 제공한다. */
	MontageEvent UMETA(DisplayName = "Montage Event"),

	/** 서버 타이머가 고정 간격으로 Instant Targeting을 다시 실행한다. */
	FixedInterval UMETA(DisplayName = "Fixed Interval")
};

/** Press부터 Release까지 유지되며 설정된 실행 기준마다 결과를 만드는 Action Definition이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Channel Action"))
class PADO_API UPDChannelActionDefinition : public UPDAbilityDefinition
{
	GENERATED_BODY()

public:
	static constexpr float MinimumPulseInterval = 0.02f;

	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Execution")
	EPDChannelExecutionMode ExecutionMode =
		EPDChannelExecutionMode::MontageEvent;

	/** FixedInterval에서 서버가 범위와 대상을 다시 수집하는 주기다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Execution",
		meta = (
			EditCondition = "ExecutionMode == EPDChannelExecutionMode::FixedInterval",
			EditConditionHides,
			ClampMin = "0.02",
			UIMin = "0.05",
			UIMax = "1.0",
			Units = "s"))
	float PulseInterval = 0.1f;

	/** FixedInterval에서 Press가 확정된 직후 첫 Pulse를 실행할지 정한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Execution",
		meta = (
			EditCondition = "ExecutionMode == EPDChannelExecutionMode::FixedInterval",
			EditConditionHides))
	bool bExecuteImmediately = true;

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
