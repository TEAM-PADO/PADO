#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PDConsumeMagazineAmmoFragment.generated.h"

class UPDWeaponMagazineComponent;

/** 발사 판정 전에 Source 무기의 탄창에서 한 발을 소비하는 필수 Fragment다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Consume Magazine Ammo"))
class PADO_API UPDConsumeMagazineAmmoFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	UPDConsumeMagazineAmmoFragment();

	virtual bool CanActivateWithPredictedState(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

private:
	UPDWeaponMagazineComponent* ResolveMagazine(
		const FPDActionExecutionContext& Context) const;
};
