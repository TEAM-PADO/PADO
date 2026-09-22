#include "PADO/Item/Fragment/PDConsumeMagazineAmmoFragment.h"

#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

UPDConsumeMagazineAmmoFragment::UPDConsumeMagazineAmmoFragment()
{
	ApplicationScope = EPDActionScope::Source;
	bRequired = true;
}

bool UPDConsumeMagazineAmmoFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	UPDWeaponMagazineComponent* Magazine = ResolveMagazine(Context);
	if (!Magazine)
	{
		OutError = TEXT("Ability Source가 유효한 Item Magazine을 제공하지 않습니다.");
		return false;
	}

	return Magazine->CanConsumeRound(OutError);
}

bool UPDConsumeMagazineAmmoFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	UPDWeaponMagazineComponent* Magazine = ResolveMagazine(Context);
	return Magazine && Magazine->TryConsumeRound();
}

UPDWeaponMagazineComponent* UPDConsumeMagazineAmmoFragment::ResolveMagazine(
	const FPDActionExecutionContext& Context) const
{
	const UPDAbilitySourceComponent* AbilitySource =
		Cast<UPDAbilitySourceComponent>(Context.EffectSourceObject);
	const APDWorldItemActor* Item = AbilitySource
		? Cast<APDWorldItemActor>(AbilitySource->GetOwner())
		: nullptr;
	return Item ? Item->GetMagazineComponent() : nullptr;
}
