#include "PADO/Item/Definition/PDItemDefinition.h"

#include "Misc/DataValidation.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/Item/Trait/PDItemTrait.h"

#if WITH_EDITOR
EDataValidationResult UPDItemDefinition::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!Validate(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

bool UPDItemDefinition::IsUsable() const
{
	return IsValid(UseAction);
}

bool UPDItemDefinition::Validate(FString& OutError) const
{
	// 잘못 설정하면 아이템이 실제로 동작하지 않는 항목만 막는다.
	// 표시 이름, 아이콘, 메시 구성, 탄약 Fragment 배치 순서처럼 비어 있거나
	// 달라도 게임이 굴러가는 항목은 검증하지 않는다.
	OutError.Reset();

	FString PresentationError;
	if (!Presentation.Validate(PresentationError))
	{
		OutError = FString::Printf(
			TEXT("Presentation이 유효하지 않습니다: %s"),
			*PresentationError);
		return false;
	}

	// UseAction 계약이 깨지면 장착해도 Ability를 부여할 수 없다.
	if (UseAction)
	{
		FString AbilityError;
		if (!UseAction->ValidateWithActionContract(AbilityError))
		{
			OutError = FString::Printf(
				TEXT("UseAction이 유효하지 않습니다: %s"),
				*AbilityError);
			return false;
		}
	}

	// 같은 Trait이 둘이면 FindTrait이 첫 번째만 돌려주고 나머지 설정은 조용히
	// 무시된다. 저작 실수가 런타임에 드러나지 않으므로 여기서 막는다.
	TSet<const UClass*> SeenTraitClasses;
	for (const UPDItemTrait* Trait : Traits)
	{
		if (!Trait)
		{
			continue;
		}

		bool bAlreadySeen = false;
		SeenTraitClasses.Add(Trait->GetClass(), &bAlreadySeen);
		if (bAlreadySeen)
		{
			OutError = FString::Printf(
				TEXT("Trait '%s'가 두 번 이상 들어 있습니다."),
				*Trait->GetClass()->GetName());
			return false;
		}

		FString TraitError;
		if (!Trait->Validate(TraitError))
		{
			OutError = FString::Printf(
				TEXT("Trait '%s'가 유효하지 않습니다: %s"),
				*Trait->GetClass()->GetName(),
				*TraitError);
			return false;
		}
	}

	return true;
}

const UPDItemTrait* UPDItemDefinition::FindTrait(
	TSubclassOf<UPDItemTrait> TraitClass) const
{
	if (!TraitClass)
	{
		return nullptr;
	}

	for (const UPDItemTrait* Trait : Traits)
	{
		if (Trait && Trait->IsA(TraitClass))
		{
			return Trait;
		}
	}

	return nullptr;
}
