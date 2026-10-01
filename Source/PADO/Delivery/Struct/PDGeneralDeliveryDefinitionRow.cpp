#include "PADO/Delivery/Struct/PDGeneralDeliveryDefinitionRow.h"

bool FPDGeneralDeliveryDefinitionRow::Validate(FString& OutError) const
{
	FString CommonDataError;
	if (!CommonData.Validate(CommonDataError))
	{
		OutError = FString::Printf(TEXT("CommonData is invalid: %s"), *CommonDataError);
		return false;
	}

	if (DeadlineSeconds <= 0)
	{
		OutError = TEXT("DeadlineSeconds must be at least 1.");
		return false;
	}

	if (RewardReductionTimeSeconds <= 0 || RewardReductionTimeSeconds >= DeadlineSeconds)
	{
		OutError = TEXT("RewardReductionTimeSeconds must be at least 1 and less than DeadlineSeconds.");
		return false;
	}

	if (ReducedReward < 0 || ReducedReward > CommonData.BaseReward)
	{
		OutError = TEXT("ReducedReward must be non-negative and no greater than BaseReward.");
		return false;
	}

	OutError.Reset();
	return true;
}
