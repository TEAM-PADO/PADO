#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"

bool FPDActionHookStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!HookTag.IsValid())
	{
		OutError = TEXT("HookTag가 비어 있습니다.");
		return false;
	}

	if (Fragments.IsEmpty())
	{
		OutError = FString::Printf(
			TEXT("Action Hook '%s'에 Fragment가 없습니다."),
			*HookTag.ToString());
		return false;
	}

	for (int32 Index = 0; Index < Fragments.Num(); ++Index)
	{
		const UPDActionFragment* Fragment = Fragments[Index];
		if (!IsValid(Fragment))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'의 Fragments[%d]가 비어 있습니다."),
				*HookTag.ToString(),
				Index);
			return false;
		}

		FString FragmentError;
		if (!Fragment->Validate(FragmentError))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'의 Fragments[%d] '%s'가 유효하지 않습니다: %s"),
				*HookTag.ToString(),
				Index,
				*GetNameSafe(Fragment),
				*FragmentError);
			return false;
		}
	}

	return true;
}

bool FPDActionHookStruct::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return Fragments.ContainsByPredicate(
		[DataTag](const UPDActionFragment* Fragment)
		{
			return IsValid(Fragment) &&
				Fragment->DeclaresSetByCallerTag(DataTag);
		});
}
