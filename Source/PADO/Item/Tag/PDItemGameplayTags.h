#pragma once

#include "NativeGameplayTags.h"

// 아이템 식별 태그다. UPDItemDefinition::ItemId가 이 계층에서 값을 고른다.
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Item_Id_Weapon_AssaultRifle);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Item_Id_Weapon_SniperRifle);

// 무기 연출 Gameplay Cue 태그는 `Config/DefaultGameplayTags.ini`에 있다.
// 런타임 C++이 이름으로 참조하지 않고 Item Definition만 가리키므로, 무기가
// 늘어도 재빌드 없이 에디터에서 추가할 수 있게 두었다.
