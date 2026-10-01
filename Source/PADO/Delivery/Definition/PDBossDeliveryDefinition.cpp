#include "PADO/Delivery/Definition/PDBossDeliveryDefinition.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UPDBossDeliveryDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!Validate(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

bool UPDBossDeliveryDefinition::Validate(FString& OutError) const
{
	OutError.Reset();

	if (DeliveryDefinitionId.IsNone())
	{
		OutError = TEXT("DeliveryDefinitionId is empty.");
		return false;
	}

	FString CommonDataError;
	if (!CommonData.Validate(CommonDataError))
	{
		OutError = FString::Printf(TEXT("CommonData is invalid: %s"), *CommonDataError);
		return false;
	}

	if (BossId.IsNone())
	{
		OutError = TEXT("BossId is empty.");
		return false;
	}

	if (CombatDeadlineSeconds <= 0)
	{
		OutError = TEXT("CombatDeadlineSeconds must be at least 1.");
		return false;
	}

	if (RecipientInteractionType == EPDBossRecipientInteractionType::None)
	{
		OutError = TEXT("RecipientInteractionType must be selected.");
		return false;
	}

	return true;
}
