#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Item/Tag/PDItemGameplayTags.h"
#include "PADO/Item/Trait/PDItemMagazineTrait.h"

namespace PDItemTestUtils
{
	/**
	 * 손에 드는 아이템 정의다. 탄창 크기가 0보다 크면 몽타주 없는 탄창을 넣어
	 * 재장전이 즉시 끝나게 한다. 테스트 World의 몸은 몽타주를 재생할 수 없다.
	 */
	inline UPDItemDefinition* MakeHeldItemDefinition(
		UObject* Outer,
		int32 MagazineCapacity,
		UPDAbilityDefinition* UseAction = nullptr)
	{
		UPDItemDefinition* Definition = NewObject<UPDItemDefinition>(Outer);
		Definition->ItemId = TAG_PD_Item_Id_Weapon_SniperRifle;
		Definition->DisplayName = FText::FromString(TEXT("Automation Held Item"));
		Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
		Definition->Presentation.bSimulatePhysicsInWorld = false;
		if (MagazineCapacity > 0)
		{
			UPDItemMagazineTrait* MagazineTrait = NewObject<UPDItemMagazineTrait>(Definition);
			MagazineTrait->Capacity = MagazineCapacity;
			Definition->Traits.Add(MagazineTrait);
		}
		Definition->UseAction = UseAction;
		return Definition;
	}

	/** 정의로 초기화하고 BeginPlay를 거친 월드 아이템이다. */
	inline APDWorldItemActor* SpawnHeldItem(UWorld& World, UPDItemDefinition& Definition)
	{
		APDWorldItemActor* Item = World.SpawnActor<APDWorldItemActor>();
		if (!Item || !Item->InitializeItem(&Definition))
		{
			return nullptr;
		}
		Item->DispatchBeginPlay();
		return Item;
	}
}

#endif
