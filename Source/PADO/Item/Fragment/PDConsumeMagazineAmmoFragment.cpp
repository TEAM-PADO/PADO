#include "PADO/Item/Fragment/PDConsumeMagazineAmmoFragment.h"

#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

UPDConsumeMagazineAmmoFragment::UPDConsumeMagazineAmmoFragment()
{
	ApplicationScope = EPDActionScope::Source;
	bRequired = true;
}

bool UPDConsumeMagazineAmmoFragment::CanActivateWithPredictedState(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	UPDWeaponMagazineComponent* Magazine = ResolveMagazine(Context);
	if (!Magazine)
	{
		// 탄창이 없는 아이템은 탄약 제약이 없다. 여기서 막으면 저작 실수로
		// 붙은 Fragment 하나가 아이템 사용을 통째로 잠근다.
		return true;
	}

	if (!Magazine->CanConsumeRoundWithReplicatedState())
	{
		OutError = TEXT("탄창이 비었거나 재장전 중입니다.");
		return false;
	}

	return true;
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
