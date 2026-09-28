#include "PADO/Item/Struct/PDItemPresentationStruct.h"

bool FPDItemPresentationStruct::Validate(FString& OutError) const
{
	// 메시가 없거나 둘 다 지정된 상태, 비어 있는 CollisionProfile은
	// 보이는 결과만 달라지고 게임은 동작하므로 막지 않는다.
	// 퇴화한 충돌 반지름만 실제로 상호작용을 불가능하게 만든다.
	OutError.Reset();
	if (!FMath::IsFinite(CollisionRadius) || CollisionRadius <= 0.0f)
	{
		OutError = TEXT("CollisionRadius는 0보다 큰 유한한 값이어야 합니다.");
		return false;
	}

	return true;
}
