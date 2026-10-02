#include "PADO/Delivery/Struct/PDDeliveryCommonDefinitionData.h"

bool FPDDeliveryCommonDefinitionData::Validate(FString& OutError) const
{
	OutError.Reset();

	if (DeliveryLocationId.IsNone())
	{
		OutError = TEXT("DeliveryLocationId is empty.");
		return false;
	}

	TSet<FName> RequiredBossIds;
	for (const FName RequiredBossId : RequiredCompletedBossDeliveryIds)
	{
		if (RequiredBossId.IsNone())
		{
			OutError = TEXT("RequiredCompletedBossDeliveryIds contains an empty ID.");
			return false;
		}

		bool bAlreadyExists = false;
		RequiredBossIds.Add(RequiredBossId, &bAlreadyExists);
		if (bAlreadyExists)
		{
			OutError = FString::Printf(TEXT("RequiredCompletedBossDeliveryIds contains duplicate ID '%s'."), *RequiredBossId.ToString());
			return false;
		}
	}

	return true;
}
