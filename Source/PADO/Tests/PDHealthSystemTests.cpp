#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Effect/PDGE_Damage.h"
#include "PADO/AbilitySystem/Effect/PDGE_MoveSpeedMultiplier.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/Character/PDHealthComponent.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Character/PDPlayerController.h"
#include "PADO/Character/PDPlayerState.h"
#include "PADO/Character/PDRespawnComponent.h"
#include "PADO/Interaction/Component/PDInteractionComponent.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Tests/PDCharacterTestUtils.h"
#include "PADO/Tests/PDItemTestUtils.h"
#include "PADO/Tests/PDTestWorldUtils.h"
#include "TimerManager.h"

namespace PDHealthSystemTests
{
	/** 무기 Fragment처럼 Source ASC가 Target ASC에 피해 GE를 적용한다. */
	void ApplyDamage(UAbilitySystemComponent& Source, UAbilitySystemComponent& Target, float Amount)
	{
		const FGameplayEffectSpecHandle Spec = Source.MakeOutgoingSpec(
			UPDGE_Damage::StaticClass(),
			1.0f,
			Source.MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(TAG_PD_Data_Damage, Amount);
		Source.ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), &Target);
	}

	float GetHealth(const UAbilitySystemComponent& AbilitySystem)
	{
		return AbilitySystem.GetNumericAttribute(UPDHealthAttributeSet::GetHealthAttribute());
	}

	/** 빈사 시간은 Blueprint 기본값으로 정하는 값이라 테스트에서는 리플렉션으로 바꾼다. */
	void SetDownedDuration(UPDHealthComponent& Health, float Seconds)
	{
		if (FFloatProperty* Property = FindFProperty<FFloatProperty>(
			UPDHealthComponent::StaticClass(),
			TEXT("DownedDuration")))
		{
			Property->SetPropertyValue_InContainer(&Health, Seconds);
		}
	}

	/**
	 * 타이머만 진행한다. FTimerManager는 틱 밖에서 건 타이머를 다음 틱에 활성화하고,
	 * 같은 프레임에 두 번 돌지 않는다. 빈 틱으로 먼저 활성화한 뒤 시간을 보낸다.
	 */
	void AdvanceTimers(UWorld& World, float Seconds)
	{
		++GFrameCounter;
		World.GetTimerManager().Tick(UE_KINDA_SMALL_NUMBER);
		++GFrameCounter;
		World.GetTimerManager().Tick(Seconds);
	}

	struct FHealthRig
	{
		UWorld* World = nullptr;

		bool SetUp()
		{
			FWorldContext* WorldContext = nullptr;
			World = PDTestWorldUtils::CreateInitializedTestWorld(WorldContext);
			return World != nullptr;
		}

		void TearDown()
		{
			PDTestWorldUtils::DestroyTestWorld(World);
			World = nullptr;
		}

		/** 실제 게임처럼 ASC는 PlayerState가 갖고, 체력도 거기에 있다. */
		APDPlayerCharacter* SpawnPlayer(const FVector& Location) const
		{
			return PDCharacterTestUtils::SpawnPlayerCharacter(World, Location);
		}

		/** 컨트롤러가 빙의한 플레이어다. 컨트롤러는 BeginPlay를 거쳐 리스폰 감시를 시작한다. */
		APDPlayerCharacter* SpawnPossessedPlayer(
			const FVector& Location,
			APDPlayerController*& OutController) const
		{
			APDPlayerCharacter* Character =
				World->SpawnActor<APDPlayerCharacter>(Location, FRotator::ZeroRotator);
			APDPlayerState* PlayerState = PDCharacterTestUtils::SpawnPlayerState(World);
			OutController = World->SpawnActor<APDPlayerController>();
			if (!Character || !PlayerState || !OutController)
			{
				return nullptr;
			}

			OutController->PlayerState = PlayerState;
			PlayerState->SetOwner(OutController);
			OutController->DispatchBeginPlay();
			OutController->Possess(Character);
			return Character;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthDamageBetweenPlayersTest,
	"PADO.Health.Damage.BetweenPlayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthDamageBetweenPlayersTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* First = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	APDPlayerCharacter* Second = Rig.SpawnPlayer(FVector(300.0f, 0.0f, 100.0f));
	UAbilitySystemComponent* FirstAbilitySystem = First ? First->GetAbilitySystemComponent() : nullptr;
	UAbilitySystemComponent* SecondAbilitySystem = Second ? Second->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("첫 번째 플레이어의 ASC가 있다."), FirstAbilitySystem) &&
		TestNotNull(TEXT("두 번째 플레이어의 ASC가 있다."), SecondAbilitySystem))
	{
		TestEqual(TEXT("최대 체력 기본값은 100이다."),
			FirstAbilitySystem->GetNumericAttribute(UPDHealthAttributeSet::GetMaxHealthAttribute()),
			100.0f);
		TestEqual(TEXT("처음에는 체력이 가득 차 있다."), GetHealth(*FirstAbilitySystem), 100.0f);

		ApplyDamage(*FirstAbilitySystem, *SecondAbilitySystem, 30.0f);
		TestEqual(TEXT("아군에게도 피해가 들어간다."), GetHealth(*SecondAbilitySystem), 70.0f);
		TestEqual(TEXT("피해를 준 쪽 체력은 그대로다."), GetHealth(*FirstAbilitySystem), 100.0f);

		ApplyDamage(*FirstAbilitySystem, *FirstAbilitySystem, 5.0f);
		TestEqual(TEXT("자기 자신에게도 피해가 들어간다."), GetHealth(*FirstAbilitySystem), 95.0f);

		ApplyDamage(*FirstAbilitySystem, *SecondAbilitySystem, 500.0f);
		TestEqual(TEXT("체력은 0 아래로 내려가지 않는다."), GetHealth(*SecondAbilitySystem), 0.0f);
		TestTrue(TEXT("체력이 0이 되면 사망한다(빈사 시간 0)."), Second->IsDead());
		TestTrue(TEXT("피해를 준 쪽은 살아 있다."), First->IsAlive());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthDeathTest,
	"PADO.Health.LifeState.Death",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthDeathTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* Victim = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	APDPlayerCharacter* Attacker = Rig.SpawnPlayer(FVector(300.0f, 0.0f, 100.0f));
	UPDItemDefinition* Definition =
		PDItemTestUtils::MakeHeldItemDefinition(GetTransientPackage(), 5);
	APDWorldItemActor* Weapon = PDItemTestUtils::SpawnHeldItem(*Rig.World, *Definition);
	APDWorldItemActor* GroundItem = PDItemTestUtils::SpawnHeldItem(*Rig.World, *Definition);
	UAbilitySystemComponent* VictimAbilitySystem = Victim ? Victim->GetAbilitySystemComponent() : nullptr;
	UAbilitySystemComponent* AttackerAbilitySystem = Attacker ? Attacker->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("맞는 쪽 ASC가 있다."), VictimAbilitySystem) &&
		TestNotNull(TEXT("때리는 쪽 ASC가 있다."), AttackerAbilitySystem) &&
		TestNotNull(TEXT("무기를 준비한다."), Weapon) &&
		TestNotNull(TEXT("바닥 아이템을 준비한다."), GroundItem) &&
		TestTrue(TEXT("손 소켓을 구성한다."), PDCharacterTestUtils::ConfigureHolderSocket(Victim)) &&
		TestTrue(TEXT("무기를 든다."), Victim->GetHeldItemComponent()->TryPickUp(Weapon)))
	{
		ApplyDamage(*AttackerAbilitySystem, *VictimAbilitySystem, 100.0f);
		TestEqual(TEXT("체력이 0이 되면 빈사 없이 사망한다."),
			static_cast<int32>(Victim->GetHealthComponent()->GetLifeState()),
			static_cast<int32>(EPDLifeState::Dead));
		TestTrue(TEXT("사망 태그가 붙는다."),
			VictimAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Dead));
		TestTrue(TEXT("죽으면 손을 쓸 수 없다."),
			VictimAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
		TestFalse(TEXT("빈사를 거치지 않았다."),
			VictimAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Downed));

		TestNull(TEXT("들고 있던 아이템을 떨어뜨린다."), Victim->GetHeldItemComponent()->GetHeldItem());
		TestTrue(TEXT("떨어진 아이템은 바닥 상태다."),
			Weapon->GetItemState() != EPDWorldItemState::Held);
		TestEqual(TEXT("죽은 몸은 걷지 않는다."),
			static_cast<int32>(Victim->GetCharacterMovement()->MovementMode.GetValue()),
			static_cast<int32>(MOVE_None));
		TestEqual(TEXT("죽은 몸의 캡슐은 충돌하지 않는다."),
			static_cast<int32>(Victim->GetCapsuleComponent()->GetCollisionEnabled()),
			static_cast<int32>(ECollisionEnabled::NoCollision));

		ApplyDamage(*AttackerAbilitySystem, *VictimAbilitySystem, 10.0f);
		TestEqual(TEXT("죽은 몸은 더 다치지 않는다."), GetHealth(*VictimAbilitySystem), 0.0f);
		TestTrue(TEXT("죽은 몸은 계속 사망 상태다."), Victim->IsDead());

		TestFalse(TEXT("죽은 몸은 아이템을 줍지 않는다."),
			Victim->GetInteractionComponent()->InteractWithTarget(GroundItem, nullptr));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthDownedTest,
	"PADO.Health.LifeState.Downed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthDownedTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* Attacker = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	APDPlayerCharacter* FinishedOff = Rig.SpawnPlayer(FVector(300.0f, 0.0f, 100.0f));
	APDPlayerCharacter* BledOut = Rig.SpawnPlayer(FVector(600.0f, 0.0f, 100.0f));
	APDPlayerCharacter* Revived = Rig.SpawnPlayer(FVector(900.0f, 0.0f, 100.0f));
	UAbilitySystemComponent* AttackerAbilitySystem =
		Attacker ? Attacker->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("때리는 쪽 ASC가 있다."), AttackerAbilitySystem) &&
		TestNotNull(TEXT("확인 사살할 대상을 준비한다."), FinishedOff) &&
		TestNotNull(TEXT("시간이 지나 죽을 대상을 준비한다."), BledOut) &&
		TestNotNull(TEXT("살릴 대상을 준비한다."), Revived))
	{
		for (APDPlayerCharacter* Target : { FinishedOff, BledOut, Revived })
		{
			SetDownedDuration(*Target->GetHealthComponent(), 5.0f);
			ApplyDamage(*AttackerAbilitySystem, *Target->GetAbilitySystemComponent(), 100.0f);
		}

		UAbilitySystemComponent* DownedAbilitySystem = FinishedOff->GetAbilitySystemComponent();
		TestEqual(TEXT("빈사 시간이 있으면 체력 0에서 빈사가 된다."),
			static_cast<int32>(FinishedOff->GetHealthComponent()->GetLifeState()),
			static_cast<int32>(EPDLifeState::Downed));
		TestTrue(TEXT("빈사 태그가 붙는다."),
			DownedAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Downed));
		TestTrue(TEXT("빈사 중에는 손을 쓸 수 없다."),
			DownedAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
		TestFalse(TEXT("빈사는 살아 있는 상태가 아니다."), FinishedOff->IsAlive());
		TestFalse(TEXT("빈사는 사망이 아니다."), FinishedOff->IsDead());

		ApplyDamage(*AttackerAbilitySystem, *DownedAbilitySystem, 1.0f);
		TestTrue(TEXT("빈사 중에 다시 맞으면 사망한다."), FinishedOff->IsDead());
		TestFalse(TEXT("사망하면 빈사 태그를 뗀다."),
			DownedAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Downed));

		TestTrue(TEXT("빈사인 몸은 살릴 수 있다."), Revived->GetHealthComponent()->Revive());
		TestTrue(TEXT("살리면 살아 있는 상태로 돌아온다."), Revived->IsAlive());
		TestEqual(TEXT("살아날 때 최대 체력의 30%를 채운다."),
			GetHealth(*Revived->GetAbilitySystemComponent()),
			30.0f);
		TestFalse(TEXT("살아나면 손을 다시 쓸 수 있다."),
			Revived->GetAbilitySystemComponent()->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
		TestFalse(TEXT("살아 있는 몸은 다시 살리지 않는다."), Revived->GetHealthComponent()->Revive());

		AdvanceTimers(*Rig.World, 6.0f);
		TestTrue(TEXT("빈사 시간이 지나면 사망한다."), BledOut->IsDead());
		TestTrue(TEXT("살아난 몸은 빈사 시간이 지나도 죽지 않는다."), Revived->IsAlive());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthKillTest,
	"PADO.Health.LifeState.Kill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthKillTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* Healthy = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	APDPlayerCharacter* Downed = Rig.SpawnPlayer(FVector(300.0f, 0.0f, 100.0f));
	if (TestNotNull(TEXT("살아 있는 대상을 준비한다."), Healthy) &&
		TestNotNull(TEXT("빈사가 될 대상을 준비한다."), Downed))
	{
		// 빈사를 켜 두어도 바로 사망한다. 타고 있던 탈것이 파괴될 때가 이 경우다.
		SetDownedDuration(*Healthy->GetHealthComponent(), 5.0f);
		TestTrue(TEXT("살아 있는 몸을 바로 사망시킨다."),
			Healthy->GetHealthComponent()->Kill(nullptr, nullptr));
		TestTrue(TEXT("빈사를 거치지 않는다."), Healthy->IsDead());
		TestEqual(TEXT("체력도 0이 된다."), GetHealth(*Healthy->GetAbilitySystemComponent()), 0.0f);
		TestFalse(TEXT("이미 죽은 몸은 다시 사망시키지 않는다."),
			Healthy->GetHealthComponent()->Kill(nullptr, nullptr));

		SetDownedDuration(*Downed->GetHealthComponent(), 5.0f);
		ApplyDamage(*Downed->GetAbilitySystemComponent(), *Downed->GetAbilitySystemComponent(), 100.0f);
		TestTrue(TEXT("빈사인 몸도 사망시킨다."), Downed->GetHealthComponent()->Kill(nullptr, nullptr));
		TestTrue(TEXT("빈사에서 사망으로 넘어간다."), Downed->IsDead());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthTagsClearedWhenBodyRemovedTest,
	"PADO.Health.LifeState.TagsClearedWhenBodyRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthTagsClearedWhenBodyRemovedTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* Victim = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	UAbilitySystemComponent* AbilitySystem = Victim ? Victim->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("플레이어 ASC가 있다."), AbilitySystem) &&
		TestTrue(TEXT("손 소켓을 구성한다."), PDCharacterTestUtils::ConfigureHolderSocket(Victim)))
	{
		// 몸의 EndPlay는 BeginPlay를 거친 액터에서만 돈다.
		Victim->DispatchBeginPlay();
		ApplyDamage(*AbilitySystem, *AbilitySystem, 100.0f);
		TestTrue(TEXT("사망 태그가 붙는다."), AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Dead));

		// ASC는 PlayerState에 있어 몸이 사라져도 남는다. 다음 몸이 죽은 상태로 시작하면 안 된다.
		Victim->Destroy();
		TestFalse(TEXT("몸이 사라지면 사망 태그를 뗀다."),
			AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Dead));
		TestFalse(TEXT("몸이 사라지면 손 사용 불가 태그도 뗀다."),
			AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthResetForRespawnTest,
	"PADO.Health.Respawn.ResetForRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthResetForRespawnTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerCharacter* Player = Rig.SpawnPlayer(FVector(0.0f, 0.0f, 100.0f));
	APDPlayerState* PlayerState = Player ? Player->GetPlayerState<APDPlayerState>() : nullptr;
	UAbilitySystemComponent* AbilitySystem = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("PlayerState가 있다."), PlayerState) &&
		TestNotNull(TEXT("ASC가 있다."), AbilitySystem))
	{
		ApplyDamage(*AbilitySystem, *AbilitySystem, 60.0f);

		// 남이 걸어 준 무한 슬로우다.
		const FGameplayEffectSpecHandle Slow = AbilitySystem->MakeOutgoingSpec(
			UPDGE_MoveSpeedMultiplier::StaticClass(),
			1.0f,
			AbilitySystem->MakeEffectContext());
		Slow.Data->SetSetByCallerMagnitude(TAG_PD_Data_MoveSpeed_Multiplier, 0.5f);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*Slow.Data.Get());
		TestEqual(TEXT("슬로우가 걸린다."),
			AbilitySystem->GetNumericAttribute(UPDMovementAttributeSet::GetMoveSpeedAttribute()),
			250.0f);

		PlayerState->ResetForRespawn();
		TestEqual(TEXT("리셋하면 체력이 가득 찬다."), GetHealth(*AbilitySystem), 100.0f);
		TestEqual(TEXT("리셋하면 걸려 있던 효과가 모두 사라진다."),
			AbilitySystem->GetActiveEffects(FGameplayEffectQuery()).Num(),
			0);
		TestEqual(TEXT("슬로우가 풀려 이동 속도가 돌아온다."),
			AbilitySystem->GetNumericAttribute(UPDMovementAttributeSet::GetMoveSpeedAttribute()),
			500.0f);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDHealthRespawnScheduledTest,
	"PADO.Health.Respawn.ScheduledOnDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDHealthRespawnScheduledTest::RunTest(const FString& Parameters)
{
	using namespace PDHealthSystemTests;
	FHealthRig Rig;
	if (!TestTrue(TEXT("테스트 World를 만든다."), Rig.SetUp()))
	{
		return false;
	}

	APDPlayerController* Controller = nullptr;
	APDPlayerCharacter* Player = Rig.SpawnPossessedPlayer(FVector(0.0f, 0.0f, 100.0f), Controller);
	UPDRespawnComponent* Respawn = Controller ? Controller->GetRespawnComponent() : nullptr;
	UAbilitySystemComponent* AbilitySystem = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("컨트롤러가 빙의한 플레이어를 준비한다."), Player) &&
		TestNotNull(TEXT("컨트롤러에 리스폰 컴포넌트가 있다."), Respawn) &&
		TestNotNull(TEXT("ASC가 있다."), AbilitySystem))
	{
		TestFalse(TEXT("살아 있으면 리스폰을 기다리지 않는다."), Respawn->IsRespawnPending());

		ApplyDamage(*AbilitySystem, *AbilitySystem, 100.0f);
		TestTrue(TEXT("빙의한 몸이 죽는다."), Player->IsDead());
		TestTrue(TEXT("죽으면 리스폰을 기다린다."), Respawn->IsRespawnPending());

		// 테스트 World에는 GameMode가 없어 새 몸을 만들 수 없다. 시간이 되면 아무것도 바꾸지 않고 끝난다.
		AdvanceTimers(*Rig.World, 6.0f);
		TestFalse(TEXT("리스폰 시간이 지나면 더 기다리지 않는다."), Respawn->IsRespawnPending());
		TestTrue(TEXT("GameMode가 없으면 시체를 지우지 않는다."), IsValid(Player));
		TestTrue(TEXT("GameMode가 없으면 빙의도 그대로다."), Controller->GetPawn() == Player);
	}

	Rig.TearDown();
	return true;
}

#endif
