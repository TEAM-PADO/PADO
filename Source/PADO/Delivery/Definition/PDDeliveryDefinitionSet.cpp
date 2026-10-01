#include "PADO/Delivery/Definition/PDDeliveryDefinitionSet.h"

#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"
#include "PADO/Delivery/Definition/PDBossDeliveryDefinition.h"

#if WITH_EDITOR
EDataValidationResult UPDDeliveryDefinitionSet::IsDataValid(FDataValidationContext& Context) const
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

bool UPDDeliveryDefinitionSet::FindGeneralDeliveryDefinition(FName DeliveryDefinitionId, FPDGeneralDeliveryDefinitionRow& OutDefinition) const
{
	if (DeliveryDefinitionId.IsNone() || !GeneralDeliveryDefinitionTable || GeneralDeliveryDefinitionTable->GetRowStruct() != FPDGeneralDeliveryDefinitionRow::StaticStruct())
	{
		return false;
	}

	const FPDGeneralDeliveryDefinitionRow* FoundDefinition = GeneralDeliveryDefinitionTable->FindRow<FPDGeneralDeliveryDefinitionRow>(DeliveryDefinitionId, TEXT("UPDDeliveryDefinitionSet::FindGeneralDeliveryDefinition"), false);
	if (!FoundDefinition)
	{
		return false;
	}

	FString ValidationError;
	if (!FoundDefinition->Validate(ValidationError))
	{
		return false;
	}

	OutDefinition = *FoundDefinition;
	return true;
}

const UPDBossDeliveryDefinition* UPDDeliveryDefinitionSet::FindBossDeliveryDefinition(FName DeliveryDefinitionId) const
{
	if (DeliveryDefinitionId.IsNone())
	{
		return nullptr;
	}

	for (const UPDBossDeliveryDefinition* BossDefinition : BossDeliveryDefinitions)
	{
		if (BossDefinition && BossDefinition->DeliveryDefinitionId == DeliveryDefinitionId)
		{
			FString ValidationError;
			return BossDefinition->Validate(ValidationError) ? BossDefinition : nullptr;
		}
	}

	return nullptr;
}

bool UPDDeliveryDefinitionSet::FindCommonDefinition(FName DeliveryDefinitionId, FPDDeliveryCommonDefinitionData& OutCommonData, EPDDeliveryType& OutDeliveryType) const
{
	FPDGeneralDeliveryDefinitionRow GeneralDefinition;
	if (FindGeneralDeliveryDefinition(DeliveryDefinitionId, GeneralDefinition))
	{
		OutCommonData = GeneralDefinition.CommonData;
		OutDeliveryType = EPDDeliveryType::General;
		return true;
	}

	if (const UPDBossDeliveryDefinition* BossDefinition = FindBossDeliveryDefinition(DeliveryDefinitionId))
	{
		OutCommonData = BossDefinition->CommonData;
		OutDeliveryType = EPDDeliveryType::Boss;
		return true;
	}

	return false;
}

bool UPDDeliveryDefinitionSet::Validate(FString& OutError) const
{
	OutError.Reset();

	if (!GeneralDeliveryDefinitionTable)
	{
		OutError = TEXT("GeneralDeliveryDefinitionTable is not configured.");
		return false;
	}

	if (GeneralDeliveryDefinitionTable->GetRowStruct() != FPDGeneralDeliveryDefinitionRow::StaticStruct())
	{
		OutError = TEXT("GeneralDeliveryDefinitionTable must use FPDGeneralDeliveryDefinitionRow as its row structure.");
		return false;
	}

	TSet<FName> DeliveryDefinitionIds;
	for (const FName GeneralRowName : GeneralDeliveryDefinitionTable->GetRowNames())
	{
		const FPDGeneralDeliveryDefinitionRow* GeneralDefinition = GeneralDeliveryDefinitionTable->FindRow<FPDGeneralDeliveryDefinitionRow>(GeneralRowName, TEXT("UPDDeliveryDefinitionSet::Validate"), false);
		if (!GeneralDefinition)
		{
			OutError = FString::Printf(TEXT("General delivery row '%s' could not be read."), *GeneralRowName.ToString());
			return false;
		}

		FString GeneralValidationError;
		if (!GeneralDefinition->Validate(GeneralValidationError))
		{
			OutError = FString::Printf(TEXT("General delivery '%s' is invalid: %s"), *GeneralRowName.ToString(), *GeneralValidationError);
			return false;
		}

		DeliveryDefinitionIds.Add(GeneralRowName);
	}

	for (const UPDBossDeliveryDefinition* BossDefinition : BossDeliveryDefinitions)
	{
		if (!BossDefinition)
		{
			OutError = TEXT("BossDeliveryDefinitions contains an empty entry.");
			return false;
		}

		FString BossValidationError;
		if (!BossDefinition->Validate(BossValidationError))
		{
			OutError = FString::Printf(TEXT("Boss delivery asset '%s' is invalid: %s"), *BossDefinition->GetName(), *BossValidationError);
			return false;
		}

		bool bAlreadyExists = false;
		DeliveryDefinitionIds.Add(BossDefinition->DeliveryDefinitionId, &bAlreadyExists);
		if (bAlreadyExists)
		{
			OutError = FString::Printf(TEXT("DeliveryDefinitionId '%s' is duplicated across general and boss definitions."), *BossDefinition->DeliveryDefinitionId.ToString());
			return false;
		}
	}

	return true;
}
