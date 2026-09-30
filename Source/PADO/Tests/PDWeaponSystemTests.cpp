#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "PADO/Character/PDRecoilComponent.h"
#include "PADO/Item/Trait/PDItemRecoilTrait.h"
#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/SpringArmComponent.h"
#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Effect/PDGE_MoveSpeedMultiplier.h"
#include "PADO/Character/PDCharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayAbilitySpec.h"
#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Cue/PDGameplayCueNotify_Tracer.h"
#include "PADO/AbilitySystem/Definition/PDFireActionDefinition.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Effect/PDGE_ActionCooldown.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Fragment/PDApplyGameplayEffectFragment.h"
#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDAimLineTraceTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Tests/PDCharacterTestUtils.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/Interface/PDReloadableItem.h"
#include "PADO/Item/Fragment/PDConsumeMagazineAmmoFragment.h"
#include "PADO/AbilitySystem/Fragment/PDExecuteGameplayCueFragment.h"
#include "PADO/Item/Tag/PDItemGameplayTags.h"
#include "PADO/Item/Trait/PDItemMagazineTrait.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"

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
		UPDItemMagazineTrait* MagazineTrait =
			NewObject<UPDItemMagazineTrait>(Weapon);
		MagazineTrait->Capacity = MagazineCapacity;
		// 몽타주를 넣지 않으면 재장전이 즉시 끝난다. 테스트 World에는 Holder
		// 스켈레탈 메시가 없어 몽타주를 재생할 수 없으므로 이 경로를 쓴다.
		Weapon->Traits.Add(MagazineTrait);

		// Single Action의 자동 반복 옵션(bAutomatic)을 검증하는 정의다. 누르고 있는
		// 동안 활성화를 다시 열고, 반복 주기는 쿨다운과 같은 값이다. 실제 총기
		// 에셋은 Fire Action이며 PDFireActionTests가 따로 검증한다.
		UPDSingleActionDefinition* Action =
			NewObject<UPDSingleActionDefinition>(Weapon);
		Action->ActionTargeting = NewObject<UPDSelfTargeting>(Action);
		if (bAutomatic)
		{
			Action->bAutomatic = true;
			Action->ActionCooldown.CooldownTag =
				FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Weapon.Fire"));
			Action->ActionCooldown.Duration = 0.1f;
		}

		FPDActionHookStruct ConsumeHook;
		ConsumeHook.HookTag = TAG_PD_ActionHook_OnExecuteStart;
		ConsumeHook.Fragments.Add(
			NewObject<UPDConsumeMagazineAmmoFragment>(Action));
		Action->ActionHooks.Add(MoveTemp(ConsumeHook));
		Weapon->UseAction = Action;
		return Weapon;
	}

	/**
	 * 타이머와 월드 시간을 함께 진행시킨다.
	 *
	 * TimerManager만 돌리면 월드 시간이 흐르지 않는다. 쿨다운은 월드 시간으로
	 * 판정하므로, 같이 올리지 않으면 두 번째 발사가 영원히 막힌다. 프레임
	 * 길이도 실제 프레임처럼 맞춰 둔다. 쿨다운이 프레임 오차를 흡수할 때 쓴다.
	 */
	void AdvanceTestWorld(UWorld* World, float DeltaSeconds)
	{
		if (!World)
		{
			return;
		}

		// FTimerManager는 틱 도중에 등록된 타이머를 다음 틱으로 미룬다.
		// 먼저 빈 틱으로 등록을 반영하지 않으면 방금 건 반복 타이머가
		// 시작하지 않는다.
		++GFrameCounter;
		World->GetTimerManager().Tick(UE_KINDA_SMALL_NUMBER);

		++GFrameCounter;
		World->TimeSeconds += DeltaSeconds;
		World->UnpausedTimeSeconds += DeltaSeconds;
		World->DeltaTimeSeconds = DeltaSeconds;
		World->GetTimerManager().Tick(DeltaSeconds);
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

	/**
	 * 액터 초기화까지 마친 World다. 이후 스폰은 실제 게임처럼
	 * InitializeComponent와 PostInitializeComponents를 거치고, 파괴하면 EndPlay가
	 * 돈다. Epic의 GAS 테스트와 같은 준비 방식이다.
	 */
	UWorld* CreateInitializedTestWorld(FWorldContext*& OutWorldContext)
	{
		UWorld* World = CreateTestWorld(OutWorldContext);
		if (World)
		{
			World->InitializeActorsForPlay(FURL());
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

	TestTrue(TEXT("30발 자동 Single Assault Definition은 유효하다."),
		Assault->Validate(Error));
	TestTrue(TEXT("5발 Single Sniper Definition은 유효하다."),
		Sniper->Validate(Error));

	UPDItemDefinition* AssaultAsset = LoadObject<UPDItemDefinition>(
		nullptr,
		TEXT("/Game/PADO/Item/Definition/DA_Item_AssaultRifle.DA_Item_AssaultRifle"));
	UPDItemDefinition* SniperAsset = LoadObject<UPDItemDefinition>(
		nullptr,
		TEXT("/Game/PADO/Item/Definition/DA_Item_SniperRifle.DA_Item_SniperRifle"));
	// Trait 전환으로 기존 DA의 탄창 직렬화가 끊겼다. 에디터에서 Trait을 다시
	// 넣기 전까지는 여기서 실패한다. 실패가 재저작 필요를 알리는 신호다.
	if (TestNotNull(TEXT("기존 Assault Item DA를 로드한다."), AssaultAsset))
	{
		TestTrue(TEXT("실제 Assault Item DA가 유효하다."), AssaultAsset->Validate(Error));
		const UPDItemMagazineTrait* AssaultMagazine =
			AssaultAsset->FindTrait<UPDItemMagazineTrait>();
		if (TestNotNull(TEXT("Assault Item DA에 탄창 Trait이 있다."), AssaultMagazine))
		{
			TestEqual(TEXT("실제 Assault Item DA 탄창은 30발이다."),
				AssaultMagazine->Capacity, 30);
		}

		// 방아쇠로 쏘는 무기는 Fire Action이다. 실패하면 DA 이전이 남았다는 신호다.
		const UPDFireActionDefinition* AssaultFire =
			Cast<UPDFireActionDefinition>(AssaultAsset->UseAction);
		if (TestNotNull(TEXT("실제 Assault Item DA는 Fire Action을 쓴다."), AssaultFire))
		{
			TestTrue(TEXT("실제 Assault Item DA는 자동 사격이다."),
				AssaultFire->FireMode == EPDFireMode::Automatic);
		}
	}
	if (TestNotNull(TEXT("Sniper Item DA를 로드한다."), SniperAsset))
	{
		TestTrue(TEXT("실제 Sniper Item DA가 유효하다."), SniperAsset->Validate(Error));
		const UPDItemMagazineTrait* SniperAssetMagazine =
			SniperAsset->FindTrait<UPDItemMagazineTrait>();
		if (TestNotNull(TEXT("Sniper Item DA에 탄창 Trait이 있다."), SniperAssetMagazine))
		{
			TestEqual(TEXT("실제 Sniper Item DA 탄창은 5발이다."),
				SniperAssetMagazine->Capacity, 5);
		}

		const UPDFireActionDefinition* SniperFire =
			Cast<UPDFireActionDefinition>(SniperAsset->UseAction);
		if (TestNotNull(TEXT("실제 Sniper Item DA는 Fire Action을 쓴다."), SniperFire))
		{
			TestTrue(TEXT("실제 Sniper Item DA는 반자동 사격이다."),
				SniperFire->FireMode == EPDFireMode::SemiAutomatic);
		}
	}

	UPDItemMagazineTrait* SniperMagazine =
		const_cast<UPDItemMagazineTrait*>(
			Sniper->FindTrait<UPDItemMagazineTrait>());
	if (!TestNotNull(TEXT("Sniper에 탄창 Trait이 있다."), SniperMagazine))
	{
		return false;
	}

	SniperMagazine->Capacity = 0;
	TestFalse(TEXT("0발 탄창은 거부한다."), Sniper->Validate(Error));
	SniperMagazine->Capacity = 5;

	// ReloadSpeed가 0이면 몽타주가 진행하지 않아 재장전이 끝나지 않는다.
	SniperMagazine->ReloadSpeed = 0.0f;
	TestFalse(TEXT("0 이하 ReloadSpeed는 거부한다."), Sniper->Validate(Error));
	SniperMagazine->ReloadSpeed = 1.0f;

	// 몽타주가 없으면 즉시 장전이므로 충전 시각도 0이다.
	TestEqual(TEXT("몽타주가 없으면 충전 시각은 0이다."),
		SniperMagazine->GetReloadCompleteTime(), 0.0f);

	// 같은 Trait이 둘이면 하나가 조용히 무시되므로 저작 단계에서 막는다.
	UPDItemMagazineTrait* DuplicateMagazine =
		NewObject<UPDItemMagazineTrait>(Sniper);
	Sniper->Traits.Add(DuplicateMagazine);
	TestFalse(TEXT("같은 Trait이 중복되면 거부한다."), Sniper->Validate(Error));
	Sniper->Traits.Pop();
	TestTrue(TEXT("중복을 제거하면 다시 유효하다."), Sniper->Validate(Error));
	// 탄약 소비 Fragment 배치는 제작자 재량이다. 없으면 탄약을 소비하지 않을
	// 뿐 게임은 동작하므로 Definition 검증에서 막지 않는다.
	Sniper->UseAction->ActionHooks.Reset();
	TestTrue(
		TEXT("탄약 소비 Fragment가 없어도 Magazine Item Definition은 유효하다."),
		Sniper->Validate(Error));
	// Trait을 빼면 탄창 없는 일반 아이템이 된다.
	Sniper->Traits.Reset();
	TestTrue(TEXT("Trait을 제거한 일반 아이템도 유효하다."),
		Sniper->Validate(Error));
	TestNull(TEXT("제거 후에는 탄창 Trait이 조회되지 않는다."),
		Sniper->FindTrait<UPDItemMagazineTrait>());
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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("Weapon을 스폰한다."), Weapon))
	{
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

		// 몽타주가 없는 무기는 대기 없이 채운다.
		TestTrue(TEXT("빈 탄창에서 재장전을 시작한다."),
			Magazine->TryStartReload());
		TestFalse(TEXT("몽타주가 없으면 재장전 상태로 머물지 않는다."),
			Magazine->IsReloading());
		TestEqual(TEXT("몽타주가 없으면 즉시 탄창을 최대치로 채운다."),
			Magazine->GetCurrentMagazineAmmo(), 5);
		TestFalse(TEXT("가득 찬 탄창은 재장전을 거부한다."),
			Magazine->TryStartReload());

		// 몽타주를 넣으면 충전 시점을 노티파이가 정한다. 이 World의 Holder는
		// AnimInstance가 없어 재생할 수 없으므로 시작 자체를 거부해야 한다.
		// 상태만 켜두면 노티파이가 오지 않아 영원히 재장전 중이 된다.
		UPDItemMagazineTrait* MagazineTrait =
			const_cast<UPDItemMagazineTrait*>(
				Definition->FindTrait<UPDItemMagazineTrait>());
		if (TestNotNull(TEXT("Definition에서 탄창 Trait을 찾는다."), MagazineTrait))
		{
			TestTrue(TEXT("재장전 검증을 위해 한 발 소비한다."),
				Magazine->TryConsumeRound());
			MagazineTrait->ReloadMontage = NewObject<UAnimMontage>(Definition);
			TestFalse(TEXT("몽타주를 재생할 수 없으면 재장전을 시작하지 않는다."),
				Magazine->TryStartReload());
			TestFalse(TEXT("시작하지 못한 재장전은 상태를 남기지 않는다."),
				Magazine->IsReloading());
			TestEqual(TEXT("시작하지 못하면 탄창도 그대로다."),
				Magazine->GetCurrentMagazineAmmo(), 4);
			MagazineTrait->ReloadMontage = nullptr;
		}

		TestTrue(TEXT("Weapon을 드롭한다."),
			Holder->GetHeldItemComponent()->DropHeldItem(FTransform::Identity));
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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
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

		// 자동 발사는 클라이언트가 간격마다 활성화를 다시 여는 방식이다.
		// 한 발이 한 활성화라서 발사마다 신호가 하나씩 나간다.
		int32 LocalShotCount = 0;
		FDelegateHandle ShotHandle = HeldItems->OnLocalShotFired.AddLambda(
			[&LocalShotCount](APDWorldItemActor*) { ++LocalShotCount; });

		TestTrue(TEXT("연사 입력을 누른다."), HeldItems->PressHeldItemUse());
		TestEqual(TEXT("누르는 즉시 첫 발사 신호가 나간다."), LocalShotCount, 1);
		TestEqual(TEXT("Press 직후 첫 발을 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 1);

		AdvanceTestWorld(TestWorld, 0.11f);
		TestEqual(TEXT("누름 유지 시 두 번째 발사 신호가 나간다."),
			LocalShotCount, 2);
		TestEqual(TEXT("누름 유지 시 두 번째 발을 소비한다."),
			Magazine->GetCurrentMagazineAmmo(), 0);

		AdvanceTestWorld(TestWorld, 0.11f);
		// 빈 탄창은 활성화 자체가 거부된다. 탄약 판정을 Fragment 하나에 두고
		// 양쪽이 같이 보므로, 나가지 않을 발사에 연출이나 반동이 붙지 않는다.
		TestEqual(TEXT("빈 탄창에서는 발사 신호가 나가지 않는다."),
			LocalShotCount, 2);
		TestEqual(TEXT("빈 탄창에서는 탄약이 더 줄지 않는다."),
			Magazine->GetCurrentMagazineAmmo(), 0);

		HeldItems->ReleaseHeldItemUse();
		HeldItems->OnLocalShotFired.Remove(ShotHandle);
		AdvanceTestWorld(TestWorld, 0.11f);
		TestEqual(TEXT("입력을 떼면 반복이 멈춘다."), LocalShotCount, 2);
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
		AdvanceTestWorld(TestWorld, 0.25f);
		TestEqual(TEXT("자동이 아니면 누름 유지 중 추가 발사는 없다."),
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
	FPDLocalCooldownClockTest,
	"PADO.GAS.Cooldown.LocalClock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDLocalCooldownClockTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("쿨다운 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		UPDAbilitySystemComponent* AbilitySystem = Holder->GetPDAbilitySystemComponent();
		const FGameplayTag CooldownTag =
			FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Weapon.Fire"));
		auto SetFrame = [TestWorld](double Time, float FrameDelta)
		{
			TestWorld->TimeSeconds = Time;
			TestWorld->DeltaTimeSeconds = FrameDelta;
		};

		SetFrame(0.0, 0.016f);
		AbilitySystem->BeginLocalActionCooldown(CooldownTag, 0.1f);
		TestTrue(TEXT("시작 직후에는 쿨다운 중이다."),
			AbilitySystem->IsLocalActionCooldownActive(CooldownTag));
		TestEqual(TEXT("UI는 로컬 기록의 남은 시간을 본다."),
			AbilitySystem->GetActionCooldownRemaining(CooldownTag), 0.1f, 1.0e-4f);

		SetFrame(0.1, 0.016f);
		TestFalse(TEXT("시작한 시각에서 쿨다운만큼 지나면 끝난다."),
			AbilitySystem->IsLocalActionCooldownActive(CooldownTag));

		// 프레임 단위로 조금 늦게 불린 반복은 예정된 시각에 이어 센다.
		// 호출 시각에서 새로 세면 늦은 만큼 간격이 매번 벌어진다.
		SetFrame(0.13, 0.05f);
		AbilitySystem->BeginLocalActionCooldown(CooldownTag, 0.1f);
		TestEqual(TEXT("한 프레임 안에 늦은 반복은 예정 시각에 이어 센다."),
			AbilitySystem->GetActionCooldownRemaining(CooldownTag), 0.07f, 1.0e-4f);

		// 프레임이 길면 한 프레임에 반복이 둘 이상 몰린다. 둘 다 나가야 반복
		// 속도가 프레임 속도와 무관해진다.
		SetFrame(0.35, 0.16f);
		TestFalse(TEXT("긴 프레임에서 첫 반복은 막히지 않는다."),
			AbilitySystem->IsLocalActionCooldownActive(CooldownTag));
		AbilitySystem->BeginLocalActionCooldown(CooldownTag, 0.1f);
		TestFalse(TEXT("같은 프레임에 몰린 두 번째 반복도 막히지 않는다."),
			AbilitySystem->IsLocalActionCooldownActive(CooldownTag));
		AbilitySystem->BeginLocalActionCooldown(CooldownTag, 0.1f);
		TestTrue(TEXT("밀린 반복을 다 소화하면 다시 쿨다운 중이다."),
			AbilitySystem->IsLocalActionCooldownActive(CooldownTag));

		// 한 프레임 넘게 지나서 누른 입력은 누른 시각부터 센다. 끝난 시각에 이어
		// 세면 다음 입력이 쿨다운보다 짧은 간격으로 나갈 수 있다.
		SetFrame(1.0, 0.016f);
		AbilitySystem->BeginLocalActionCooldown(CooldownTag, 0.1f);
		TestEqual(TEXT("늦게 누른 입력은 누른 시각부터 센다."),
			AbilitySystem->GetActionCooldownRemaining(CooldownTag), 0.1f, 1.0e-4f);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDLocalCooldownOverReplicatedTest,
	"PADO.Item.Weapon.Cooldown.IgnoresLateServerCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDLocalCooldownOverReplicatedTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("쿨다운 발사 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("무기를 스폰한다."), Weapon))
	{
		UPDAbilitySystemComponent* AbilitySystem = Holder->GetPDAbilitySystemComponent();
		TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder));
		UPDHeldItemComponent* HeldItems = Holder->GetHeldItemComponent();
		UPDItemDefinition* Definition = MakeMagazineItemDefinition(Weapon, 5, true);
		TestTrue(TEXT("쿨다운이 있는 무기를 초기화한다."), Weapon->InitializeItem(Definition));
		Weapon->DispatchBeginPlay();
		TestTrue(TEXT("무기를 줍는다."), HeldItems->TryPickUp(Weapon));
		UPDWeaponMagazineComponent* Magazine = Weapon->GetMagazineComponent();
		const FGameplayTag CooldownTag =
			FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Weapon.Fire"));

		TestTrue(TEXT("첫 발을 쏜다."), HeldItems->PressHeldItemUse());
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("첫 발을 소비한다."), Magazine->GetCurrentMagazineAmmo(), 4);

		TestFalse(TEXT("쿨다운 중에는 다시 쏠 수 없다."), HeldItems->PressHeldItemUse());
		TestEqual(TEXT("쿨다운 중에는 탄약이 줄지 않는다."),
			Magazine->GetCurrentMagazineAmmo(), 4);

		// 타이머를 돌리지 않고 시간만 넘긴다. 쿨다운 GE가 아직 남아 있어,
		// 서버 쿨다운이 늦게 끝나는 상황과 같다.
		TestWorld->TimeSeconds += 0.1;
		TestWorld->DeltaTimeSeconds = 0.016f;
		TestTrue(TEXT("쿨다운 GE 태그는 아직 남아 있다."),
			AbilitySystem->HasMatchingGameplayTag(CooldownTag));
		TestTrue(TEXT("로컬 기록이 끝났으면 늦게 끝나는 GE에 막히지 않고 쏜다."),
			HeldItems->PressHeldItemUse());
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("두 번째 발을 소비한다."), Magazine->GetCurrentMagazineAmmo(), 3);
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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
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
	FPDGameplayCueTagTest,
	"PADO.Item.Weapon.GameplayCueTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDGameplayCueTagTest::RunTest(const FString& Parameters)
{
	// Fragment의 Validate가 이 루트 태그를 요구한다. 자식 태그를 등록하면
	// 부모가 자동 생성되므로 별도 선언 없이도 유효해야 한다.
	const FGameplayTag CueRoot =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayCue"), false);
	TestTrue(TEXT("GameplayCue 루트 태그가 존재한다."), CueRoot.IsValid());

	// Config/DefaultGameplayTags.ini에 선언한 태그다. 네이티브가 아니므로
	// ini가 실제로 로드됐는지까지 여기서 확인한다.
	const TCHAR* WeaponCueNames[] = {
		TEXT("GameplayCue.Weapon.AssaultRifle.Fire"),
		TEXT("GameplayCue.Weapon.AssaultRifle.Tracer"),
		TEXT("GameplayCue.Weapon.SniperRifle.Fire"),
		TEXT("GameplayCue.Weapon.SniperRifle.Tracer"),
		TEXT("GameplayCue.Weapon.Impact.Default")
	};

	for (const TCHAR* CueName : WeaponCueNames)
	{
		const FGameplayTag CueTag =
			FGameplayTag::RequestGameplayTag(FName(CueName), false);
		if (!TestTrue(
			FString::Printf(TEXT("'%s'가 등록되어 있다."), CueName),
			CueTag.IsValid()))
		{
			continue;
		}
		TestTrue(
			FString::Printf(TEXT("'%s'가 GameplayCue 하위다."), CueName),
			CueTag.MatchesTag(CueRoot));
	}

	// Fragment는 GameplayCue 하위가 아닌 태그를 거부해야 한다.
	UPDExecuteGameplayCueFragment* Fragment =
		NewObject<UPDExecuteGameplayCueFragment>(GetTransientPackage());
	FString Error;
	Fragment->CueTag = FGameplayTag();
	TestFalse(TEXT("빈 CueTag는 거부한다."), Fragment->Validate(Error));

	Fragment->CueTag = TAG_PD_Item_Id_Weapon_AssaultRifle;
	TestFalse(TEXT("GameplayCue 하위가 아닌 태그는 거부한다."),
		Fragment->Validate(Error));

	Fragment->CueTag = FGameplayTag::RequestGameplayTag(
		TEXT("GameplayCue.Weapon.AssaultRifle.Fire"), false);
	TestTrue(TEXT("무기 발사 Cue 태그는 통과한다."), Fragment->Validate(Error));

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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	if (!TestNotNull(TEXT("Holder를 스폰한다."), Holder))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}
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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
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
	if (!TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)))
	{
		DestroyTestWorld(TestWorld);
		return false;
	}

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

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDWeaponRecoilTraitTest,
	"PADO.Item.Weapon.Recoil.Trait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDWeaponRecoilTraitTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	UPDItemDefinition* Weapon = NewObject<UPDItemDefinition>();
	Weapon->ItemId = TAG_PD_Item_Id_Weapon_AssaultRifle;
	Weapon->Presentation.StaticMesh = NewObject<UStaticMesh>(Weapon);
	Weapon->Presentation.bSimulatePhysicsInWorld = false;

	UPDItemRecoilTrait* Recoil = NewObject<UPDItemRecoilTrait>(Weapon);
	Weapon->Traits.Add(Recoil);

	FString Error;
	TestTrue(TEXT("기본 반동 설정은 유효하다."), Weapon->Validate(Error));
	// 기존 무기 DA는 이 값을 저장한 적이 없다. 기본값이 곧 기존 동작이다.
	TestTrue(TEXT("복원은 기본으로 켜져 있다."), Recoil->bEnableRecovery);

	// 범위가 뒤집히면 좌우 반동이 한쪽으로만 나가 저작 의도와 달라진다.
	Recoil->YawPerShotMin = 1.0f;
	Recoil->YawPerShotMax = -1.0f;
	TestFalse(TEXT("좌우 반동 범위가 뒤집히면 거부한다."), Weapon->Validate(Error));
	Recoil->YawPerShotMin = -0.4f;
	Recoil->YawPerShotMax = 0.4f;
	TestTrue(TEXT("범위를 되돌리면 다시 유효하다."), Weapon->Validate(Error));

	// NaN은 컨트롤 회전까지 그대로 전파된다.
	Recoil->PitchPerShot = FMath::Sqrt(-1.0f);
	TestFalse(TEXT("유한하지 않은 반동 수치는 거부한다."), Weapon->Validate(Error));
	Recoil->PitchPerShot = 1.2f;

	// 커브를 안 꽂으면 매 발 같은 배율이다. 나중에 커브만 넣으면 패턴이 된다.
	TestEqual(TEXT("커브가 없으면 첫 발 배율은 1이다."),
		Recoil->GetSprayMultiplier(0), 1.0f);
	TestEqual(TEXT("커브가 없으면 열 번째 발도 배율 1이다."),
		Recoil->GetSprayMultiplier(9), 1.0f);

	// 반동은 선택 기능이다. Trait을 빼면 값 자체가 없다.
	Weapon->Traits.Reset();
	TestNull(TEXT("Trait을 빼면 반동 설정이 조회되지 않는다."),
		Weapon->FindTrait<UPDItemRecoilTrait>());
	TestTrue(TEXT("반동 없는 아이템도 유효하다."), Weapon->Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDWeaponRecoilComponentTest,
	"PADO.Item.Weapon.Recoil.LocalOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDWeaponRecoilComponentTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("반동 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("Weapon을 스폰한다."), Weapon))
	{
		UPDRecoilComponent* RecoilComponent = Holder->GetRecoilComponent();
		if (TestNotNull(TEXT("캐릭터에 반동 컴포넌트가 있다."), RecoilComponent))
		{
			TestTrue(TEXT("Holder 손 소켓을 구성한다."),
				ConfigureHolderSocket(Holder));
			UPDItemDefinition* Definition =
				MakeMagazineItemDefinition(Weapon, 5, true);
			Definition->Traits.Add(NewObject<UPDItemRecoilTrait>(Definition));
			TestTrue(TEXT("반동 무기를 초기화한다."),
				Weapon->InitializeItem(Definition));

			// 이 World의 Holder는 로컬 조종 Controller가 없다. 반동은 컨트롤
			// 회전 조작이라 그 경우 아무것도 하지 않아야 한다. 서버나 시뮬레이션
			// 프록시에서 돌면 남의 시점을 흔들게 된다.
			Weapon->DispatchBeginPlay();
			TestTrue(TEXT("반동 무기를 줍는다."),
				Holder->GetHeldItemComponent()->TryPickUp(Weapon));
			Holder->GetHeldItemComponent()->PressHeldItemUse();
			TestFalse(TEXT("로컬 조종이 아니면 반동이 쌓이지 않는다."),
				RecoilComponent->HasActiveRecoil());
			Holder->GetHeldItemComponent()->ReleaseHeldItemUse();
		}
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDShotResultContractTest,
	"PADO.Item.Weapon.ShotResult.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDShotResultContractTest::RunTest(const FString& Parameters)
{
	UPDSingleActionDefinition* Action = NewObject<UPDSingleActionDefinition>();
	Action->ActionTargeting = NewObject<UPDAimLineTraceTargeting>(Action);

	UPDExecuteGameplayCueFragment* Tracer =
		NewObject<UPDExecuteGameplayCueFragment>(Action);
	Tracer->CueTag = FGameplayTag::RequestGameplayTag(
		TEXT("GameplayCue.Weapon.AssaultRifle.Fire"), false);
	Tracer->ApplicationScope = EPDActionScope::Source;
	Tracer->bPlayAtShotEnd = true;

	FPDActionHookStruct Hook;
	Hook.HookTag = TAG_PD_ActionHook_OnExecuteStart;
	Hook.Fragments.Add(Tracer);
	Action->ActionHooks.Add(MoveTemp(Hook));

	FString Error;
	TestTrue(TEXT("선을 긋는 Targeting의 OnExecuteStart에서는 이번 발 결과를 쓸 수 있다."),
		Action->ValidateWithActionContract(Error));

	// 결과는 OnExecuteStart에만 온다. 다른 Hook에 두면 조용히 재생되지 않는다.
	Action->ActionHooks[0].HookTag = TAG_PD_ActionHook_OnExecute;
	TestFalse(TEXT("OnExecuteStart가 아닌 Hook에 두면 거부한다."),
		Action->ValidateWithActionContract(Error));
	Action->ActionHooks[0].HookTag = TAG_PD_ActionHook_OnExecuteStart;

	Action->ActionTargeting = NewObject<UPDSelfTargeting>(Action);
	TestFalse(TEXT("결과를 만들지 않는 Targeting이면 거부한다."),
		Action->ValidateWithActionContract(Error));

	// 소켓으로 위치를 덮으면 멈춘 곳이 사라진다.
	Tracer->ItemSocketName = TEXT("Muzzle");
	TestFalse(TEXT("ItemSocketName과 함께 쓰면 거부한다."), Tracer->Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDAimLineTraceShotResultTest,
	"PADO.Item.Weapon.ShotResult.AimLineTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDAimLineTraceShotResultTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("판정 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	// 컨트롤러가 없으면 눈높이에서 Actor 정면(+X)으로 쏜다.
	APDPlayerCharacter* Shooter = TestWorld->SpawnActor<APDPlayerCharacter>();
	UPDAimLineTraceTargeting* Targeting =
		NewObject<UPDAimLineTraceTargeting>(GetTransientPackage());
	Targeting->TraceDistance = 1000.0f;
	if (TestNotNull(TEXT("사수를 스폰한다."), Shooter))
	{
		FPDActionTargetingContext Context;
		Context.SourceActor = Shooter;

		FPDActionTargetingResult Miss;
		Targeting->GatherTargets(Context, Miss);
		TestTrue(TEXT("빗나가도 이번 발 결과가 있다."), Miss.bHasShotResult);
		TestFalse(TEXT("빗나간 탄은 막히지 않았다."), Miss.ShotResult.bBlockingHit);
		TestEqual(TEXT("빗나간 탄은 사거리 끝에서 멈춘다."),
			Miss.ShotResult.ImpactPoint, Miss.ShotResult.TraceEnd);
		TestEqual(TEXT("사거리는 TraceDistance다."),
			FVector::Dist(Miss.ShotResult.TraceStart, Miss.ShotResult.TraceEnd),
			1000.0, 1.0);

		// 1단계 판정이 벽을 맞히면 끝점이 벽 표면 위에 온다. 그래도 벽에서
		// 멈춰야 한다. 표면에서 딱 끝나는 판정은 오차로 벽을 놓칠 수 있다.
		AActor* Wall = TestWorld->SpawnActor<AActor>();
		UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
		WallBox->SetBoxExtent(FVector(10.0f, 300.0f, 300.0f));
		WallBox->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		WallBox->SetWorldLocation(FVector(500.0f, 0.0f, 0.0f));
		Wall->SetRootComponent(WallBox);
		WallBox->RegisterComponent();

		FPDActionTargetingResult WallHit;
		Targeting->GatherTargets(Context, WallHit);
		TestTrue(TEXT("벽에 쏜 탄은 막힌다."), WallHit.ShotResult.bBlockingHit);
		TestTrue(TEXT("벽에서 멈춘다."), WallHit.ShotResult.GetActor() == Wall);
		TestEqual(TEXT("벽 앞면에서 멈춘다."),
			WallHit.ShotResult.ImpactPoint.X, 490.0, 1.0);
		TestEqual(TEXT("벽은 대상이 아니다."), WallHit.Targets.Num(), 0);

		// 벽 앞 대상에 맞으면 대상에서 멈춘다. 대상을 지나 벽까지 가지 않는다.
		APDPlayerCharacter* Target = TestWorld->SpawnActor<APDPlayerCharacter>(
			FVector(300.0f, 0.0f, 0.0f),
			FRotator::ZeroRotator);
		if (TestNotNull(TEXT("대상을 스폰한다."), Target))
		{
			FPDActionTargetingResult TargetHit;
			Targeting->GatherTargets(Context, TargetHit);
			TestEqual(TEXT("대상 하나를 맞힌다."), TargetHit.Targets.Num(), 1);
			TestTrue(TEXT("대상에 맞은 탄도 막힌 것이다."),
				TargetHit.ShotResult.bBlockingHit);
			TestTrue(TEXT("대상에서 멈춘다."),
				TargetHit.ShotResult.GetActor() == Target);
			TestTrue(TEXT("벽보다 앞에서 멈춘다."),
				TargetHit.ShotResult.ImpactPoint.X < 490.0);
		}
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDTracerCueTest,
	"PADO.Item.Weapon.TracerCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDTracerCueTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	// 렌더링할 수 없는 환경(-nullrhi, 데디케이티드 서버)에서는 엔진이 Niagara
	// 컴포넌트를 만들지 않는다. 검증할 대상이 없으므로 건너뛴다.
	if (!FApp::CanEverRender())
	{
		AddInfo(TEXT("렌더링할 수 없는 환경이라 트레이서 Cue 검증을 건너뜁니다."));
		return true;
	}

	UNiagaraSystem* TracerSystem = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/KIC/VFX/Gun/BulletLaser/NS_BulletTracer.NS_BulletTracer"));
	UNiagaraSystem* ImpactSystem = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/KIC/Trap/LaserTrap/NS_LaserSpark1.NS_LaserSpark1"));
	if (!TestNotNull(TEXT("트레이서 Niagara System을 로드한다."), TracerSystem) ||
		!TestNotNull(TEXT("탄착 Niagara System을 로드한다."), ImpactSystem))
	{
		return false;
	}

	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("트레이서 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	auto FindComponents = [TestWorld](const UNiagaraSystem* System)
	{
		TArray<UNiagaraComponent*> Found;
		for (TObjectIterator<UNiagaraComponent> It; It; ++It)
		{
			if (IsValid(*It) && It->GetWorld() == TestWorld && It->GetAsset() == System)
			{
				Found.Add(*It);
			}
		}
		return Found;
	};

	AActor* Shooter = TestWorld->SpawnActor<AActor>();
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("사수를 스폰한다."), Shooter) &&
		TestNotNull(TEXT("무기를 스폰한다."), Weapon))
	{
		UPDItemDefinition* Definition = MakeMagazineItemDefinition(Weapon, 30, true);
		UStaticMeshSocket* MuzzleSocket =
			NewObject<UStaticMeshSocket>(Definition->Presentation.StaticMesh);
		MuzzleSocket->SocketName = TEXT("Muzzle");
		MuzzleSocket->RelativeLocation = FVector(50.0f, 0.0f, 0.0f);
		Definition->Presentation.StaticMesh->Sockets.Add(MuzzleSocket);
		TestTrue(TEXT("총구 소켓이 있는 무기를 초기화한다."), Weapon->InitializeItem(Definition));
		Weapon->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
		const FVector Muzzle = Weapon->GetItemMesh()->GetSocketLocation(TEXT("Muzzle"));

		UPDGameplayCueNotify_Tracer* Notify =
			NewObject<UPDGameplayCueNotify_Tracer>(GetTransientPackage());
		Notify->TracerSystem = TracerSystem;
		Notify->ImpactSystem = ImpactSystem;
		Notify->TracerSpeed = 10000.0f;

		// 판정 시작점을 총구와 다르게 둔다. 출발점은 판정이 아니라 이 화면의
		// 총구여야 한다.
		auto MakeShotCue = [Shooter, Weapon](const FVector& StopPoint, bool bBlocked)
		{
			FHitResult Shot(FVector(0.0f, 0.0f, 160.0f), FVector(3000.0f, 0.0f, 100.0f));
			Shot.bBlockingHit = bBlocked;
			Shot.Location = StopPoint;
			Shot.ImpactPoint = StopPoint;
			Shot.Normal = FVector(-1.0f, 0.0f, 0.0f);
			Shot.ImpactNormal = Shot.Normal;

			FGameplayEffectContextHandle Context(new FGameplayEffectContext());
			Context.AddInstigator(Shooter, Weapon);
			Context.AddHitResult(Shot, true);
			FGameplayCueParameters CueParameters(Context);
			CueParameters.EffectCauser = Weapon;
			return CueParameters;
		};

		const FVector WallPoint(1000.0f, 0.0f, 100.0f);
		TestTrue(TEXT("막힌 탄의 Cue를 처리한다."),
			Notify->OnExecute(Shooter, MakeShotCue(WallPoint, true)));

		TArray<UNiagaraComponent*> Tracers = FindComponents(TracerSystem);
		if (TestEqual(TEXT("트레이서를 하나 스폰한다."), Tracers.Num(), 1))
		{
			TestEqual(TEXT("트레이서는 총구에서 출발한다."),
				Tracers[0]->GetComponentLocation(), Muzzle, 0.1f);
			TestNull(TEXT("트레이서는 총에 붙지 않는다."),
				Tracers[0]->GetAttachParent());
			bool bHasEnd = false;
			const FVector TracerEnd =
				Tracers[0]->GetVariablePosition(TEXT("TracerEnd"), bHasEnd);
			TestTrue(TEXT("트레이서에 끝점을 넘긴다."), bHasEnd);
			TestEqual(TEXT("끝점은 이번 발이 멈춘 곳이다."), TracerEnd, WallPoint, 0.1f);
		}

		// 탄착은 탄 머리가 끝점에 닿을 때 나온다. 950cm / 10000cm/s = 0.095초.
		TestEqual(TEXT("탄착은 바로 나오지 않는다."), FindComponents(ImpactSystem).Num(), 0);
		AdvanceTestWorld(TestWorld, 0.05f);
		TestEqual(TEXT("도착 전에는 탄착이 없다."), FindComponents(ImpactSystem).Num(), 0);
		AdvanceTestWorld(TestWorld, 0.1f);
		TArray<UNiagaraComponent*> Impacts = FindComponents(ImpactSystem);
		if (TestEqual(TEXT("도착하면 탄착이 하나 나온다."), Impacts.Num(), 1))
		{
			TestEqual(TEXT("탄착은 끝점에서 나온다."),
				Impacts[0]->GetComponentLocation(), WallPoint, 0.1f);
		}

		// 빗나간 탄은 허공에서 멈췄다. 트레이서만 날리고 탄착은 없다.
		TestTrue(TEXT("빗나간 탄의 Cue를 처리한다."),
			Notify->OnExecute(Shooter, MakeShotCue(FVector(3000.0f, 0.0f, 100.0f), false)));
		AdvanceTestWorld(TestWorld, 1.0f);
		TestEqual(TEXT("빗나간 탄은 탄착을 남기지 않는다."),
			FindComponents(ImpactSystem).Num(), 1);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

namespace PDFireActionTests
{
	using namespace PDWeaponSystemTests;

	constexpr double Frame60 = 1.0 / 60.0;

	UPDItemDefinition* MakeFireItemDefinition(
		UObject* Outer,
		int32 MagazineCapacity,
		EPDFireMode FireMode,
		float ShotInterval)
	{
		UPDItemDefinition* Weapon = NewObject<UPDItemDefinition>(Outer);
		Weapon->ItemId = TAG_PD_Item_Id_Weapon_AssaultRifle;
		Weapon->DisplayName = FText::FromString(TEXT("Automation Fire Weapon"));
		Weapon->Presentation.StaticMesh = NewObject<UStaticMesh>(Weapon);
		Weapon->Presentation.bSimulatePhysicsInWorld = false;
		UPDItemMagazineTrait* MagazineTrait =
			NewObject<UPDItemMagazineTrait>(Weapon);
		MagazineTrait->Capacity = MagazineCapacity;
		Weapon->Traits.Add(MagazineTrait);

		UPDFireActionDefinition* Action =
			NewObject<UPDFireActionDefinition>(Weapon);
		Action->ActionTargeting = NewObject<UPDSelfTargeting>(Action);
		Action->FireMode = FireMode;
		Action->ShotInterval = ShotInterval;

		FPDActionHookStruct ConsumeHook;
		ConsumeHook.HookTag = TAG_PD_ActionHook_OnExecuteStart;
		ConsumeHook.Fragments.Add(
			NewObject<UPDConsumeMagazineAmmoFragment>(Action));
		Action->ActionHooks.Add(MoveTemp(ConsumeHook));
		Weapon->UseAction = Action;
		return Weapon;
	}

	/**
	 * 월드 시간과 타이머를 같은 양만큼 진행한다. 발사 일정은 월드 시간으로 발
	 * 시각을 정하고 타이머로 깨어나므로, 둘이 어긋나면 발이 프레임 단위로 밀려
	 * 셈이 흔들린다. 앞 프레임에 건 타이머를 먼저 반영하려고 빈 틱을 한 번 돈다.
	 */
	void AdvanceFrame(UWorld* World, double DeltaSeconds)
	{
		++GFrameCounter;
		World->GetTimerManager().Tick(0.0f);

		++GFrameCounter;
		World->TimeSeconds += DeltaSeconds;
		World->UnpausedTimeSeconds += DeltaSeconds;
		World->DeltaTimeSeconds = static_cast<float>(DeltaSeconds);
		World->GetTimerManager().Tick(static_cast<float>(DeltaSeconds));
	}

	void AdvanceFrames(UWorld* World, double DeltaSeconds, int32 FrameCount)
	{
		for (int32 Index = 0; Index < FrameCount; ++Index)
		{
			AdvanceFrame(World, DeltaSeconds);
		}
	}

	/** Fire Action 무기를 든 사수 한 명이다. 로컬 발사 신호를 센다. */
	struct FFireRig
	{
		UWorld* World = nullptr;
		APDPlayerCharacter* Holder = nullptr;
		APDWorldItemActor* Weapon = nullptr;
		UPDHeldItemComponent* HeldItems = nullptr;
		UPDWeaponMagazineComponent* Magazine = nullptr;
		int32 ShotCount = 0;
		FDelegateHandle ShotHandle;

		bool SetUp(TFunctionRef<UPDItemDefinition*(UObject*)> MakeDefinition)
		{
			FWorldContext* WorldContext = nullptr;
			World = CreateTestWorld(WorldContext);
			if (!World)
			{
				return false;
			}

			Holder = PDCharacterTestUtils::SpawnPlayerCharacter(World);
			Weapon = World->SpawnActor<APDWorldItemActor>();
			if (!Holder || !Weapon)
			{
				return false;
			}

			if (!ConfigureHolderSocket(Holder) ||
				!Weapon->InitializeItem(MakeDefinition(Weapon)))
			{
				return false;
			}

			Weapon->DispatchBeginPlay();
			HeldItems = Holder->GetHeldItemComponent();
			if (!HeldItems->TryPickUp(Weapon))
			{
				return false;
			}

			Magazine = Weapon->GetMagazineComponent();
			ShotHandle = HeldItems->OnLocalShotFired.AddLambda(
				[this](APDWorldItemActor*) { ++ShotCount; });
			return Magazine != nullptr;
		}

		void TearDown()
		{
			if (HeldItems)
			{
				HeldItems->OnLocalShotFired.Remove(ShotHandle);
			}
			DestroyTestWorld(World);
			World = nullptr;
		}

		FGameplayAbilitySpecHandle GetAbilityHandle() const
		{
			return Weapon->GetAbilitySourceComponent()->GetGrantedAbilityHandle();
		}

		UPDGA_FireAction* FindFireAction() const
		{
			const FGameplayAbilitySpec* Spec =
				Holder->GetPDAbilitySystemComponent()->FindAbilitySpecFromHandle(
					GetAbilityHandle());
			return Spec ? Cast<UPDGA_FireAction>(Spec->GetPrimaryInstance()) : nullptr;
		}

		/** 서버가 받을 발 묶음이다. 대상이 있으면 발마다 그 대상에 맞힌다. */
		FPDFireShotBatchStruct MakeBatch(
			int32 FirstShotIndex,
			int32 BatchShotCount,
			AActor* HitTarget) const
		{
			FPDFireShotBatchStruct Batch;
			Batch.AbilityHandle = GetAbilityHandle();
			Batch.SourceObject = Weapon->GetAbilitySourceComponent();
			for (int32 Offset = 0; Offset < BatchShotCount; ++Offset)
			{
				FPDFireShotStruct& Shot = Batch.Shots.AddDefaulted_GetRef();
				Shot.ShotIndex = FirstShotIndex + Offset;
				if (HitTarget)
				{
					const FHitResult Hit(
						HitTarget,
						nullptr,
						HitTarget->GetActorLocation(),
						FVector::BackwardVector);
					Shot.Hits.Add(FPDFireShotHitStruct::Make(HitTarget, &Hit));
				}
			}
			return Batch;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionScheduleTest,
	"PADO.Item.Weapon.FireAction.Schedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionScheduleTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;

	struct FFrameCase
	{
		const TCHAR* Name;
		TArray<double> FramePattern;
	};
	const FFrameCase FrameCases[] = {
		{ TEXT("60fps"), { Frame60 } },
		{ TEXT("30fps"), { 1.0 / 30.0 } },
		{ TEXT("144fps"), { 1.0 / 144.0 } },
		{ TEXT("불규칙 프레임"), { 0.005, 0.03, 0.012, 0.041 } }
	};

	// 발 간격 0.1초 자동이면 0.95초 동안 0, 0.1, ..., 0.9초에 쏜다. 프레임 길이와
	// 무관하게 10발이어야 한다. 한 프레임이 발 간격보다 짧은 경우만 다룬다.
	for (const FFrameCase& FrameCase : FrameCases)
	{
		FFireRig Rig;
		if (!TestTrue(
			FString::Printf(TEXT("%s: 자동 무기를 든다."), FrameCase.Name),
			Rig.SetUp([](UObject* Outer)
			{
				return MakeFireItemDefinition(Outer, 100, EPDFireMode::Automatic, 0.1f);
			})))
		{
			Rig.TearDown();
			continue;
		}

		UPDGA_FireAction* FireAction = Rig.FindFireAction();
		TestTrue(
			FString::Printf(TEXT("%s: 무기를 들면 Fire Action이 활성화된다."), FrameCase.Name),
			FireAction && FireAction->IsActive());

		TestTrue(TEXT("방아쇠를 당긴다."), Rig.HeldItems->PressHeldItemUse());
		TestEqual(
			FString::Printf(TEXT("%s: 누르는 즉시 첫 발이 나간다."), FrameCase.Name),
			Rig.ShotCount,
			1);

		double Elapsed = 0.0;
		int32 FrameIndex = 0;
		constexpr double Duration = 0.95;
		while (Elapsed < Duration - UE_KINDA_SMALL_NUMBER)
		{
			const double Delta = FMath::Min(
				FrameCase.FramePattern[FrameIndex++ % FrameCase.FramePattern.Num()],
				Duration - Elapsed);
			AdvanceFrame(Rig.World, Delta);
			Elapsed += Delta;
		}

		TestEqual(
			FString::Printf(TEXT("%s: 0.95초 동안 10발이다."), FrameCase.Name),
			Rig.ShotCount,
			10);
		TestEqual(
			FString::Printf(TEXT("%s: 쏜 만큼 탄약을 쓴다."), FrameCase.Name),
			Rig.Magazine->GetCurrentMagazineAmmo(),
			90);

		Rig.HeldItems->ReleaseHeldItemUse();
		AdvanceFrames(Rig.World, Frame60, 30);
		TestEqual(
			FString::Printf(TEXT("%s: 떼면 멈춘다."), FrameCase.Name),
			Rig.ShotCount,
			10);
		Rig.TearDown();
	}

	// 발 간격이 프레임보다 짧으면 한 프레임에 여러 발이 나간다. 그래도 연사력은
	// 발 간격대로다. 0.02초 간격을 30fps로 0.9667초 돌리면 0, 0.02, ..., 0.96초의
	// 49발이다.
	FFireRig FastRig;
	if (TestTrue(
		TEXT("발 간격이 프레임보다 짧은 무기를 든다."),
		FastRig.SetUp([](UObject* Outer)
		{
			return MakeFireItemDefinition(Outer, 100, EPDFireMode::Automatic, 0.02f);
		})))
	{
		FastRig.HeldItems->PressHeldItemUse();
		AdvanceFrames(FastRig.World, 1.0 / 30.0, 2);
		TestEqual(TEXT("두 번째 프레임에는 두 발이 함께 나간다."), FastRig.ShotCount, 4);
		AdvanceFrames(FastRig.World, 1.0 / 30.0, 27);
		TestEqual(TEXT("프레임보다 짧은 간격도 연사력이 줄지 않는다."), FastRig.ShotCount, 49);
		FastRig.HeldItems->ReleaseHeldItemUse();
	}
	FastRig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionTriggerModesTest,
	"PADO.Item.Weapon.FireAction.TriggerModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionTriggerModesTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;

	FFireRig SemiRig;
	if (TestTrue(
		TEXT("반자동 무기를 든다."),
		SemiRig.SetUp([](UObject* Outer)
		{
			return MakeFireItemDefinition(Outer, 30, EPDFireMode::SemiAutomatic, 0.25f);
		})))
	{
		UPDHeldItemComponent* HeldItems = SemiRig.HeldItems;
		HeldItems->PressHeldItemUse();
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("반자동은 누를 때 한 발이다."), SemiRig.ShotCount, 1);

		AdvanceFrames(SemiRig.World, Frame60, 6);
		HeldItems->PressHeldItemUse();
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("준비 전에 누른 입력은 버린다."), SemiRig.ShotCount, 1);

		AdvanceFrames(SemiRig.World, Frame60, 12);
		HeldItems->PressHeldItemUse();
		TestEqual(TEXT("준비된 뒤 누르면 바로 나간다."), SemiRig.ShotCount, 2);
		AdvanceFrames(SemiRig.World, Frame60, 60);
		TestEqual(TEXT("반자동은 누르고 있어도 더 쏘지 않는다."), SemiRig.ShotCount, 2);
		HeldItems->ReleaseHeldItemUse();
	}
	SemiRig.TearDown();

	FFireRig BurstRig;
	if (TestTrue(
		TEXT("점사 무기를 든다."),
		BurstRig.SetUp([](UObject* Outer)
		{
			UPDItemDefinition* Definition =
				MakeFireItemDefinition(Outer, 30, EPDFireMode::Burst, 0.05f);
			UPDFireActionDefinition* Action =
				CastChecked<UPDFireActionDefinition>(Definition->UseAction);
			Action->BurstCount = 3;
			Action->BurstCooldown = 0.3f;
			return Definition;
		})))
	{
		UPDHeldItemComponent* HeldItems = BurstRig.HeldItems;
		HeldItems->PressHeldItemUse();
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("점사는 누르는 즉시 첫 발이 나간다."), BurstRig.ShotCount, 1);

		// 0.05, 0.1초에 나머지 두 발이 나간다. 중간에 떼도 끝까지 쏜다.
		AdvanceFrames(BurstRig.World, Frame60, 12);
		TestEqual(TEXT("점사는 떼도 정해진 발 수를 모두 쏜다."), BurstRig.ShotCount, 3);

		// 마지막 발(0.1초) 뒤 0.3초가 지나야 다음 점사다.
		HeldItems->PressHeldItemUse();
		HeldItems->ReleaseHeldItemUse();
		TestEqual(TEXT("점사 대기 중에 누른 입력은 버린다."), BurstRig.ShotCount, 3);

		AdvanceFrames(BurstRig.World, Frame60, 15);
		HeldItems->PressHeldItemUse();
		AdvanceFrames(BurstRig.World, Frame60, 12);
		TestEqual(TEXT("대기가 끝나면 다음 점사를 쏜다."), BurstRig.ShotCount, 6);
		AdvanceFrames(BurstRig.World, Frame60, 60);
		TestEqual(TEXT("점사는 누르고 있어도 한 번뿐이다."), BurstRig.ShotCount, 6);
		HeldItems->ReleaseHeldItemUse();
	}
	BurstRig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionShotGateTest,
	"PADO.Item.Weapon.FireAction.ShotGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionShotGateTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;

	FFireRig Rig;
	if (TestTrue(
		TEXT("3발 자동 무기를 든다."),
		Rig.SetUp([](UObject* Outer)
		{
			return MakeFireItemDefinition(Outer, 3, EPDFireMode::Automatic, 0.1f);
		})))
	{
		UPDHeldItemComponent* HeldItems = Rig.HeldItems;
		UPDAbilitySystemComponent* AbilitySystem =
			Rig.Holder->GetPDAbilitySystemComponent();

		HeldItems->PressHeldItemUse();
		AdvanceFrames(Rig.World, Frame60, 60);
		TestEqual(TEXT("탄약만큼만 쏜다."), Rig.ShotCount, 3);
		TestEqual(TEXT("탄창이 비었다."), Rig.Magazine->GetCurrentMagazineAmmo(), 0);

		// 탄약 소진은 방아쇠 입력을 끝낸다. 재장전 뒤 누르고 있던 채로 이어 쏘지 않는다.
		// 서버의 재장전이다. 인터페이스 경로(Execute_TryStartReload)는 Actor가
		// 초기화되지 않은 테스트 World에서 ProcessEvent가 건너뛰므로 직접 부른다.
		TestTrue(TEXT("재장전한다."), Rig.Magazine->TryStartReload());
		TestEqual(TEXT("재장전으로 탄창이 찬다."), Rig.Magazine->GetCurrentMagazineAmmo(), 3);
		AdvanceFrames(Rig.World, Frame60, 30);
		TestEqual(TEXT("재장전 뒤에는 다시 눌러야 쏜다."), Rig.ShotCount, 3);

		HeldItems->ReleaseHeldItemUse();
		HeldItems->PressHeldItemUse();
		TestEqual(TEXT("다시 누르면 쏜다."), Rig.ShotCount, 4);

		// Block은 발사만 멈추고 방아쇠 상태는 유지한다. 풀리면 이어 쏜다.
		const FGameplayTagContainer ActionTags(TAG_PD_Ability_Action);
		AbilitySystem->BlockAbilitiesWithTags(ActionTags);
		AdvanceFrames(Rig.World, Frame60, 30);
		TestEqual(TEXT("막힌 동안에는 쏘지 않는다."), Rig.ShotCount, 4);
		TestEqual(TEXT("막힌 동안에는 탄약이 줄지 않는다."),
			Rig.Magazine->GetCurrentMagazineAmmo(), 2);

		AbilitySystem->UnBlockAbilitiesWithTags(ActionTags);
		AdvanceFrames(Rig.World, Frame60, 20);
		TestEqual(TEXT("풀리면 누르고 있던 사격이 이어진다."), Rig.ShotCount, 6);
		TestEqual(TEXT("이어 쏜 만큼 탄약을 쓴다."),
			Rig.Magazine->GetCurrentMagazineAmmo(), 0);
		HeldItems->ReleaseHeldItemUse();
	}
	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionServerProcessingTest,
	"PADO.Item.Weapon.FireAction.ServerProcessing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionServerProcessingTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;

	// 맞은 횟수를 대상의 태그 수로 센다. 지속 GE 하나가 태그 하나를 붙인다.
	const FGameplayTag HitMarkTag =
		FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Weapon.Fire"));

	FFireRig Rig;
	if (!TestTrue(
		TEXT("대상에게 결과를 주는 무기를 든다."),
		Rig.SetUp([HitMarkTag](UObject* Outer)
		{
			UPDItemDefinition* Definition =
				MakeFireItemDefinition(Outer, 30, EPDFireMode::Automatic, 0.1f);
			UPDAbilityDefinition* Action = Definition->UseAction;
			UPDApplyGameplayEffectFragment* Mark =
				NewObject<UPDApplyGameplayEffectFragment>(Action);
			Mark->EffectRecipe.EffectClass = UPDGE_ActionCooldown::StaticClass();
			Mark->EffectRecipe.DynamicGrantedTags.AddTag(HitMarkTag);
			FPDSetByCallerValueStruct& Duration =
				Mark->EffectRecipe.SetByCallers.AddDefaulted_GetRef();
			Duration.DataTag = TAG_PD_Data_Cooldown_Duration;
			Duration.Magnitude = 60.0f;

			FPDActionHookStruct ExecuteHook;
			ExecuteHook.HookTag = TAG_PD_ActionHook_OnExecute;
			ExecuteHook.Fragments.Add(Mark);
			Action->ActionHooks.Add(MoveTemp(ExecuteHook));
			return Definition;
		})))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Target = PDCharacterTestUtils::SpawnPlayerCharacter(
		Rig.World,
		FVector(300.0f, 0.0f, 0.0f));
	UPDGA_FireAction* FireAction = Rig.FindFireAction();
	if (TestNotNull(TEXT("대상을 스폰한다."), Target) &&
		TestNotNull(TEXT("활성 Fire Action을 찾는다."), FireAction))
	{
		UPDAbilitySystemComponent* TargetAbilitySystem =
			Target->GetPDAbilitySystemComponent();
		UPDWeaponMagazineComponent* Magazine = Rig.Magazine;

		// 받은 판정을 다시 추적하지 않고 그대로 적용한다. 대상은 사수 뒤에 있지만
		// 기록이 맞았다고 하면 맞은 것이다.
		FireAction->ProcessShotBatch(Rig.MakeBatch(1, 2, Target));
		TestEqual(TEXT("발마다 탄약을 쓴다."), Magazine->GetCurrentMagazineAmmo(), 28);
		TestEqual(TEXT("기록된 명중을 발마다 적용한다."),
			TargetAbilitySystem->GetTagCount(HitMarkTag), 2);

		// 명중의 주체는 사수의 PlayerState, 물리적 원인은 무기다. 사수의 몸이
		// 먼저 사라져도 주체가 남도록 Instigator에 몸을 넣지 않는다.
		const TArray<FActiveGameplayEffectHandle> HitEffects =
			TargetAbilitySystem->GetActiveEffects(FGameplayEffectQuery());
		if (TestTrue(TEXT("명중 효과가 대상에 남아 있다."), HitEffects.Num() > 0))
		{
			const FActiveGameplayEffect* HitEffect =
				TargetAbilitySystem->GetActiveGameplayEffect(HitEffects[0]);
			const FGameplayEffectContextHandle HitContext = HitEffect
				? HitEffect->Spec.GetContext()
				: FGameplayEffectContextHandle();
			TestTrue(TEXT("명중의 Instigator는 사수의 PlayerState다."),
				HitContext.GetInstigator() == Rig.Holder->GetPlayerState());
			TestTrue(TEXT("명중의 EffectCauser는 무기다."),
				HitContext.GetEffectCauser() == Rig.Weapon);
			TestTrue(TEXT("명중의 Instigator ASC는 사수의 ASC다."),
				HitContext.GetInstigatorAbilitySystemComponent() ==
					Rig.Holder->GetPDAbilitySystemComponent());
		}

		FPDFireShotBatchStruct OtherWeapon = Rig.MakeBatch(3, 1, Target);
		OtherWeapon.SourceObject = Target;
		FireAction->ProcessShotBatch(OtherWeapon);
		FPDFireShotBatchStruct OtherSpec = Rig.MakeBatch(3, 1, Target);
		OtherSpec.AbilityHandle = FGameplayAbilitySpecHandle();
		FireAction->ProcessShotBatch(OtherSpec);
		TestEqual(TEXT("다른 무기나 다른 Spec의 묶음은 버린다."),
			Magazine->GetCurrentMagazineAmmo(), 28);

		// 핑이 한계를 넘으면 쏜 것은 인정하고 명중만 인정하지 않는다.
		APlayerState* ShooterState = Rig.World->SpawnActor<APlayerState>();
		if (TestNotNull(TEXT("사수의 PlayerState를 스폰한다."), ShooterState))
		{
			Rig.Holder->SetPlayerState(ShooterState);
			ShooterState->ExactPing = FireAction->MaxAcceptedPingMilliseconds + 50.0f;
			FireAction->ProcessShotBatch(Rig.MakeBatch(3, 1, Target));
			TestEqual(TEXT("지연 한계를 넘어도 탄약은 쓴다."),
				Magazine->GetCurrentMagazineAmmo(), 27);
			TestEqual(TEXT("지연 한계를 넘으면 명중을 인정하지 않는다."),
				TargetAbilitySystem->GetTagCount(HitMarkTag), 2);

			ShooterState->ExactPing = FireAction->MaxAcceptedPingMilliseconds - 50.0f;
			FireAction->ProcessShotBatch(Rig.MakeBatch(4, 1, Target));
			TestEqual(TEXT("한계 안이면 명중을 인정한다."),
				TargetAbilitySystem->GetTagCount(HitMarkTag), 3);
			Rig.Holder->SetPlayerState(nullptr);
		}

		// 서버만 아는 무력화는 서버가 우선이다. 막힌 동안 도착한 발은 없었던 것이다.
		UPDAbilitySystemComponent* ShooterAbilitySystem =
			Rig.Holder->GetPDAbilitySystemComponent();
		const FGameplayTagContainer ActionTags(TAG_PD_Ability_Action);
		ShooterAbilitySystem->BlockAbilitiesWithTags(ActionTags);
		FireAction->ProcessShotBatch(Rig.MakeBatch(5, 1, Target));
		TestEqual(TEXT("서버에서 막힌 발은 탄약을 쓰지 않는다."),
			Magazine->GetCurrentMagazineAmmo(), 26);
		TestEqual(TEXT("서버에서 막힌 발은 명중을 적용하지 않는다."),
			TargetAbilitySystem->GetTagCount(HitMarkTag), 3);
		ShooterAbilitySystem->UnBlockAbilitiesWithTags(ActionTags);

		// 탄약보다 많은 발이 오면 남은 탄약까지만 결과를 준다. 넘친 4발은 탄약
		// 소비가 실패했다는 경고를 남긴다.
		AddExpectedMessagePlain(
			TEXT("탄창이 비어 있습니다."),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			4);
		FireAction->ProcessShotBatch(Rig.MakeBatch(6, 30, Target));
		TestEqual(TEXT("탄창을 넘는 발은 무효다."), Magazine->GetCurrentMagazineAmmo(), 0);
		TestEqual(TEXT("무효인 발은 명중을 적용하지 않는다."),
			TargetAbilitySystem->GetTagCount(HitMarkTag), 29);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionAmmoPredictionTest,
	"PADO.Item.Weapon.FireAction.AmmoPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionAmmoPredictionTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateTestWorld(WorldContext);
	if (!TestNotNull(TEXT("탄약 예측 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("무기를 스폰한다."), Weapon) &&
		TestTrue(TEXT("30발 무기를 초기화한다."), Weapon->InitializeItem(
			MakeFireItemDefinition(Weapon, 30, EPDFireMode::Automatic, 0.1f))))
	{
		UPDWeaponMagazineComponent* Magazine = Weapon->GetMagazineComponent();

		// 같은 컴포넌트를 역할만 바꿔 서버와 소유 클라이언트 양쪽에서 본다.
		// 복제로 도착하는 값(탄약, 처리 기록)은 서버 역할일 때 바꾼다.
		auto AsServer = [Weapon]() { Weapon->SetRole(ROLE_Authority); };
		auto AsOwner = [Weapon]() { Weapon->SetRole(ROLE_AutonomousProxy); };
		// Spec을 만들면 새 Handle이 발급된다. 줍기마다 다른 Spec이 생기는 것과 같다.
		auto MakeSpecHandle = []()
		{
			return FGameplayAbilitySpec(UPDGA_FireAction::StaticClass()).Handle;
		};
		const FGameplayAbilitySpecHandle FirstSpec = MakeSpecHandle();
		const FGameplayAbilitySpecHandle OtherOwnerSpec = MakeSpecHandle();
		const FGameplayAbilitySpecHandle NextSpec = MakeSpecHandle();

		AsOwner();
		Magazine->BeginLocalShotSession(FirstSpec, 0);
		for (int32 ShotIndex = 1; ShotIndex <= 3; ++ShotIndex)
		{
			Magazine->RecordLocalShot(FirstSpec, ShotIndex);
		}
		TestEqual(TEXT("서버가 처리하기 전의 발만큼 빼고 본다."),
			Magazine->GetCurrentMagazineAmmo(), 27);

		AsServer();
		TestTrue(TEXT("서버가 첫 발을 처리한다."), Magazine->TryConsumeRound());
		TestTrue(TEXT("서버가 둘째 발을 처리한다."), Magazine->TryConsumeRound());
		Magazine->RecordProcessedShot(FirstSpec, 2);
		TestEqual(TEXT("서버는 실제 탄약을 본다."), Magazine->GetCurrentMagazineAmmo(), 28);
		AsOwner();
		TestEqual(TEXT("처리된 발은 서버 탄약에 이미 들어 있다."),
			Magazine->GetCurrentMagazineAmmo(), 27);

		// 서버가 무효로 한 발도 처리한 발이다. 탄약이 줄지 않았으므로 서버 값에 맞춰진다.
		AsServer();
		Magazine->RecordProcessedShot(FirstSpec, 3);
		AsOwner();
		TestEqual(TEXT("무효가 된 발은 서버 값으로 수렴한다."),
			Magazine->GetCurrentMagazineAmmo(), 28);
		TestEqual(TEXT("미처리 발이 남지 않는다."), Magazine->GetUnprocessedLocalShotCount(), 0);

		// 재장전을 요청하면 서버의 답이 올 때까지 쏘지 않는다.
		TestTrue(TEXT("탄창이 차 있지 않으면 재장전을 요청할 수 있다."),
			Magazine->CanRequestReload());
		Magazine->MarkReloadRequested();
		TestFalse(TEXT("요청한 뒤에는 쏘지 않는다."),
			Magazine->CanConsumeRoundWithReplicatedState());
		TestFalse(TEXT("요청을 겹쳐 보내지 않는다."), Magazine->CanRequestReload());
		Magazine->ClearReloadRequest();
		TestTrue(TEXT("답이 오면 다시 쏜다."), Magazine->CanConsumeRoundWithReplicatedState());

		Magazine->EndLocalShotSession(FirstSpec);
		TestEqual(TEXT("무기를 놓으면 복제된 탄약을 그대로 본다."),
			Magazine->GetCurrentMagazineAmmo(), 28);

		// 다른 줍기(다른 Spec)의 처리 기록은 이번 발과 섞이지 않는다.
		AsServer();
		Magazine->RecordProcessedShot(OtherOwnerSpec, 57);
		AsOwner();
		Magazine->BeginLocalShotSession(NextSpec, 0);
		Magazine->RecordLocalShot(NextSpec, 1);
		TestEqual(TEXT("다른 Spec의 처리 기록은 이번 발을 처리한 것으로 보지 않는다."),
			Magazine->GetCurrentMagazineAmmo(), 27);

		Magazine->RecordLocalShot(NextSpec, 28);
		TestEqual(TEXT("예측 탄약이 바닥나면 0이다."), Magazine->GetCurrentMagazineAmmo(), 0);
		TestFalse(TEXT("예측 탄약이 없으면 쏘지 않는다."),
			Magazine->CanConsumeRoundWithReplicatedState());
		Magazine->EndLocalShotSession(NextSpec);

		AsServer();
		Magazine->RecordLocalShot(NextSpec, 5);
		TestEqual(TEXT("서버는 로컬 발을 빼지 않는다."), Magazine->GetCurrentMagazineAmmo(), 28);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionExecutionScopeTest,
	"PADO.GAS.Fragment.ExecutionScope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionExecutionScopeTest::RunTest(const FString& Parameters)
{
	UPDExecuteGameplayCueFragment* Cue =
		NewObject<UPDExecuteGameplayCueFragment>(GetTransientPackage());
	UPDConsumeMagazineAmmoFragment* Ammo =
		NewObject<UPDConsumeMagazineAmmoFragment>(GetTransientPackage());

	FPDActionExecutionContext Context;
	Context.ExecutionScope = EPDActionExecutionScope::PresentationOnly;
	TestTrue(TEXT("연출 전용 실행은 Cue를 돌린다."), Context.AllowsFragment(*Cue));
	TestFalse(TEXT("연출 전용 실행은 탄약을 쓰지 않는다."), Context.AllowsFragment(*Ammo));
	TestTrue(TEXT("연출 전용 실행은 권한 없이도 재생한다."), Context.CanPlayPresentation());

	Context.ExecutionScope = EPDActionExecutionScope::ResultsOnly;
	TestFalse(TEXT("결과 전용 실행은 Cue를 돌리지 않는다."), Context.AllowsFragment(*Cue));
	TestTrue(TEXT("결과 전용 실행은 탄약을 쓴다."), Context.AllowsFragment(*Ammo));

	Context.ExecutionScope = EPDActionExecutionScope::Predicting;
	TestFalse(TEXT("예측을 켜지 않은 Cue는 예측 실행에서 돌리지 않는다."),
		Context.AllowsFragment(*Cue));
	Cue->bPredictOnOwningClient = true;
	TestTrue(TEXT("예측을 켠 Cue는 예측 실행에서 돌린다."), Context.AllowsFragment(*Cue));
	TestFalse(TEXT("예측 실행은 탄약을 쓰지 않는다."), Context.AllowsFragment(*Ammo));

	Context.ExecutionScope = EPDActionExecutionScope::Authority;
	TestTrue(TEXT("서버 실행은 Cue를 돌린다."), Context.AllowsFragment(*Cue));
	TestTrue(TEXT("서버 실행은 탄약을 쓴다."), Context.AllowsFragment(*Ammo));
	TestFalse(TEXT("권한이 없으면 서버 실행 문맥으로 재생하지 않는다."),
		Context.CanPlayPresentation());

	// 첫 발과 그 뒤 N발마다다. 발 번호가 없는 실행은 매번이다.
	Cue->PlayEveryNthShot = 3;
	TestTrue(TEXT("첫 발에 재생한다."), Cue->ShouldPlayForShot(1));
	TestFalse(TEXT("둘째 발은 건너뛴다."), Cue->ShouldPlayForShot(2));
	TestFalse(TEXT("셋째 발은 건너뛴다."), Cue->ShouldPlayForShot(3));
	TestTrue(TEXT("넷째 발에 재생한다."), Cue->ShouldPlayForShot(4));
	TestTrue(TEXT("발 번호가 없으면 재생한다."), Cue->ShouldPlayForShot(INDEX_NONE));
	Cue->PlayEveryNthShot = 1;
	TestTrue(TEXT("1이면 매 발이다."), Cue->ShouldPlayForShot(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionLifecycleTest,
	"PADO.Item.Weapon.FireAction.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace PDFireActionTests;

	FFireRig Rig;
	if (TestTrue(
		TEXT("자동 무기를 든다."),
		Rig.SetUp([](UObject* Outer)
		{
			return MakeFireItemDefinition(Outer, 30, EPDFireMode::Automatic, 0.1f);
		})))
	{
		UPDAbilitySystemComponent* AbilitySystem =
			Rig.Holder->GetPDAbilitySystemComponent();
		UPDGA_FireAction* FireAction = Rig.FindFireAction();
		TestTrue(TEXT("무기를 들면 활성화된다."), FireAction && FireAction->IsActive());

		// 다른 Ability의 Cancel로 끝나면 다시 들기 전까지 쏠 수 없다.
		AbilitySystem->CancelAbilityHandle(Rig.GetAbilityHandle());
		TestTrue(TEXT("Cancel로는 끝나지 않는다."), FireAction && FireAction->IsActive());

		TestTrue(TEXT("무기를 놓는다."),
			Rig.HeldItems->DropHeldItem(FTransform::Identity));
		TestFalse(TEXT("무기를 놓으면 끝난다."), FireAction && FireAction->IsActive());

		// 막힌 상태로 들어도 활성화는 된다. 막힘은 발마다 본다.
		const FGameplayTagContainer ActionTags(TAG_PD_Ability_Action);
		AbilitySystem->BlockAbilitiesWithTags(ActionTags);
		TestTrue(TEXT("막힌 상태에서 무기를 다시 든다."), Rig.HeldItems->TryPickUp(Rig.Weapon));
		UPDGA_FireAction* ReequippedAction = Rig.FindFireAction();
		TestTrue(TEXT("막힌 상태로 들어도 활성화된다."),
			ReequippedAction && ReequippedAction->IsActive());

		Rig.HeldItems->PressHeldItemUse();
		TestEqual(TEXT("막힌 동안에는 쏘지 않는다."), Rig.ShotCount, 0);
		AbilitySystem->UnBlockAbilitiesWithTags(ActionTags);
		AdvanceFrames(Rig.World, Frame60, 10);
		TestTrue(TEXT("풀리면 누르고 있던 사격이 시작된다."), Rig.ShotCount > 0);
		Rig.HeldItems->ReleaseHeldItemUse();
	}
	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDCharacterAbilitySystemPossessionTest,
	"PADO.Character.AbilitySystem.Possession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDCharacterAbilitySystemPossessionTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateInitializedTestWorld(WorldContext);
	if (!TestNotNull(TEXT("빙의 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Character = TestWorld->SpawnActor<APDPlayerCharacter>();
	APDPlayerState* PlayerState =
		PDCharacterTestUtils::SpawnPlayerState(TestWorld);
	APlayerController* Controller = TestWorld->SpawnActor<APlayerController>();
	if (TestNotNull(TEXT("캐릭터를 스폰한다."), Character) &&
		TestNotNull(TEXT("PlayerState를 스폰한다."), PlayerState) &&
		TestNotNull(TEXT("컨트롤러를 스폰한다."), Controller))
	{
		// 게임에서는 GameMode가 컨트롤러의 PlayerState를 만든다. 빙의하면
		// 엔진이 이 PlayerState를 몸에 넘기고, 캐릭터가 그 ASC에 연결된다.
		Controller->PlayerState = PlayerState;
		PlayerState->SetOwner(Controller);
		Controller->Possess(Character);

		UPDAbilitySystemComponent* AbilitySystem =
			PlayerState->GetPDAbilitySystemComponent();
		TestTrue(TEXT("캐릭터는 PlayerState의 ASC를 쓴다."),
			Character->GetAbilitySystemComponent() == AbilitySystem);
		TestNull(TEXT("캐릭터에는 따로 ASC가 없다."),
			Character->FindComponentByClass<UAbilitySystemComponent>());
		TestTrue(TEXT("ASC의 주체는 PlayerState다."),
			AbilitySystem->GetOwnerActor() == PlayerState);
		TestTrue(TEXT("ASC의 아바타는 캐릭터다."),
			AbilitySystem->GetAvatarActor() == Character);
		TestTrue(TEXT("PlayerState의 Owner 체인으로 컨트롤러를 찾는다."),
			AbilitySystem->AbilityActorInfo->PlayerController.Get() == Controller);

		bool bFoundMoveSpeed = false;
		const float MoveSpeed = AbilitySystem->GetGameplayAttributeValue(
			UPDMovementAttributeSet::GetMoveSpeedAttribute(),
			bFoundMoveSpeed);
		TestTrue(TEXT("이동 속도 Attribute는 PlayerState에 있다."), bFoundMoveSpeed);
		TestEqual(TEXT("빙의 직후 이동 속도가 무브먼트에 반영된다."),
			Character->GetPDCharacterMovement()->GetAttributeMoveSpeed(),
			MoveSpeed,
			0.01f);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDCharacterAbilitySystemUnexpectedPlayerStateTest,
	"PADO.Character.AbilitySystem.UnexpectedPlayerState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDCharacterAbilitySystemUnexpectedPlayerStateTest::RunTest(
	const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateInitializedTestWorld(WorldContext);
	if (!TestNotNull(TEXT("PlayerState 설정 오류 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Character = TestWorld->SpawnActor<APDPlayerCharacter>();
	APlayerState* PlayerState = TestWorld->SpawnActor<APlayerState>();
	APlayerController* Controller = TestWorld->SpawnActor<APlayerController>();
	if (TestNotNull(TEXT("캐릭터를 스폰한다."), Character) &&
		TestNotNull(TEXT("PlayerState를 스폰한다."), PlayerState) &&
		TestNotNull(TEXT("컨트롤러를 스폰한다."), Controller))
	{
		// 빙의는 PossessedBy와 PawnClientRestart 두 경계를 지난다. 설정 오류는
		// 경계마다 반복되지만 한 번만 알린다.
		AddExpectedMessagePlain(
			TEXT("APDPlayerState가 아니라서"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			1);

		const float DefaultSpeed =
			Character->GetPDCharacterMovement()->GetAttributeMoveSpeed();
		Controller->PlayerState = PlayerState;
		PlayerState->SetOwner(Controller);
		Controller->Possess(Character);

		TestNull(TEXT("GameMode 설정이 틀리면 연결할 ASC가 없다."),
			Character->GetAbilitySystemComponent());
		TestEqual(TEXT("ASC가 없어도 이동 속도는 무브먼트 기본값을 유지한다."),
			Character->GetPDCharacterMovement()->GetAttributeMoveSpeed(),
			DefaultSpeed,
			0.01f);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDCharacterAbilitySystemAvatarHandOffTest,
	"PADO.Character.AbilitySystem.AvatarHandOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDCharacterAbilitySystemAvatarHandOffTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateInitializedTestWorld(WorldContext);
	if (!TestNotNull(TEXT("몸 교체 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* FirstBody =
		PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDPlayerCharacter* SecondBody = TestWorld->SpawnActor<APDPlayerCharacter>(
		FVector(300.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator);
	APDPlayerState* PlayerState =
		FirstBody ? FirstBody->GetPlayerState<APDPlayerState>() : nullptr;
	if (TestNotNull(TEXT("첫 번째 몸을 스폰한다."), FirstBody) &&
		TestNotNull(TEXT("두 번째 몸을 스폰한다."), SecondBody) &&
		TestNotNull(TEXT("첫 번째 몸에 PlayerState가 있다."), PlayerState))
	{
		UPDAbilitySystemComponent* AbilitySystem =
			PlayerState->GetPDAbilitySystemComponent();
		const float BaseSpeed = AbilitySystem->GetNumericAttribute(
			UPDMovementAttributeSet::GetMoveSpeedAttribute());

		// 리스폰처럼 같은 PlayerState에 새 몸이 연결된다. 클라이언트에서는
		// 이전 몸의 파괴보다 새 몸이 먼저 도착할 수 있다.
		SecondBody->SetPlayerState(PlayerState);
		SecondBody->InitializeAbilitySystem(AbilitySystem, PlayerState);

		TestTrue(TEXT("아바타가 새 몸으로 넘어간다."),
			AbilitySystem->GetAvatarActor() == SecondBody);
		TestTrue(TEXT("새 몸은 같은 ASC를 쓴다."),
			SecondBody->GetAbilitySystemComponent() == AbilitySystem);
		TestNull(TEXT("이전 몸은 ASC 연결을 잃는다."),
			FirstBody->GetAbilitySystemComponent());

		UPDGE_MoveSpeedMultiplier* SlowEffect =
			NewObject<UPDGE_MoveSpeedMultiplier>(GetTransientPackage());
		FGameplayEffectSpec SlowSpec(
			SlowEffect,
			AbilitySystem->MakeEffectContext(),
			1.0f);
		SlowSpec.SetSetByCallerMagnitude(TAG_PD_Data_MoveSpeed_Multiplier, 0.5f);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(SlowSpec);

		TestEqual(TEXT("바뀐 이동 속도는 새 몸에 반영된다."),
			SecondBody->GetPDCharacterMovement()->GetAttributeMoveSpeed(),
			BaseSpeed * 0.5f,
			0.01f);
		TestEqual(TEXT("이전 몸은 더 이상 이동 속도를 받지 않는다."),
			FirstBody->GetPDCharacterMovement()->GetAttributeMoveSpeed(),
			BaseSpeed,
			0.01f);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDCharacterAbilitySystemAvatarEndPlayTest,
	"PADO.Character.AbilitySystem.AvatarEndPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDCharacterAbilitySystemAvatarEndPlayTest::RunTest(const FString& Parameters)
{
	using namespace PDWeaponSystemTests;
	FWorldContext* WorldContext = nullptr;
	UWorld* TestWorld = CreateInitializedTestWorld(WorldContext);
	if (!TestNotNull(TEXT("몸 파괴 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	APDPlayerCharacter* Holder =
		PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDWorldItemActor* Weapon = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("Weapon을 스폰한다."), Weapon) &&
		TestTrue(TEXT("Holder 손 소켓을 구성한다."), ConfigureHolderSocket(Holder)) &&
		TestTrue(TEXT("Weapon을 초기화한다."),
			Weapon->InitializeItem(MakeMagazineItemDefinition(Weapon, 5, false))))
	{
		// 몸의 EndPlay는 BeginPlay를 거친 액터에서만 돈다.
		Holder->DispatchBeginPlay();
		Weapon->DispatchBeginPlay();

		UPDAbilitySystemComponent* AbilitySystem =
			Holder->GetPDAbilitySystemComponent();
		APlayerState* PlayerState = Holder->GetPlayerState();
		TestTrue(TEXT("무기를 줍는다."),
			Holder->GetHeldItemComponent()->TryPickUp(Weapon));
		const FGameplayAbilitySpecHandle WeaponAbility =
			Weapon->GetAbilitySourceComponent()->GetGrantedAbilityHandle();
		TestNotNull(TEXT("무기 Ability가 PlayerState의 ASC에 부여된다."),
			AbilitySystem->FindAbilitySpecFromHandle(WeaponAbility));

		// 몸이 살아 있을 때 만든 문맥이다. 몸이 사라진 뒤에도 주체가 남아야 한다.
		const FGameplayEffectContextHandle EffectContext =
			AbilitySystem->MakeEffectContext();

		Holder->Destroy();

		TestNull(TEXT("몸이 사라지면 들고 있던 무기의 Ability를 회수한다."),
			AbilitySystem->FindAbilitySpecFromHandle(WeaponAbility));
		TestTrue(TEXT("무기는 다시 월드 아이템이 된다."),
			Weapon->GetItemState() == EPDWorldItemState::World);
		TestNull(TEXT("몸이 사라지면 아바타가 없다."),
			AbilitySystem->GetAvatarActor());
		// 파괴된 액터를 가리키는 약참조도 null을 돌려준다. 몸이 연결을 직접
		// 풀었다면 참조가 파괴된 몸을 가리킨 채 남지 않는다.
		TestFalse(TEXT("몸이 EndPlay에서 아바타 연결을 직접 푼다."),
			AbilitySystem->AbilityActorInfo->AvatarActor.IsStale());
		TestTrue(TEXT("ASC의 주체는 그대로 PlayerState다."),
			AbilitySystem->GetOwnerActor() == PlayerState);
		TestTrue(TEXT("몸이 사라져도 문맥의 주체는 남는다."),
			EffectContext.GetInstigator() == PlayerState);
	}

	DestroyTestWorld(TestWorld);
	return true;
}

#endif
