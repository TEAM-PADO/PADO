#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Effect/PDGE_MoveSpeedMultiplier.h"
#include "PADO/Character/PDCharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/Interface/PDReloadableItem.h"
#include "PADO/Item/Fragment/PDConsumeMagazineAmmoFragment.h"
#include "PADO/Item/Tag/PDItemGameplayTags.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace PDWeaponSystemTests
{
	UPDItemDefinition* MakeMagazineItemDefinition(
		UObject* Outer,
		int32 MagazineCapacity,
		bool bAutomatic)
	{
		UPDItemDefinition* Weapon = NewObject<UPDItemDefinition>(Outer);
		Weapon->ItemId = bAutomatic
			? TAG_PD_Item_Id_Weapon_AssaultRifle
			: TAG_PD_Item_Id_Weapon_SniperRifle;
		Weapon->DisplayName = FText::FromString(
			bAutomatic ? TEXT("Automation Assault Rifle") : TEXT("Automation Sniper Rifle"));
		Weapon->Presentation.StaticMesh = NewObject<UStaticMesh>(Weapon);
		Weapon->Presentation.bSimulatePhysicsInWorld = false;
		Weapon->Magazine.bEnabled = true;
		Weapon->Magazine.Capacity = MagazineCapacity;
		Weapon->Magazine.ReloadDuration = 0.01f;

		UPDAbilityDefinition* Action = bAutomatic
			? static_cast<UPDAbilityDefinition*>(
				NewObject<UPDChannelActionDefinition>(Weapon))
			: static_cast<UPDAbilityDefinition*>(
				NewObject<UPDSingleActionDefinition>(Weapon));
		Action->ActionTargeting = NewObject<UPDSelfTargeting>(Action);
		if (UPDChannelActionDefinition* Channel =
			Cast<UPDChannelActionDefinition>(Action))
		{
			Channel->ExecutionMode = EPDChannelExecutionMode::FixedInterval;
			Channel->PulseInterval = 0.1f;
			Channel->bExecuteImmediately = true;
		}

		FPDActionHookStruct ConsumeHook;
		ConsumeHook.HookTag = TAG_PD_ActionHook_OnExecuteStart;
		ConsumeHook.Fragments.Add(
			NewObject<UPDConsumeMagazineAmmoFragment>(Action));
		Action->ActionHooks.Add(MoveTemp(ConsumeHook));
		Weapon->UseAction = Action;
		return Weapon;
	}

	UWorld* CreateTestWorld(FWorldContext*& OutWorldContext)
	{
		const FName WorldName = MakeUniqueObjectName(
			nullptr,
			UWorld::StaticClass(),
			TEXT("PDWeaponTestWorld"),
			EUniqueObjectNameOptions::GloballyUnique);
		UWorld* World = UWorld::CreateWorld(
			EWorldType::Game,
			false,
			WorldName,
			GetTransientPackage());
		OutWorldContext = World
			? &GEngine->CreateNewWorldContext(EWorldType::Game)
			: nullptr;
		if (OutWorldContext)
		{
			OutWorldContext->SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	bool ConfigureHolderSocket(APDPlayerCharacter* Holder)
	{
		if (!Holder)
		{
			return false;
		}

		UStaticMesh* HandMeshAsset = NewObject<UStaticMesh>(Holder);
		UStaticMeshSocket* HandSocket = NewObject<UStaticMeshSocket>(HandMeshAsset);
		HandSocket->SocketName = TEXT("HandItem");
		HandMeshAsset->Sockets.Add(HandSocket);

		UStaticMeshComponent* HandMesh = NewObject<UStaticMeshComponent>(Holder);
		HandMesh->SetupAttachment(Holder->GetRootComponent());
		HandMesh->SetStaticMesh(HandMeshAsset);
		HandMesh->RegisterComponent();
		return Holder->GetHeldItemComponent()->ConfigureAttachment(
			HandMesh,
			TEXT("HandItem"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDItemMagazineDefinitionValidationTest,
	"PADO.Item.Magazine.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDItemMagazineDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FString Error;
	UPDItemDefinition* Assault = MakeMagazineItemDefinition(
		GetTransientPackage(),
		30,
		true);
	UPDItemDefinition* Sniper = MakeMagazineItemDefinition(
		GetTransientPackage(),
		5,
		false);

	TestTrue(TEXT("30발 FixedInterval Assault Definition은 유효하다."),
		Assault->Validate(Error));
	TestTrue(TEXT("5발 Single Sniper Definition은 유효하다."),
		Sniper->Validate(Error));

	UPDItemDefinition* AssaultAsset = LoadObject<UPDItemDefinition>(
		nullptr,
		TEXT("/Game/PADO/Item/Definition/DA_Item_AssaultRifle.DA_Item_AssaultRifle"));
	UPDItemDefinition* SniperAsset = LoadObject<UPDItemDefinition>(
		nullptr,
		TEXT("/Game/PADO/Item/Definition/DA_Item_SniperRifle.DA_Item_SniperRifle"));
	if (TestNotNull(TEXT("기존 Assault Item DA를 로드한다."), AssaultAsset))
	{
		TestTrue(TEXT("실제 Assault Item DA가 유효하다."), AssaultAsset->Validate(Error));
		TestEqual(TEXT("실제 Assault Item DA 탄창은 30발이다."),
			AssaultAsset->Magazine.Capacity, 30);
	}
	if (TestNotNull(TEXT("Sniper Item DA를 로드한다."), SniperAsset))
	{
		TestTrue(TEXT("실제 Sniper Item DA가 유효하다."), SniperAsset->Validate(Error));
		TestEqual(TEXT("실제 Sniper Item DA 탄창은 5발이다."),
			SniperAsset->Magazine.Capacity, 5);
	}

	Sniper->Magazine.Capacity = 0;
	TestFalse(TEXT("0발 탄창은 거부한다."), Sniper->Validate(Error));
	Sniper->Magazine.Capacity = 5;
	// 탄약 소비 Fragment 배치는 제작자 재량이다. 없으면 탄약을 소비하지 않을
	// 뿐 게임은 동작하므로 Definition 검증에서 막지 않는다.
	Sniper->UseAction->ActionHooks.Reset();
	TestTrue(
		TEXT("탄약 소비 Fragment가 없어도 Magazine Item Definition은 유효하다."),
		Sniper->Validate(Error));
	Sniper->Magazine.bEnabled = false;
	TestTrue(TEXT("탄창이 비활성화된 일반 아이템은 같은 Definition으로 유효하다."),
		Sniper->Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDWeaponMagazineLifecycleTest,
	"PADO.Item.Weapon.Magazine.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDWeaponMagazineLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("Weapon 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("Weapon을 스폰한다."), Weapon))
	{
		Holder->GetPDAbilitySystemComponent()->InitAbilityActorInfo(Holder, Holder);
		TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder));

		UPDItemDefinition* Definition = MakeMagazineItemDefinition(Weapon, 5, false);
		TestTrue(TEXT("Weapon을 5발 탄창으로 초기화한다."),
			Weapon->InitializeItem(Definition));
		UPDWeaponMagazineComponent* Magazine = Weapon->GetMagazineComponent();
		TestNotNull(TEXT("Weapon에 Magazine Component가 있다."), Magazine);
		TestEqual(TEXT("초기 탄창은 가득 차 있다."),
			Magazine->GetCurrentMagazineAmmo(), 5);

		for (int32 ShotIndex = 0; ShotIndex < 5; ++ShotIndex)
		{
			TestTrue(TEXT("남은 탄약이 있으면 한 발을 소비한다."),
				Magazine->TryConsumeRound());
		}
		TestEqual(TEXT("5발 소비 후 탄창은 비어 있다."),
			Magazine->GetCurrentMagazineAmmo(), 0);
		TestFalse(TEXT("빈 탄창에서는 추가 소비할 수 없다."),
			Magazine->TryConsumeRound());

		Weapon->DispatchBeginPlay();
		TestTrue(TEXT("Holder가 Weapon을 줍는다."),
			Holder->GetHeldItemComponent()->TryPickUp(Weapon));
		TestTrue(TEXT("빈 탄창에서 재장전을 시작한다."),
			Magazine->TryStartReload());
		TestTrue(TEXT("재장전 상태가 활성화된다."), Magazine->IsReloading());

		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.02f);
		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.02f);
		TestFalse(TEXT("재장전 타이머 완료 후 상태를 해제한다."),
			Magazine->IsReloading());
		TestEqual(TEXT("재장전 완료 시 탄창을 최대치로 채운다."),
			Magazine->GetCurrentMagazineAmmo(), 5);

		TestTrue(TEXT("재장전 검증을 위해 한 발 소비한다."),
			Magazine->TryConsumeRound());
		TestTrue(TEXT("부분 탄창에서 재장전을 시작한다."),
			Magazine->TryStartReload());
		TestTrue(TEXT("재장전 중 Weapon을 드롭한다."),
			Holder->GetHeldItemComponent()->DropHeldItem(FTransform::Identity));
		TestFalse(TEXT("드롭 시 재장전을 취소한다."), Magazine->IsReloading());
		TestEqual(TEXT("드롭해도 부분 탄창을 보존한다."),
			Magazine->GetCurrentMagazineAmmo(), 4);

		UPDItemDefinition* PlainItem = NewObject<UPDItemDefinition>(Weapon);
		PlainItem->ItemId = Definition->ItemId;
		PlainItem->DisplayName = FText::FromString(TEXT("Automation Plain Item"));
		PlainItem->Presentation = Definition->Presentation;
		TestTrue(TEXT("같은 Actor를 일반 아이템 Definition으로 전환한다."),
			Weapon->InitializeItem(PlainItem));
		TestNull(TEXT("일반 아이템은 활성 Magazine을 제공하지 않는다."),
			Weapon->GetMagazineComponent());
		TestTrue(TEXT("전환된 일반 아이템을 줍는다."),
			Holder->GetHeldItemComponent()->TryPickUp(Weapon));
		TestFalse(TEXT("일반 아이템은 재장전 요청을 거부한다."),
			IPDReloadableItem::Execute_TryStartReload(Weapon, Holder));
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDWeaponInputFiringTest,
	"PADO.Item.Weapon.Input.FiringModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDWeaponInputFiringTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("발사 입력 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	Holder->GetPDAbilitySystemComponent()->InitAbilityActorInfo(Holder, Holder);
	if (!TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	UPDHeldItemComponent* HeldItems = Holder->GetHeldItemComponent();

	APDWorldItemActor* Assault = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Assault Weapon을 스폰한다."), Assault))
	{
		UPDItemDefinition* Definition = MakeMagazineItemDefinition(Assault, 2, true);
		TestTrue(TEXT("연사 무기를 초기화한다."), Assault->InitializeItem(Definition));
		Assault->DispatchBeginPlay();
		TestTrue(TEXT("연사 무기를 줍는다."), HeldItems->TryPickUp(Assault));
		UPDWeaponMagazineComponent* Magazine = Assault->GetMagazineComponent();
		TestTrue(TEXT("연사 입력을 누른다."), HeldItems->PressHeldItemUse());
		TestEqual(TEXT("Press 직후 첫 발을 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 1);

		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.11f);
		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.11f);
		TestEqual(TEXT("누름 유지 시 두 번째 발을 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 0);
		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.11f);
		TestEqual(TEXT("빈 탄창에서는 추가 연사하지 않는다."),
			Magazine->GetCurrentMagazineAmmo(), 0);
		HeldItems->ReleaseHeldItemUse();
		TestTrue(TEXT("연사 무기를 드롭한다."),
			HeldItems->DropHeldItem(FTransform::Identity));
	}

	APDWorldItemActor* Sniper = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Sniper Weapon을 스폰한다."), Sniper))
	{
		UPDItemDefinition* Definition = MakeMagazineItemDefinition(Sniper, 5, false);
		TestTrue(TEXT("단발 무기를 초기화한다."), Sniper->InitializeItem(Definition));
		Sniper->DispatchBeginPlay();
		TestTrue(TEXT("단발 무기를 줍는다."), HeldItems->TryPickUp(Sniper));
		UPDWeaponMagazineComponent* Magazine = Sniper->GetMagazineComponent();
		TestTrue(TEXT("단발 입력을 누른다."), HeldItems->PressHeldItemUse());
		TestEqual(TEXT("Press 한 번에 한 발만 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 4);
		++GFrameCounter;
		TestWorld->GetTimerManager().Tick(0.25f);
		TestEqual(TEXT("누름 유지 중 추가 발사는 없다."),
			Magazine->GetCurrentMagazineAmmo(), 4);
		HeldItems->ReleaseHeldItemUse();
		TestTrue(TEXT("두 번째 단발 입력을 누른다."),
			HeldItems->PressHeldItemUse());
		TestEqual(TEXT("두 번째 Press에서 한 발을 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 3);
		HeldItems->ReleaseHeldItemUse();
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDItemInteractionInputTest,
	"PADO.Item.Interaction.PickUpAndDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDItemInteractionInputTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("상호작용 입력 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	Holder->GetPDAbilitySystemComponent()->InitAbilityActorInfo(Holder, Holder);
	if (!TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	UPDHeldItemComponent* HeldItems = Holder->GetHeldItemComponent();
	APDWorldItemActor* Item = TestWorld->SpawnActor<APDWorldItemActor>();
	if (!TestNotNull(TEXT("World Item을 스폰한다."), Item))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	TestTrue(TEXT("아이템을 초기화한다."),
		Item->InitializeItem(MakeMagazineItemDefinition(Item, 5, false)));
	Item->DispatchBeginPlay();

	// 서버 권한에서는 RequestPickUp이 곧바로 TryPickUp으로 확정된다.
	TestTrue(TEXT("상호작용 요청으로 아이템을 줍는다."),
		HeldItems->RequestPickUp(Item));
	TestTrue(TEXT("줍기 후 Held Item이 일치한다."),
		HeldItems->GetHeldItem() == Item);
	TestTrue(TEXT("아이템 상태가 Held로 바뀐다."),
		Item->GetItemState() == EPDWorldItemState::Held);
	TestTrue(TEXT("아이템 Holder가 캐릭터다."),
		Item->GetHolder() == Holder);

	APDWorldItemActor* Second = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("두 번째 World Item을 스폰한다."), Second))
	{
		TestTrue(TEXT("두 번째 아이템을 초기화한다."),
			Second->InitializeItem(MakeMagazineItemDefinition(Second, 5, false)));
		Second->DispatchBeginPlay();

		// 이미 들고 있으면 상호작용 입력은 아무것도 바꾸지 않는다.
		Holder->Interact();
		TestTrue(TEXT("보유 중 상호작용 입력이 아이템을 교체하지 않는다."),
			HeldItems->GetHeldItem() == Item);
		TestFalse(TEXT("보유 중에는 다른 아이템 줍기를 거부한다."),
			HeldItems->RequestPickUp(Second));
		TestTrue(TEXT("거부된 대상은 World 상태로 남는다."),
			Second->GetItemState() == EPDWorldItemState::World);

		// 근접 후보 선택 검사에 끼어들지 않게 치운다. 이 아이템은 Holder와
		// 같은 지점에 스폰돼 있어서 두면 언제나 가장 가까운 후보가 된다.
		Second->Destroy();
	}

	// 드롭 입력 경로다.
	Holder->DropHeldItem();
	TestNull(TEXT("드롭 후 Held Item이 비워진다."), HeldItems->GetHeldItem());
	TestTrue(TEXT("드롭한 아이템이 World 상태로 돌아간다."),
		Item->GetItemState() == EPDWorldItemState::World);
	TestNull(TEXT("드롭한 아이템의 Holder가 해제된다."), Item->GetHolder());

	// 내려놓은 위치는 줍기 반경 안이므로 다시 집을 수 있다.
	TestTrue(TEXT("드롭 후 다시 주울 수 있다."), HeldItems->RequestPickUp(Item));
	Holder->DropHeldItem();

	// 드롭한 아이템은 캐릭터보다 낮은 바닥에 놓인다. 시선 Sweep이 수평이면
	// 스쳐 지나가므로 근접 후보 선택이 이 경우를 받아 줘야 한다.
	Item->SetActorLocation(
		Holder->GetActorLocation() + FVector(120.0f, 0.0f, -76.0f));
	TestTrue(TEXT("바닥 높이의 드롭 아이템을 근접 후보로 고른다."),
		Holder->FindNearestPickupCandidate() == Item);
	TestTrue(TEXT("상호작용 대상 탐색이 같은 아이템을 돌려준다."),
		Holder->FindInteractTarget() == Item);

	// 여러 후보가 있으면 가장 가까운 것을 고른다.
	APDWorldItemActor* Farther = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("두 번째 후보를 스폰한다."), Farther))
	{
		TestTrue(TEXT("두 번째 후보를 초기화한다."),
			Farther->InitializeItem(MakeMagazineItemDefinition(Farther, 5, false)));
		Farther->DispatchBeginPlay();
		Farther->SetActorLocation(
			Holder->GetActorLocation() + FVector(200.0f, 0.0f, -76.0f));
		TestTrue(TEXT("가까운 쪽 후보를 고른다."),
			Holder->FindNearestPickupCandidate() == Item);
		Farther->Destroy();
	}

	// 줍기 반경 밖으로 굴러간 아이템은 후보에서 빠진다.
	Item->SetActorLocation(
		Holder->GetActorLocation() +
		FVector(HeldItems->GetMaxPickupDistance() + 100.0f, 0.0f, -76.0f));
	TestNull(TEXT("줍기 반경 밖 아이템은 고르지 않는다."),
		Holder->FindNearestPickupCandidate());

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDAimStateTransitionTest,
	"PADO.Character.Aim.StateTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDAimStateTransitionTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("조준 단계 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	Holder->GetPDAbilitySystemComponent()->InitAbilityActorInfo(Holder, Holder);
	if (!TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	// 기본값에서는 무기가 없으면 어떤 단계로도 올라가지 못한다.
	TestFalse(TEXT("맨손에서는 조준 조건을 만족하지 않는다."),
		Holder->CanEnterAimState());
	Holder->StartShouldering();
	TestTrue(TEXT("맨손 견착 입력은 Idle을 유지한다."),
		Holder->GetAimState() == EPDAimState::Idle);
	Holder->ToggleAiming();
	TestTrue(TEXT("맨손 조준 토글은 Idle을 유지한다."),
		Holder->GetAimState() == EPDAimState::Idle);

	UPDHeldItemComponent* HeldItems = Holder->GetHeldItemComponent();
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (!TestNotNull(TEXT("무기를 스폰한다."), Weapon))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
	TestTrue(TEXT("무기를 초기화한다."),
		Weapon->InitializeItem(MakeMagazineItemDefinition(Weapon, 5, false)));
	Weapon->DispatchBeginPlay();
	TestTrue(TEXT("무기를 든다."), HeldItems->RequestPickUp(Weapon));

	// Idle -> 견착 -> Idle
	Holder->StartShouldering();
	TestTrue(TEXT("누르고 있으면 견착으로 간다."),
		Holder->GetAimState() == EPDAimState::Shouldered);
	Holder->StartShouldering();
	TestTrue(TEXT("견착 진입은 여러 번 호출해도 같다."),
		Holder->GetAimState() == EPDAimState::Shouldered);
	Holder->StopShouldering();
	TestTrue(TEXT("떼면 Idle로 돌아온다."),
		Holder->GetAimState() == EPDAimState::Idle);

	// Idle -> 조준 -> Idle
	Holder->ToggleAiming();
	TestTrue(TEXT("짧게 누르면 조준으로 간다."),
		Holder->GetAimState() == EPDAimState::Aiming);
	Holder->ToggleAiming();
	TestTrue(TEXT("다시 짧게 누르면 Idle로 간다."),
		Holder->GetAimState() == EPDAimState::Idle);

	// 조준 중 꾹 누르면 견착으로 내려오고, 떼면 Idle로 간다.
	Holder->ToggleAiming();
	TestTrue(TEXT("조준을 켠다."),
		Holder->GetAimState() == EPDAimState::Aiming);
	Holder->StartShouldering();
	TestTrue(TEXT("조준 중 누르고 있으면 견착으로 내려온다."),
		Holder->GetAimState() == EPDAimState::Shouldered);
	Holder->StopShouldering();
	TestTrue(TEXT("떼면 Idle로 간다."),
		Holder->GetAimState() == EPDAimState::Idle);

	// 토글로 켠 조준은 입력을 떼도 유지한다.
	Holder->ToggleAiming();
	Holder->StopShouldering();
	TestTrue(TEXT("조준은 Hold 해제로 풀리지 않는다."),
		Holder->GetAimState() == EPDAimState::Aiming);

	// 카메라가 목표 배치로 수렴한다.
	USpringArmComponent* CameraBoom = Holder->GetCameraBoom();
	if (TestNotNull(TEXT("Camera Boom을 얻는다."), CameraBoom))
	{
		const float StartArmLength = CameraBoom->TargetArmLength;
		Holder->SetAimState(EPDAimState::Shouldered);
		for (int32 Step = 0; Step < 120; ++Step)
		{
			Holder->Tick(1.0f / 60.0f);
		}
		TestTrue(TEXT("견착에서 팔 길이가 줄어든다."),
			CameraBoom->TargetArmLength < StartArmLength);
		TestTrue(TEXT("견착에서 카메라가 옆으로 치우친다."),
			!CameraBoom->SocketOffset.IsNearlyZero());
	}

	// 무기를 잃으면 다음 Tick에서 Idle로 내려간다.
	Holder->SetAimState(EPDAimState::Aiming);
	Holder->DropHeldItem();
	Holder->Tick(1.0f / 60.0f);
	TestTrue(TEXT("무기를 잃으면 Idle로 돌아온다."),
		Holder->GetAimState() == EPDAimState::Idle);

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDMovementStanceSpeedTest,
	"PADO.Character.Movement.StanceSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDMovementStanceSpeedTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("이동 속도 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	UPDCharacterMovementComponent* Movement = Holder->GetPDCharacterMovement();
	if (!TestNotNull(TEXT("커스텀 무브먼트가 붙어 있다."), Movement))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	UPDAbilitySystemComponent* AbilitySystem = Holder->GetPDAbilitySystemComponent();
	AbilitySystem->InitAbilityActorInfo(Holder, Holder);
	if (!TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	// 어트리뷰트 변경 델리게이트는 캐릭터의 BeginPlay 경계에서 연결된다.
	Holder->DispatchBeginPlay();

	const float BaseSpeed = AbilitySystem->GetNumericAttribute(
		UPDMovementAttributeSet::GetMoveSpeedAttribute());
	TestTrue(TEXT("어트리뷰트 기본 속도가 0보다 크다."), BaseSpeed > 0.0f);
	TestEqual(TEXT("무브먼트가 어트리뷰트 속도를 받아 간다."),
		Movement->GetAttributeMoveSpeed(), BaseSpeed, 0.01f);
	TestEqual(TEXT("평상시 자세 배율은 1이다."),
		Movement->GetStanceSpeedMultiplier(), 1.0f);

	// 자세 배율은 어트리뷰트와 독립적으로 곱해진다.
	Movement->SetAimState(EPDAimState::Shouldered);
	TestEqual(TEXT("견착 배율이 적용된다."),
		Movement->GetStanceSpeedMultiplier(),
		Movement->ShoulderedSpeedMultiplier);

	Movement->SetAimState(EPDAimState::Aiming);
	TestEqual(TEXT("조준 배율이 적용된다."),
		Movement->GetStanceSpeedMultiplier(),
		Movement->AimingSpeedMultiplier);

	// 조준 중에는 달리지 않는다.
	Movement->SetWantsToSprint(true);
	TestFalse(TEXT("조준 중에는 스프린트가 불가능하다."), Movement->CanSprint());
	TestEqual(TEXT("조준 중 스프린트 입력은 배율을 바꾸지 않는다."),
		Movement->GetStanceSpeedMultiplier(),
		Movement->AimingSpeedMultiplier);

	Movement->SetAimState(EPDAimState::Idle);
	TestEqual(TEXT("평상시에는 스프린트 배율이 적용된다."),
		Movement->GetStanceSpeedMultiplier(),
		Movement->SprintSpeedMultiplier);
	Movement->SetWantsToSprint(false);

	// 슬로우는 어트리뷰트 계층에 붙고, 자세 배율과 곱해진다.
	UPDGE_MoveSpeedMultiplier* SlowEffect =
		NewObject<UPDGE_MoveSpeedMultiplier>(GetTransientPackage());
	FGameplayEffectContextHandle EffectContext = AbilitySystem->MakeEffectContext();
	FGameplayEffectSpec SlowSpec(SlowEffect, EffectContext, 1.0f);
	SlowSpec.SetSetByCallerMagnitude(TAG_PD_Data_MoveSpeed_Multiplier, 0.5f);
	AbilitySystem->ApplyGameplayEffectSpecToSelf(SlowSpec);

	TestEqual(TEXT("50% 슬로우가 어트리뷰트 값을 절반으로 만든다."),
		AbilitySystem->GetNumericAttribute(
			UPDMovementAttributeSet::GetMoveSpeedAttribute()),
		BaseSpeed * 0.5f, 0.01f);
	TestEqual(TEXT("바뀐 어트리뷰트가 무브먼트까지 전달된다."),
		Movement->GetAttributeMoveSpeed(), BaseSpeed * 0.5f, 0.01f);

	Movement->SetAimState(EPDAimState::Shouldered);
	TestEqual(
		TEXT("견착과 슬로우가 곱해진다."),
		Movement->GetAttributeMoveSpeed() * Movement->GetStanceSpeedMultiplier(),
		BaseSpeed * 0.5f * Movement->ShoulderedSpeedMultiplier,
		0.01f);

	// 슬로우 두 개는 감소량이 더해지지 않고 곱해져야 한다.
	FGameplayEffectSpec SecondSlowSpec(SlowEffect, EffectContext, 1.0f);
	SecondSlowSpec.SetSetByCallerMagnitude(TAG_PD_Data_MoveSpeed_Multiplier, 0.5f);
	AbilitySystem->ApplyGameplayEffectSpecToSelf(SecondSlowSpec);
	TestEqual(TEXT("슬로우 두 개는 0이 아니라 0.25배가 된다."),
		AbilitySystem->GetNumericAttribute(
			UPDMovementAttributeSet::GetMoveSpeedAttribute()),
		BaseSpeed * 0.25f, 0.01f);

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDMovementStancePredictionTest,
	"PADO.Character.Movement.StancePrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDMovementStancePredictionTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("예측 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = TestWorld->SpawnActor<APDPlayerCharacter>();
	UPDCharacterMovementComponent* Movement =
		Holder ? Holder->GetPDCharacterMovement() : nullptr;
	if (!TestNotNull(TEXT("커스텀 무브먼트를 얻는다."), Movement))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

	// 자세가 압축 플래그를 왕복해도 그대로 복원돼야 서버 재생이 일치한다.
	const EPDAimState States[] = {
		EPDAimState::Idle,
		EPDAimState::Shouldered,
		EPDAimState::Aiming
	};

	for (const EPDAimState State : States)
	{
		for (int32 SprintStep = 0; SprintStep < 2; ++SprintStep)
		{
			const bool bSprint = SprintStep != 0;
			Movement->SetAimState(State);
			Movement->SetWantsToSprint(bSprint);

			FPDSavedMove SavedMove;
			SavedMove.SetMoveFor(Holder, 0.016f, FVector::ZeroVector,
				*static_cast<FNetworkPredictionData_Client_Character*>(
					Movement->GetPredictionData_Client()));

			// 서버가 받는 것은 압축 플래그뿐이다.
			const uint8 Flags = SavedMove.GetCompressedFlags();
			Movement->SetAimState(EPDAimState::Idle);
			Movement->SetWantsToSprint(false);
			Movement->UpdateFromCompressedFlags(Flags);

			TestTrue(
				FString::Printf(TEXT("조준 단계가 압축 플래그를 왕복해도 같다. (%d)"),
					static_cast<int32>(State)),
				Movement->GetAimState() == State);
			TestEqual(
				FString::Printf(TEXT("스프린트 의도가 압축 플래그를 왕복해도 같다. (%d)"),
					static_cast<int32>(State)),
				Movement->WantsToSprint(), bSprint);
		}
	}

	// 자세가 다른 이동은 합치지 않아야 서버 재생에서 구간이 뭉개지지 않는다.
	Movement->SetAimState(EPDAimState::Idle);
	Movement->SetWantsToSprint(false);
	FNetworkPredictionData_Client_Character& ClientData =
		*static_cast<FNetworkPredictionData_Client_Character*>(
			Movement->GetPredictionData_Client());

	TSharedPtr<FPDSavedMove> WalkMove = MakeShared<FPDSavedMove>();
	WalkMove->SetMoveFor(Holder, 0.016f, FVector::ZeroVector, ClientData);

	Movement->SetWantsToSprint(true);
	TSharedPtr<FPDSavedMove> SprintMove = MakeShared<FPDSavedMove>();
	SprintMove->SetMoveFor(Holder, 0.016f, FVector::ZeroVector, ClientData);

	TestFalse(TEXT("스프린트 여부가 다른 이동은 합치지 않는다."),
		WalkMove->CanCombineWith(SprintMove, Holder, 0.05f));

	DestroyTestWorld(TestWorld);
	return true;
}

#endif
