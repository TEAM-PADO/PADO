#include "PADO/Item/Definition/PDItemDefinition.h"

#include "Misc/DataValidation.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"

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

	// Capacity가 0이면 발사 자체가 불가능하고, ReloadDuration이 0 이하면
	// 재장전 타이머가 정상 동작하지 않는다.
	if (Magazine.bEnabled &&
		(Magazine.Capacity < 1 || !FMath::IsFinite(Magazine.ReloadDuration) ||
			Magazine.ReloadDuration <= 0.0f))
	{
		OutError = TEXT("Magazine의 Capacity는 1 이상, ReloadDuration은 양수여야 합니다.");
		return false;
	}

	return true;
}
