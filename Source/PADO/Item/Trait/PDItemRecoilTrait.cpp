#include "PADO/Item/Trait/PDItemRecoilTrait.h"

#include "Curves/CurveFloat.h"

bool UPDItemRecoilTrait::Validate(FString& OutError) const
{
	// 값이 커서 이상해 보이는 것은 저작자 재량이다. 계산이 깨지는 것만 막는다.
	OutError.Reset();
	if (!FMath::IsFinite(PitchPerShot) || !FMath::IsFinite(PitchVariance) ||
		!FMath::IsFinite(YawPerShotMin) || !FMath::IsFinite(YawPerShotMax) ||
		!FMath::IsFinite(RecoveryDelay) || !FMath::IsFinite(RecoverySpeed))
	{
		OutError = TEXT("Recoil 수치에 유한하지 않은 값이 있습니다.");
		return false;
	}

	// 뒤집혀 있으면 FRandRange가 범위를 만들지 못해 반동이 한쪽으로만 나간다.
	if (YawPerShotMin > YawPerShotMax)
	{
		OutError = TEXT("Recoil의 YawPerShotMin은 YawPerShotMax보다 클 수 없습니다.");
		return false;
	}

	return true;
}

float UPDItemRecoilTrait::GetSprayMultiplier(int32 ShotIndex) const
{
	if (!SprayPattern)
	{
		return 1.0f;
	}

	return SprayPattern->GetFloatValue(static_cast<float>(ShotIndex));
}
