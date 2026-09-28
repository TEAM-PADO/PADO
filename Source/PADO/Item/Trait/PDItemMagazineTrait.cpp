#include "PADO/Item/Trait/PDItemMagazineTrait.h"

#include "Animation/AnimMontage.h"
#include "PADO/Item/Animation/PDAnimNotify_ReloadComplete.h"

bool UPDItemMagazineTrait::Validate(FString& OutError) const
{
	// Capacity가 0이면 발사 자체가 불가능하고, ReloadSpeed가 0 이하면 몽타주가
	// 진행하지 않아 재장전이 영원히 끝나지 않는다.
	OutError.Reset();
	if (Capacity < 1)
	{
		OutError = TEXT("Magazine의 Capacity는 1 이상이어야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(ReloadSpeed) || ReloadSpeed <= 0.0f)
	{
		OutError = TEXT("Magazine의 ReloadSpeed는 양수여야 합니다.");
		return false;
	}

	return true;
}

float UPDItemMagazineTrait::GetReloadCompleteTime() const
{
	if (!ReloadMontage)
	{
		return 0.0f;
	}

	for (const FAnimNotifyEvent& Event : ReloadMontage->Notifies)
	{
		if (Event.Notify &&
			Event.Notify->IsA<UPDAnimNotify_ReloadComplete>())
		{
			return Event.GetTriggerTime();
		}
	}

	// 노티파이를 아직 안 찍었으면 몽타주가 끝날 때 충전한다. 저작 중에도
	// 재장전이 멈추지 않도록 여기서 막지 않는다.
	return ReloadMontage->GetPlayLength();
}
