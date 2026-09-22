#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PDSingleActionDefinition.generated.h"

class UPDThrowProjectileFragment;

/** Press 한 번에 한 번 실행하고 종료하는 Action Definition이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Single Action"))
class PADO_API UPDSingleActionDefinition : public UPDAbilityDefinition
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UPDGA_Base> GetAbilityClass() const override;

	/** 충전 투척 Fragment가 있으면 Press가 아니라 Release에서 실행한다. */
	bool ExecutesOnInputRelease() const;
	const UPDThrowProjectileFragment* FindChargedThrowProjectileFragment() const;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
