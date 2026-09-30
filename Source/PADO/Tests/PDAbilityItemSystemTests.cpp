#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "PADO/AbilitySystem/Ability/PDGA_Action.h"
#include "PADO/AbilitySystem/Ability/PDGA_ChannelAction.h"
#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "PADO/AbilitySystem/Definition/PDFireActionDefinition.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Fragment/PDApplyGameplayEffectFragment.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDAimLineTraceTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDEventTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDItemSocketTrailTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDOverlapTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDSweepTargeting.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Tests/PDCharacterTestUtils.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

namespace PDAbilityItemSystemTests
{
	UPDApplyGameplayEffectFragment* AddEffectFragment(
		UPDAbilityDefinition* Definition,
		FPDActionHookStruct& Hook)
	{
		UPDApplyGameplayEffectFragment* Fragment =
			NewObject<UPDApplyGameplayEffectFragment>(Definition);
		Fragment->EffectRecipe.EffectClass = UGameplayEffect::StaticClass();
		Hook.Fragments.Add(Fragment);
		return Fragment;
	}

	UPDSingleActionDefinition* MakeSingleDefinition(
		TSubclassOf<UPDActionTargeting> TargetingClass)
	{
		UPDSingleActionDefinition* Definition =
			NewObject<UPDSingleActionDefinition>();
		Definition->ActionTargeting =
			NewObject<UPDActionTargeting>(Definition, TargetingClass);

		FPDActionHookStruct Hook;
		Hook.HookTag = TAG_PD_ActionHook_OnExecute;
		AddEffectFragment(Definition, Hook);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDActionDefinitionValidationTest,
	"PADO.GAS.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDActionDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace PDAbilityItemSystemTests;
	FString Error;

	for (const TSubclassOf<UPDActionTargeting> TargetingClass :
		{
			TSubclassOf<UPDActionTargeting>(UPDSelfTargeting::StaticClass()),
			TSubclassOf<UPDActionTargeting>(UPDEventTargeting::StaticClass()),
			TSubclassOf<UPDActionTargeting>(UPDOverlapTargeting::StaticClass()),
			TSubclassOf<UPDActionTargeting>(UPDSweepTargeting::StaticClass()),
			TSubclassOf<UPDActionTargeting>(UPDAimLineTraceTargeting::StaticClass())
		})
	{
		UPDSingleActionDefinition* Definition =
			MakeSingleDefinition(TargetingClass);
		TestTrue(
			*FString::Printf(TEXT("%s Definition은 유효하다."),
				*GetNameSafe(TargetingClass.Get())),
			Definition->ValidateWithActionContract(Error));
	}

	UPDSingleActionDefinition* TrailDefinition =
		MakeSingleDefinition(UPDItemSocketTrailTargeting::StaticClass());
	TestFalse(
		TEXT("Socket Trail Single Action은 Montage 없이 사용할 수 없다."),
		TrailDefinition->ValidateWithActionContract(Error));
	TrailDefinition->ActionMontage.Montage =
		NewObject<UAnimMontage>(TrailDefinition);
	TestTrue(
		TEXT("Montage를 가진 Socket Trail Definition은 유효하다."),
		TrailDefinition->ValidateWithActionContract(Error));

	UPDChannelActionDefinition* Channel =
		NewObject<UPDChannelActionDefinition>();
	Channel->ActionTargeting = NewObject<UPDSelfTargeting>(Channel);
	FPDActionHookStruct ChannelHook;
	ChannelHook.HookTag = TAG_PD_ActionHook_OnExecute;
	AddEffectFragment(Channel, ChannelHook);
	Channel->ActionHooks.Add(MoveTemp(ChannelHook));
	// 실행 시점은 몽타주의 Notify뿐이다. 고정 간격 실행은 Fire Action으로 옮겼다.
	TestFalse(
		TEXT("Channel Action은 Montage 없이 사용할 수 없다."),
		Channel->ValidateWithActionContract(Error));
	Channel->ActionMontage.Montage = NewObject<UAnimMontage>(Channel);
	TestTrue(
		TEXT("Montage를 가진 Channel Action은 유효하다."),
		Channel->ValidateWithActionContract(Error));

	TestTrue(
		TEXT("Single Action Ability 클래스는 코드로 고정된다."),
		MakeSingleDefinition(UPDSelfTargeting::StaticClass())->GetAbilityClass() ==
			UPDGA_Action::StaticClass());
	TestTrue(
		TEXT("Channel Action Ability 클래스는 코드로 고정된다."),
		Channel->GetAbilityClass() == UPDGA_ChannelAction::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDFireActionDefinitionValidationTest,
	"PADO.GAS.Definition.FireAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDFireActionDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace PDAbilityItemSystemTests;
	FString Error;

	auto MakeFireDefinition = [](TSubclassOf<UPDActionTargeting> TargetingClass)
	{
		UPDFireActionDefinition* Definition = NewObject<UPDFireActionDefinition>();
		Definition->ActionTargeting =
			NewObject<UPDActionTargeting>(Definition, TargetingClass);
		FPDActionHookStruct Hook;
		Hook.HookTag = TAG_PD_ActionHook_OnExecute;
		AddEffectFragment(Definition, Hook);
		Definition->ActionHooks.Add(MoveTemp(Hook));
		return Definition;
	};

	UPDFireActionDefinition* Fire =
		MakeFireDefinition(UPDAimLineTraceTargeting::StaticClass());
	TestTrue(TEXT("Aim Line Trace Fire Action은 유효하다."),
		Fire->ValidateWithActionContract(Error));
	TestTrue(TEXT("Fire Action Ability 클래스는 코드로 고정된다."),
		Fire->GetAbilityClass() == UPDGA_FireAction::StaticClass());

	// 발 간격이 0이면 발사 일정이 한자리에서 끝없이 쏜다.
	Fire->ShotInterval = 0.0f;
	TestFalse(TEXT("발 간격이 0이면 거부한다."), Fire->ValidateWithActionContract(Error));
	Fire->ShotInterval = 0.1f;

	// 무기를 드는 순간은 발사와 무관하다. 발 단위 Hook만 받는다.
	FPDActionHookStruct StartHook;
	StartHook.HookTag = TAG_PD_ActionHook_OnStart;
	AddEffectFragment(Fire, StartHook);
	Fire->ActionHooks.Add(MoveTemp(StartHook));
	TestFalse(TEXT("Fire Action은 OnStart Hook을 받지 않는다."),
		Fire->ValidateWithActionContract(Error));

	TestFalse(
		TEXT("몽타주 구간이 대상을 정하는 Targeting은 쓸 수 없다."),
		MakeFireDefinition(UPDItemSocketTrailTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	TestFalse(
		TEXT("활성화 대상이 필요한 Targeting은 쓸 수 없다."),
		MakeFireDefinition(UPDEventTargeting::StaticClass())
			->ValidateWithActionContract(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDAbilitySourceResolutionTest,
	"PADO.GAS.Source.Resolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDAbilitySourceResolutionTest::RunTest(const FString& Parameters)
{
	using namespace PDAbilityItemSystemTests;
	FString Error;
	UPDAbilitySourceComponent* Source = NewObject<UPDAbilitySourceComponent>();
	UPDAbilityDefinition* Definition =
		MakeSingleDefinition(UPDSelfTargeting::StaticClass());

	TestTrue(
		TEXT("Grant 전에는 런타임 Definition을 Source에 주입할 수 있다."),
		Source->ConfigureAbilityDefinition(Definition));
	const UPDAbilityDefinition* Resolved = nullptr;
	TestTrue(
		TEXT("Ability Source에서 검증된 Definition을 복원한다."),
		UPDAbilitySystemComponent::ResolveDefinitionFromSource(
			Source, Resolved, &Error));
	TestTrue(
		TEXT("복원 결과는 주입한 정확한 Definition이다."),
		Resolved == Definition);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDItemDefinitionValidationTest,
	"PADO.Item.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDItemDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace PDAbilityItemSystemTests;
	FString Error;
	UPDItemDefinition* Definition = NewObject<UPDItemDefinition>();
	Definition->ItemId = TAG_PD_Ability_Action;
	Definition->DisplayName = FText::FromString(TEXT("Automation Item"));
	Definition->Presentation.StaticMesh = NewObject<UStaticMesh>(Definition);
	Definition->Presentation.bSimulatePhysicsInWorld = false;
	Definition->UseAction =
		MakeSingleDefinition(UPDSelfTargeting::StaticClass());

	TestTrue(TEXT("최소 Item Definition은 유효하다."), Definition->Validate(Error));

	// 메시 구성은 보이는 결과만 달라지고 동작은 유지되므로 막지 않는다.
	Definition->Presentation.SkeletalMesh =
		NewObject<USkeletalMesh>(Definition);
	TestTrue(
		TEXT("Static/Skeletal Mesh를 동시에 지정해도 Definition은 유효하다."),
		Definition->Validate(Error));

	Definition->Presentation.StaticMesh = nullptr;
	Definition->Presentation.SkeletalMesh = nullptr;
	TestTrue(
		TEXT("메시를 지정하지 않아도 Definition은 유효하다."),
		Definition->Validate(Error));

	// 상호작용 자체를 불가능하게 만드는 값만 거부한다.
	Definition->Presentation.CollisionRadius = 0.0f;
	TestFalse(
		TEXT("충돌 반지름이 0인 Definition은 거부한다."),
		Definition->Validate(Error));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHeldItemAbilityLifecycleTest,
	"PADO.Item.AbilityLifecycle.GrantAndRevoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHeldItemAbilityLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace PDAbilityItemSystemTests;
	const FName WorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("PDHeldItemAbilityTestWorld"),
		EUniqueObjectNameOptions::GloballyUnique);
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		WorldName,
		GetTransientPackage());
	if (!TestNotNull(TEXT("아이템 수명주기 검증용 World를 만든다."), TestWorld))
	{
		return false;
	}

	FWorldContext& WorldContext =
		GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	APDPlayerCharacter* Holder = PDCharacterTestUtils::SpawnPlayerCharacter(TestWorld);
	APDWorldItemActor* Item = TestWorld->SpawnActor<APDWorldItemActor>();
	if (TestNotNull(TEXT("Holder를 스폰한다."), Holder) &&
		TestNotNull(TEXT("World Item을 스폰한다."), Item))
	{
		UStaticMesh* HandMeshAsset = NewObject<UStaticMesh>(Holder);
		UStaticMeshSocket* HandSocket =
			NewObject<UStaticMeshSocket>(HandMeshAsset);
		HandSocket->SocketName = TEXT("HandItem");
		HandMeshAsset->Sockets.Add(HandSocket);

		UStaticMeshComponent* HandMesh =
			NewObject<UStaticMeshComponent>(Holder);
		HandMesh->SetupAttachment(Holder->GetRootComponent());
		HandMesh->SetStaticMesh(HandMeshAsset);
		HandMesh->RegisterComponent();

		UPDHeldItemComponent* HeldItems = Holder->GetHeldItemComponent();
		TestTrue(
			TEXT("Holder의 장착 소켓을 설정한다."),
			HeldItems->ConfigureAttachment(HandMesh, TEXT("HandItem")));

		UPDItemDefinition* ItemDefinition = NewObject<UPDItemDefinition>(Item);
		ItemDefinition->ItemId = TAG_PD_Ability_Action;
		ItemDefinition->DisplayName = FText::FromString(TEXT("Lifecycle Item"));
		ItemDefinition->Presentation.StaticMesh =
			NewObject<UStaticMesh>(ItemDefinition);
		ItemDefinition->Presentation.bSimulatePhysicsInWorld = false;
		ItemDefinition->UseAction =
			MakeSingleDefinition(UPDSelfTargeting::StaticClass());

		TestTrue(
			TEXT("World Item에 Definition을 초기화한다."),
			Item->InitializeItem(ItemDefinition));
		TestTrue(TEXT("Holder가 Item을 줍는다."), HeldItems->TryPickUp(Item));
		TestTrue(TEXT("Item이 Held 상태가 된다."),
			Item->GetItemState() == EPDWorldItemState::Held);

		const FGameplayAbilitySpecHandle GrantedHandle =
			Item->GetAbilitySourceComponent()->GetGrantedAbilityHandle();
		TestTrue(TEXT("장착 시 Ability Spec을 부여한다."), GrantedHandle.IsValid());
		TestNotNull(
			TEXT("Holder ASC에서 부여한 Spec을 찾는다."),
			Holder->GetPDAbilitySystemComponent()->FindAbilitySpecFromHandle(
				GrantedHandle));

		TestTrue(
			TEXT("아이템을 드롭한다."),
			HeldItems->DropHeldItem(
				FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 0.0f))));
		TestFalse(
			TEXT("드롭 시 Source의 Ability Handle을 해제한다."),
			Item->GetAbilitySourceComponent()->GetGrantedAbilityHandle().IsValid());
		TestNull(
			TEXT("드롭 시 Holder ASC에서 Ability Spec을 회수한다."),
			Holder->GetPDAbilitySystemComponent()->FindAbilitySpecFromHandle(
				GrantedHandle));
	}

	TestWorld->DestroyWorld(false);
	GEngine->DestroyWorldContext(TestWorld);
	return true;
}

#endif
