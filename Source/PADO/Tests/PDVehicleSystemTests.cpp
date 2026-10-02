#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PADO/AbilitySystem/Ability/PDGA_Action.h"
#include "PADO/AbilitySystem/Ability/PDGA_ChannelAction.h"
#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Effect/PDGE_Damage.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PADO/AbilitySystem/Fragment/PDApplyGameplayEffectFragment.h"
#include "PADO/AbilitySystem/Fragment/PDThrowProjectileFragment.h"
#include "PADO/AbilitySystem/Projectile/PDActionProjectile.h"
#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"
#include "PADO/AbilitySystem/Struct/PDProjectileConfigStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDAimLineTraceTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"
#include "PADO/AbilitySystem/Targeting/PDTargetingCollision.h"
#include "PADO/Character/PDCharacterMovementComponent.h"
#include "PADO/Character/PDHealthComponent.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Character/PDPlayerController.h"
#include "PADO/Character/PDPlayerState.h"
#include "PADO/Interaction/Component/PDInteractionComponent.h"
#include "PADO/Interaction/Interface/PDInteractable.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Item/Tag/PDItemGameplayTags.h"
#include "PADO/Item/Trait/PDItemMagazineTrait.h"
#include "PADO/Tests/PDCharacterTestUtils.h"
#include "PADO/Tests/PDItemTestUtils.h"
#include "PADO/Tests/PDTestWorldUtils.h"
#include "PADO/Vehicle/Component/PDVehicleHealthComponent.h"
#include "PADO/Vehicle/Component/PDVehicleImpactComponent.h"
#include "PADO/Vehicle/Component/PDVehicleOccupancyComponent.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"
#include "PADO/Vehicle/Component/PDWheeledVehicleMovementComponent.h"
#include "PADO/Vehicle/PDVehicleContactSubsystem.h"
#include "PADO/Vehicle/PDWheeledVehicle.h"
#include "Physics/NetworkPhysicsComponent.h"

namespace PDVehicleSystemTests
{
	using namespace PDTestWorldUtils;

	/** 좌석 하나를 붙인다. 실제로는 차량 Blueprint 뷰포트에서 배치한다. */
	UPDVehicleSeatComponent* AddSeat(
		APDWheeledVehicle& Vehicle,
		int32 SeatNumber,
		EPDVehicleSeatRole SeatRole,
		const FVector& RelativeLocation)
	{
		UPDVehicleSeatComponent* Seat = NewObject<UPDVehicleSeatComponent>(&Vehicle);
		Seat->ConfigureSeat(SeatNumber, SeatRole);
		Seat->SetupAttachment(Vehicle.GetRootComponent());
		Seat->SetRelativeLocation(RelativeLocation);
		Seat->RegisterComponent();
		return Seat;
	}

	/**
	 * 운전석(왼쪽 앞)과 동승석 둘(오른쪽 앞, 왼쪽 뒤)을 가진 차량이다. 차량의 +X가
	 * 앞, +Y가 오른쪽이다. 상호작용 대상 호출은 인터페이스 이벤트라서 액터
	 * 초기화를 마친 World를 쓴다.
	 */
	struct FVehicleRig
	{
		UWorld* World = nullptr;
		APDWheeledVehicle* Vehicle = nullptr;
		UPDVehicleOccupancyComponent* Occupancy = nullptr;
		UPDVehicleSeatComponent* DriverSeat = nullptr;
		UPDVehicleSeatComponent* FrontPassengerSeat = nullptr;
		UPDVehicleSeatComponent* RearPassengerSeat = nullptr;

		bool SetUp()
		{
			FWorldContext* WorldContext = nullptr;
			World = CreateInitializedTestWorld(WorldContext);
			Vehicle = World ? World->SpawnActor<APDWheeledVehicle>() : nullptr;
			if (!Vehicle)
			{
				return false;
			}

			DriverSeat = AddSeat(
				*Vehicle, 1, EPDVehicleSeatRole::Driver, FVector(50.0f, -60.0f, 0.0f));
			FrontPassengerSeat = AddSeat(
				*Vehicle, 2, EPDVehicleSeatRole::Passenger, FVector(50.0f, 60.0f, 0.0f));
			RearPassengerSeat = AddSeat(
				*Vehicle, 3, EPDVehicleSeatRole::Passenger, FVector(-80.0f, -60.0f, 0.0f));

			// 실제 스폰처럼 BeginPlay를 거친다. 운전자가 없는 동안 서버가 입력을
			// 만드는 상태가 여기서 시작된다.
			Vehicle->DispatchBeginPlay();
			Occupancy = Vehicle->GetOccupancyComponent();
			return Occupancy != nullptr;
		}

		void TearDown()
		{
			DestroyTestWorld(World);
			World = nullptr;
		}

		APDPlayerCharacter* SpawnRider(const FVector& Location) const
		{
			return PDCharacterTestUtils::SpawnPlayerCharacter(World, Location);
		}

		/**
		 * 컨트롤러가 빙의한 플레이어 캐릭터다. 조종 권한은 이 컨트롤러로 넘어간다.
		 * 시점 전환을 보려면 APDPlayerController를 넘긴다. bLocalController가
		 * false면 서버에서 본 원격 플레이어처럼 로컬이 아닌 컨트롤러가 된다.
		 */
		template <typename TController>
		APDPlayerCharacter* SpawnPossessedRider(
			const FVector& Location,
			TController*& OutController,
			bool bLocalController = true) const
		{
			APDPlayerCharacter* Rider =
				World->SpawnActor<APDPlayerCharacter>(Location, FRotator::ZeroRotator);
			APDPlayerState* PlayerState = PDCharacterTestUtils::SpawnPlayerState(World);
			OutController = World->SpawnActor<TController>();
			if (!Rider || !PlayerState || !OutController)
			{
				return nullptr;
			}

			// 넷 드라이버가 없는 World에서는 ULocalPlayer가 있어야 로컬로 본다.
			// 테스트 World에는 없으므로 실제 게임에서 GameMode가 하듯 직접 표시한다.
			if (bLocalController)
			{
				OutController->SetAsLocalPlayerController();
			}
			OutController->PlayerState = PlayerState;
			PlayerState->SetOwner(OutController);
			OutController->Possess(Rider);
			return Rider;
		}
	};

	// 손 아이템 준비는 피해 테스트와 공유한다.
	using PDItemTestUtils::MakeHeldItemDefinition;
	using PDItemTestUtils::SpawnHeldItem;

	/** 지금 활성인 Action 수다. 무기를 든 동안 활성인 Fire Action은 세지 않는다. */
	int32 CountActiveActions(const UAbilitySystemComponent& AbilitySystem)
	{
		int32 Count = 0;
		for (const FGameplayAbilitySpec& Spec : AbilitySystem.GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability &&
				!Spec.Ability->IsA<UPDGA_FireAction>() &&
				Spec.Ability->GetAssetTags().HasTag(TAG_PD_Ability_Action))
			{
				++Count;
			}
		}
		return Count;
	}

	float GetHealth(const UAbilitySystemComponent& AbilitySystem)
	{
		return AbilitySystem.GetNumericAttribute(UPDHealthAttributeSet::GetHealthAttribute());
	}

	/**
	 * 차체 충돌체다. 테스트 차량에는 메시 애셋이 없어 충돌이 없으므로 실제 차체처럼
	 * Vehicle 오브젝트 타입으로 모든 채널을 막는 상자를 붙인다.
	 */
	UBoxComponent* AddBodyCollision(APDWheeledVehicle& Vehicle)
	{
		UBoxComponent* Body = NewObject<UBoxComponent>(&Vehicle);
		Body->SetBoxExtent(FVector(250.0f, 120.0f, 100.0f));
		Body->SetCollisionObjectType(ECC_Vehicle);
		Body->SetCollisionResponseToAllChannels(ECR_Block);
		Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Body->SetupAttachment(Vehicle.GetRootComponent());
		Body->RegisterComponent();
		return Body;
	}

	/** 컨트롤러 없이 스폰한 몸에는 이동 모드가 없어 넉백이 걸리지 않는다. 빙의한 몸처럼 걷게 한다. */
	void StartWalking(APDPlayerCharacter& Character)
	{
		Character.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	/** 카메라가 지금 그 대상을 보거나 그 대상으로 넘어가는 중이다. */
	bool IsCameraHeadingTo(const APlayerController& Controller, const AActor* Target)
	{
		const APlayerCameraManager* CameraManager = Controller.PlayerCameraManager;
		if (!CameraManager)
		{
			return false;
		}

		return CameraManager->PendingViewTarget.Target
			? CameraManager->PendingViewTarget.Target == Target
			: CameraManager->GetViewTarget() == Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatSelectionTest,
	"PADO.Vehicle.Occupancy.SeatSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatSelectionTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("좌석 셋인 차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* First = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* Second = Rig.SpawnRider(FVector(0.0f, 400.0f, 0.0f));
	APDPlayerCharacter* Third = Rig.SpawnRider(FVector(0.0f, 500.0f, 0.0f));
	APDPlayerCharacter* Fourth = Rig.SpawnRider(FVector(0.0f, 600.0f, 0.0f));
	if (TestNotNull(TEXT("첫 번째 탑승자를 스폰한다."), First) &&
		TestNotNull(TEXT("두 번째 탑승자를 스폰한다."), Second) &&
		TestNotNull(TEXT("세 번째 탑승자를 스폰한다."), Third) &&
		TestNotNull(TEXT("네 번째 탑승자를 스폰한다."), Fourth))
	{
		UPDVehicleOccupancyComponent* Occupancy = Rig.Occupancy;

		TestTrue(TEXT("희망 좌석이 없으면 가장 가까운 좌석에 앉는다."),
			Occupancy->TryEnter(First, nullptr) &&
				Occupancy->GetSeatOccupant(Rig.FrontPassengerSeat) == First);
		TestFalse(TEXT("이미 탄 사람은 다시 타지 않는다."),
			Occupancy->TryEnter(First, Rig.DriverSeat));

		TestTrue(TEXT("희망 좌석이 차 있으면 번호 순서대로 다음 빈 좌석에 앉는다."),
			Occupancy->TryEnter(Second, Rig.FrontPassengerSeat) &&
				Occupancy->GetSeatOccupant(Rig.RearPassengerSeat) == Second);
		TestTrue(TEXT("끝 번호까지 차 있으면 처음 좌석부터 본다."),
			Occupancy->TryEnter(Third, Rig.RearPassengerSeat) &&
				Occupancy->GetSeatOccupant(Rig.DriverSeat) == Third);

		TestFalse(TEXT("만석이면 빈 좌석이 없다."), Occupancy->HasFreeSeat());
		TestFalse(TEXT("빈 좌석이 없으면 탑승을 거부한다."),
			Occupancy->TryEnter(Fourth, nullptr));
		TestFalse(TEXT("거부된 사람은 타지 않은 상태로 남는다."),
			Fourth->GetVehicleOccupantComponent()->IsSeated());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatedBodyTest,
	"PADO.Vehicle.Occupancy.SeatedBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatedBodyTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(0.0f, 300.0f, 0.0f));
	if (TestNotNull(TEXT("탑승자를 스폰한다."), Rider))
	{
		UPDVehicleOccupantComponent* Occupant = Rider->GetVehicleOccupantComponent();
		UCharacterMovementComponent* Movement = Rider->GetCharacterMovement();
		const bool bOriginalUseControllerYaw = Rider->bUseControllerRotationYaw;

		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("탄 탈것을 안다."), Occupant->GetCurrentVehicle() == Rig.Vehicle);
		TestTrue(TEXT("몸이 좌석에 붙는다."),
			Rider->GetRootComponent()->GetAttachParent() == Rig.FrontPassengerSeat);
		TestFalse(TEXT("탑승 중에는 몸의 충돌이 꺼진다."), Rider->GetActorEnableCollision());
		TestEqual(TEXT("탑승 중에는 걷지 않는다."),
			static_cast<int32>(Movement->MovementMode.GetValue()),
			static_cast<int32>(MOVE_None));
		TestFalse(TEXT("좌석 안에서 몸이 시점을 따라 돌지 않는다."),
			Rider->bUseControllerRotationYaw);

		TestTrue(TEXT("내린다."), Rig.Occupancy->TryExit(Rider));
		TestFalse(TEXT("내리면 타지 않은 상태다."), Occupant->IsSeated());
		TestNull(TEXT("좌석이 빈다."),
			Rig.Occupancy->GetSeatOccupant(Rig.FrontPassengerSeat));
		TestNull(TEXT("몸이 좌석에서 떨어진다."),
			Rider->GetRootComponent()->GetAttachParent());
		TestTrue(TEXT("앉았던 좌석의 하차 지점에 내린다."),
			Rider->GetActorLocation().Equals(
				Rig.FrontPassengerSeat->GetExitLocation(), 1.0f));
		TestTrue(TEXT("몸의 충돌이 돌아온다."), Rider->GetActorEnableCollision());
		TestNotEqual(TEXT("다시 움직일 수 있다."),
			static_cast<int32>(Movement->MovementMode.GetValue()),
			static_cast<int32>(MOVE_None));
		TestEqual(TEXT("시점 추종 설정이 돌아온다."),
			Rider->bUseControllerRotationYaw, bOriginalUseControllerYaw);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleDriverControlTest,
	"PADO.Vehicle.Occupancy.DriverControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleDriverControlTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Driver =
		Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	if (TestNotNull(TEXT("컨트롤러가 빙의한 운전자를 준비한다."), Driver))
	{
		UNetworkPhysicsComponent* NetworkPhysics =
			Rig.Vehicle->FindComponentByClass<UNetworkPhysicsComponent>();

		// 운전자가 없는 동안은 서버가 정지 입력을 만든다.
		if (NetworkPhysics)
		{
			TestTrue(TEXT("운전자가 없으면 서버가 물리 입력을 만든다."),
				NetworkPhysics->GetIsRelayingLocalInputs());
		}
		else
		{
			AddInfo(TEXT("Physics Prediction이 꺼져 있어 네트워크 물리 입력 전환은 건너뛴다."));
		}

		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Driver, Rig.DriverSeat));
		TestTrue(TEXT("운전석에 앉으면 조종 권한이 넘어간다."),
			Rig.Vehicle->GetVehicleController() == Controller);
		TestTrue(TEXT("차량 Owner가 운전자의 컨트롤러다."),
			Rig.Vehicle->GetOwner() == Controller);
		TestTrue(TEXT("운전자는 차량에 빙의하지 않는다."),
			Controller->GetPawn() == Driver && Driver->GetController() == Controller);
		if (NetworkPhysics)
		{
			TestFalse(TEXT("운전자가 있으면 서버가 물리 입력을 만들지 않는다."),
				NetworkPhysics->GetIsRelayingLocalInputs());
		}

		TestTrue(TEXT("운전석에서 내린다."), Rig.Occupancy->TryExit(Driver));
		TestNull(TEXT("내리면 조종 권한을 거둔다."), Rig.Vehicle->GetVehicleController());
		TestNull(TEXT("내리면 차량 Owner가 빈다."), Rig.Vehicle->GetOwner());
		if (NetworkPhysics)
		{
			TestTrue(TEXT("운전자가 내리면 서버가 다시 물리 입력을 만든다."),
				NetworkPhysics->GetIsRelayingLocalInputs());
		}

		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Driver, Rig.FrontPassengerSeat));
		TestNull(TEXT("동승석은 조종 권한을 받지 않는다."),
			Rig.Vehicle->GetVehicleController());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleInteractionTest,
	"PADO.Vehicle.Interaction.EnterAndExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleInteractionTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	// 뒷좌석 문이다. 좌석 아래에 붙은 컴포넌트를 조준하면 그 좌석이 희망 좌석이다.
	UBoxComponent* RearDoor = NewObject<UBoxComponent>(Rig.Vehicle);
	RearDoor->SetupAttachment(Rig.RearPassengerSeat);
	RearDoor->RegisterComponent();

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 200.0f, 0.0f));
	APDPlayerCharacter* Second = Rig.SpawnRider(FVector(50.0f, 400.0f, 0.0f));
	if (TestNotNull(TEXT("탑승자를 스폰한다."), Rider) &&
		TestNotNull(TEXT("두 번째 탑승자를 스폰한다."), Second))
	{
		UPDVehicleOccupantComponent* Occupant = Rider->GetVehicleOccupantComponent();
		UPDInteractionComponent* Interaction = Rider->GetInteractionComponent();

		TestTrue(TEXT("상호작용으로 탄다."),
			Interaction->InteractWithTarget(Rig.Vehicle, nullptr) && Occupant->IsSeated());
		TestTrue(TEXT("조준한 좌석이 없으면 가장 가까운 좌석에 앉는다."),
			Occupant->GetCurrentSeat() == Rig.FrontPassengerSeat);
		TestFalse(TEXT("탄 채로는 다시 탈 수 없다."),
			Interaction->InteractWithTarget(Rig.Vehicle, nullptr));

		TestTrue(TEXT("좌석 아래의 문을 조준하면 그 좌석에 앉는다."),
			Second->GetInteractionComponent()->InteractWithTarget(Rig.Vehicle, RearDoor) &&
				Second->GetVehicleOccupantComponent()->GetCurrentSeat() ==
					Rig.RearPassengerSeat);

		Rider->Interact();
		TestFalse(TEXT("탑승 중 상호작용 입력은 하차다."), Occupant->IsSeated());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleOccupantDestroyedTest,
	"PADO.Vehicle.Occupancy.OccupantDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleOccupantDestroyedTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Driver =
		Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	if (TestNotNull(TEXT("운전자를 준비한다."), Driver))
	{
		// 몸의 EndPlay는 BeginPlay를 거친 액터에서만 돈다.
		Driver->DispatchBeginPlay();
		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Driver, Rig.DriverSeat));

		// 접속 종료처럼 몸이 탄 채로 사라진다.
		Driver->Destroy();
		TestNull(TEXT("몸이 사라지면 좌석이 빈다."),
			Rig.Occupancy->GetSeatOccupant(Rig.DriverSeat));
		TestNull(TEXT("운전자가 사라지면 조종 권한을 거둔다."),
			Rig.Vehicle->GetVehicleController());
		TestTrue(TEXT("빈 좌석이 생긴다."), Rig.Occupancy->HasFreeSeat());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatSwitchesViewTest,
	"PADO.Vehicle.Camera.SeatSwitchesView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatSwitchesViewTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerController* Controller = nullptr;
	APDPlayerCharacter* Rider =
		Rig.SpawnPossessedRider(FVector(50.0f, 300.0f, 0.0f), Controller);
	if (TestNotNull(TEXT("컨트롤러가 빙의한 탑승자를 준비한다."), Rider))
	{
		// 테스트 컨트롤러에는 좌석 매핑 에셋이 없다. 좌석 역할마다 맞는 매핑을
		// 찾는지는 누락 경고가 어느 쪽으로 한 번씩 나오는지로 본다.
		AddExpectedMessagePlain(
			TEXT("VehiclePassengerMappingContext"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			1);
		AddExpectedMessagePlain(
			TEXT("VehicleDriverMappingContext"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			1);

		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("동승석에 앉아도 차량 카메라를 본다."),
			Controller->GetViewedVehicle() == Rig.Vehicle &&
				IsCameraHeadingTo(*Controller, Rig.Vehicle));
		TestNull(TEXT("동승석은 조종하지 않는다."), Controller->GetControlledVehicle());

		TestTrue(TEXT("동승석에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestNull(TEXT("내리면 차량을 보지 않는다."), Controller->GetViewedVehicle());
		TestTrue(TEXT("내리면 캐릭터 카메라로 돌아온다."),
			IsCameraHeadingTo(*Controller, Rider));

		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.DriverSeat));
		TestTrue(TEXT("운전석도 같은 차량 카메라를 본다."),
			Controller->GetViewedVehicle() == Rig.Vehicle &&
				IsCameraHeadingTo(*Controller, Rig.Vehicle));
		TestTrue(TEXT("운전석은 조종한다."),
			Controller->GetControlledVehicle() == Rig.Vehicle);

		TestTrue(TEXT("운전석에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestNull(TEXT("내리면 조종하지 않는다."), Controller->GetControlledVehicle());
		TestNull(TEXT("내리면 차량을 보지 않는다."), Controller->GetViewedVehicle());
		TestTrue(TEXT("운전석에서 내려도 캐릭터 카메라로 돌아온다."),
			IsCameraHeadingTo(*Controller, Rider));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleCameraFollowsViewerTest,
	"PADO.Vehicle.Camera.FollowsLocalViewer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleCameraFollowsViewerTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerController* Viewer = nullptr;
	APDPlayerController* Bystander = nullptr;
	APDPlayerCharacter* Passenger =
		Rig.SpawnPossessedRider(FVector(50.0f, 300.0f, 0.0f), Viewer);
	APDPlayerCharacter* Walker =
		Rig.SpawnPossessedRider(FVector(50.0f, 600.0f, 0.0f), Bystander);
	USpringArmComponent* CameraBoom = Rig.Vehicle->GetCameraBoom();
	if (TestNotNull(TEXT("동승자를 준비한다."), Passenger) &&
		TestNotNull(TEXT("타지 않은 플레이어를 준비한다."), Walker) &&
		TestNotNull(TEXT("차량 카메라 붐이 있다."), CameraBoom))
	{
		const FRotator ViewerRotation(-15.0f, 120.0f, 0.0f);
		Viewer->SetControlRotation(ViewerRotation);
		Bystander->SetControlRotation(FRotator(0.0f, -60.0f, 0.0f));

		// 테스트 컨트롤러에는 좌석 매핑 에셋이 없다. 카메라와는 무관한 경고다.
		AddExpectedMessagePlain(
			TEXT("VehiclePassengerMappingContext"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			1);
		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Passenger, Rig.FrontPassengerSeat));
		Rig.Vehicle->Tick(0.0f);
		TestTrue(TEXT("운전자가 없어도 이 차를 보는 동승자의 시점을 따라 돈다."),
			CameraBoom->GetComponentRotation().Equals(ViewerRotation, 0.01f));

		TestTrue(TEXT("내린다."), Rig.Occupancy->TryExit(Passenger));
		Viewer->SetControlRotation(FRotator(0.0f, 30.0f, 0.0f));
		Rig.Vehicle->Tick(0.0f);
		TestTrue(TEXT("보는 사람이 없으면 아무도 카메라를 돌리지 않는다."),
			CameraBoom->GetComponentRotation().Equals(ViewerRotation, 0.01f));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleDriverNetRoleTest,
	"PADO.Vehicle.Occupancy.DriverNetRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleDriverNetRoleTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* RemoteController = nullptr;
	APDPlayerCharacter* RemoteDriver = Rig.SpawnPossessedRider(
		FVector(50.0f, -300.0f, 0.0f), RemoteController, false);
	APlayerController* LocalController = nullptr;
	APDPlayerCharacter* LocalDriver =
		Rig.SpawnPossessedRider(FVector(50.0f, -600.0f, 0.0f), LocalController);
	if (TestNotNull(TEXT("원격 운전자를 준비한다."), RemoteDriver) &&
		TestNotNull(TEXT("로컬 운전자를 준비한다."), LocalDriver))
	{
		TestEqual(TEXT("운전자가 없으면 모든 클라이언트에서 시뮬레이트 프록시다."),
			static_cast<int32>(Rig.Vehicle->GetRemoteRole()),
			static_cast<int32>(ROLE_SimulatedProxy));

		TestTrue(TEXT("원격 운전자가 운전석에 탄다."),
			Rig.Occupancy->TryEnter(RemoteDriver, Rig.DriverSeat));
		TestEqual(TEXT("운전자 머신에서는 자율 프록시가 된다."),
			static_cast<int32>(Rig.Vehicle->GetRemoteRole()),
			static_cast<int32>(ROLE_AutonomousProxy));
		TestTrue(TEXT("운전자도 차량에 빙의하지 않는다."),
			RemoteController->GetPawn() == RemoteDriver);

		TestTrue(TEXT("원격 운전자가 내린다."), Rig.Occupancy->TryExit(RemoteDriver));
		TestEqual(TEXT("내리면 시뮬레이트 프록시로 돌아온다."),
			static_cast<int32>(Rig.Vehicle->GetRemoteRole()),
			static_cast<int32>(ROLE_SimulatedProxy));

		TestTrue(TEXT("로컬 운전자가 탄다."),
			Rig.Occupancy->TryEnter(LocalDriver, Rig.DriverSeat));
		TestEqual(TEXT("리슨 서버 호스트처럼 이 머신의 운전자면 역할을 바꾸지 않는다."),
			static_cast<int32>(Rig.Vehicle->GetRemoteRole()),
			static_cast<int32>(ROLE_SimulatedProxy));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleNoAIPossessionTest,
	"PADO.Vehicle.Occupancy.NoAIPossession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleNoAIPossessionTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	TestEqual(TEXT("맵에 놓이거나 스폰돼도 AI가 자동으로 빙의하지 않는다."),
		static_cast<int32>(Rig.Vehicle->AutoPossessAI),
		static_cast<int32>(EAutoPossessAI::Disabled));

	// 맵에 놓인 Pawn에 엔진이 부르는 것과 같은 경로다.
	Rig.Vehicle->SpawnDefaultController();
	TestNull(TEXT("기본 컨트롤러를 만들라고 해도 빙의되지 않는다."),
		Rig.Vehicle->GetController());

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleNotPushedByCharacterTest,
	"PADO.Vehicle.Movement.NotPushedByCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleNotPushedByCharacterTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	AStaticMeshActor* Crate = Rig.World->SpawnActor<AStaticMeshActor>();
	if (TestNotNull(TEXT("물리 소품을 스폰한다."), Crate))
	{
		TestFalse(TEXT("걷다가 부딪혀도 탈것은 밀지 않는다."),
			UPDCharacterMovementComponent::CanPushImpactedActor(Rig.Vehicle));
		TestTrue(TEXT("탈것이 아닌 물리 대상은 엔진 규칙대로 민다."),
			UPDCharacterMovementComponent::CanPushImpactedActor(Crate));
		TestTrue(TEXT("액터가 없는 충돌은 엔진 규칙에 맡긴다."),
			UPDCharacterMovementComponent::CanPushImpactedActor(nullptr));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleTargetingIgnoresOwnVehicleTest,
	"PADO.Vehicle.Targeting.IgnoresOwnVehicle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleTargetingIgnoresOwnVehicleTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	if (TestNotNull(TEXT("탑승자를 스폰한다."), Rider))
	{
		const uint32 VehicleId = Rig.Vehicle->GetUniqueID();

		FCollisionQueryParams WalkingParams;
		PDTargetingCollision::AddIgnoredSourceActors(WalkingParams, *Rider, nullptr);
		TestTrue(TEXT("주체 자신은 언제나 무시한다."),
			WalkingParams.GetIgnoredSourceObjects().Contains(Rider->GetUniqueID()));
		TestFalse(TEXT("타지 않았으면 차량은 판정 대상으로 남는다."),
			WalkingParams.GetIgnoredSourceObjects().Contains(VehicleId));

		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		FCollisionQueryParams SeatedParams;
		PDTargetingCollision::AddIgnoredSourceActors(SeatedParams, *Rider, nullptr);
		TestTrue(TEXT("앉아 있으면 자기 차량을 무시한다."),
			SeatedParams.GetIgnoredSourceObjects().Contains(VehicleId));

		APDPlayerCharacter* Bystander = Rig.SpawnRider(FVector(50.0f, 600.0f, 0.0f));
		if (TestNotNull(TEXT("타지 않은 사람을 스폰한다."), Bystander))
		{
			FCollisionQueryParams BystanderParams;
			PDTargetingCollision::AddIgnoredSourceActors(
				BystanderParams, *Bystander, nullptr);
			TestFalse(TEXT("다른 사람에게 그 차량은 여전히 판정 대상이다."),
				BystanderParams.GetIgnoredSourceObjects().Contains(VehicleId));
		}
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleProjectileIgnoresOwnVehicleTest,
	"PADO.Vehicle.Targeting.ProjectileIgnoresOwnVehicle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleProjectileIgnoresOwnVehicleTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	UStaticMesh* SphereMesh =
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (TestNotNull(TEXT("탑승자를 스폰한다."), Rider) &&
		TestNotNull(TEXT("투사체 메시를 불러온다."), SphereMesh) &&
		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat)))
	{
		// 투사체 충돌 프로필 정의는 이 테스트 범위 밖이다.
		AddExpectedMessagePlain(
			TEXT("COLLISION PROFILE [PDProjectile] is not found"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			-1);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Rider;
		SpawnParameters.Instigator = Rider;
		APDActionProjectile* Projectile = Rig.World->SpawnActor<APDActionProjectile>(
			Rider->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);

		FPDProjectileLaunchConfigStruct LaunchConfig;
		LaunchConfig.ProjectileClass = APDActionProjectile::StaticClass();
		LaunchConfig.ProjectileMesh = SphereMesh;
		const FPDProjectileExplosionConfigStruct ExplosionConfig;
		const TArray<TObjectPtr<UPDActionFragment>> NoFragments;

		if (TestNotNull(TEXT("투사체를 스폰한다."), Projectile) &&
			TestTrue(TEXT("앉은 채로 던진다."),
				Projectile->InitializeProjectile(
					LaunchConfig,
					ExplosionConfig,
					NoFragments,
					NoFragments,
					Rider->GetPDAbilitySystemComponent(),
					nullptr,
					Rider,
					Rider,
					FVector(1000.0f, 0.0f, 0.0f))))
		{
			const TArray<AActor*>& MoveIgnoreActors =
				Projectile->GetCollisionComponent()->GetMoveIgnoreActors();
			TestTrue(TEXT("던진 사람은 이동 중에 무시한다."),
				MoveIgnoreActors.Contains(Rider));
			TestTrue(TEXT("던진 사람이 앉아 있던 차량도 이동 중에 무시한다."),
				MoveIgnoreActors.Contains(Rig.Vehicle));

			// 투사체의 EndPlay는 BeginPlay를 거친 액터에서만 돈다.
			Projectile->DispatchBeginPlay();
			Projectile->Destroy();
			TestFalse(TEXT("투사체가 사라지면 차량 무시도 함께 풀린다."),
				Projectile->GetCollisionComponent()->GetMoveIgnoreActors().Contains(
					Rig.Vehicle));
		}
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSwitchSeatTest,
	"PADO.Vehicle.Occupancy.SwitchSeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSwitchSeatTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("좌석 셋인 차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Rider =
		Rig.SpawnPossessedRider(FVector(50.0f, 300.0f, 0.0f), Controller);
	APDPlayerCharacter* Rear = Rig.SpawnRider(FVector(-80.0f, -300.0f, 0.0f));
	APDPlayerCharacter* Walker = Rig.SpawnRider(FVector(0.0f, 600.0f, 0.0f));
	if (TestNotNull(TEXT("컨트롤러가 빙의한 탑승자를 준비한다."), Rider) &&
		TestNotNull(TEXT("뒷좌석 탑승자를 준비한다."), Rear) &&
		TestNotNull(TEXT("타지 않은 사람을 준비한다."), Walker))
	{
		UPDVehicleOccupancyComponent* Occupancy = Rig.Occupancy;
		UPDVehicleOccupantComponent* Occupant = Rider->GetVehicleOccupantComponent();
		UAbilitySystemComponent* AbilitySystem = Rider->GetAbilitySystemComponent();

		TestTrue(TEXT("동승석에 탄다."), Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("뒷좌석에 탄다."), Occupancy->TryEnter(Rear, Rig.RearPassengerSeat));

		TestFalse(TEXT("차 있는 좌석으로는 옮기지 않는다."), Occupant->RequestSwitchSeat(3));
		TestFalse(TEXT("없는 좌석 번호는 거부한다."), Occupant->RequestSwitchSeat(9));
		TestFalse(TEXT("앉은 좌석으로는 옮기지 않는다."), Occupant->RequestSwitchSeat(2));
		TestTrue(TEXT("거부되면 그 자리에 남는다."),
			Occupant->GetCurrentSeat() == Rig.FrontPassengerSeat);

		TestTrue(TEXT("번호로 빈 운전석에 옮긴다."), Occupant->RequestSwitchSeat(1));
		TestTrue(TEXT("운전석에 앉는다."),
			Occupant->GetCurrentSeat() == Rig.DriverSeat &&
				Occupancy->GetSeatOccupant(Rig.DriverSeat) == Rider);
		TestNull(TEXT("떠난 좌석은 빈다."), Occupancy->GetSeatOccupant(Rig.FrontPassengerSeat));
		TestTrue(TEXT("몸이 새 좌석에 붙는다."),
			Rider->GetRootComponent()->GetAttachParent() == Rig.DriverSeat);
		TestTrue(TEXT("운전석으로 옮기면 조종 권한을 받는다."),
			Rig.Vehicle->GetVehicleController() == Controller);

		TestTrue(TEXT("다음 빈 좌석으로 옮긴다."), Occupant->RequestSwitchToNextSeat());
		TestTrue(TEXT("좌석 순서대로 다음 빈 좌석에 앉는다."),
			Occupant->GetCurrentSeat() == Rig.FrontPassengerSeat);
		TestNull(TEXT("운전석을 떠나면 조종 권한을 거둔다."),
			Rig.Vehicle->GetVehicleController());

		TestTrue(TEXT("다음 좌석이 차 있으면 건너뛰고 처음 좌석부터 본다."),
			Occupant->RequestSwitchToNextSeat() &&
				Occupant->GetCurrentSeat() == Rig.DriverSeat);

		TestEqual(TEXT("좌석을 옮겨도 손 사용 불가 태그는 하나다."),
			AbilitySystem ? AbilitySystem->GetTagCount(TAG_PD_State_HandsBlocked) : -1,
			1);

		TestTrue(TEXT("남은 빈 좌석에 탄다."),
			Occupancy->TryEnter(Walker, Rig.FrontPassengerSeat));
		TestFalse(TEXT("빈 좌석이 없으면 다음 좌석으로 옮기지 않는다."),
			Occupant->RequestSwitchToNextSeat());

		TestTrue(TEXT("뒷좌석 탑승자가 내린다."), Occupancy->TryExit(Rear));
		TestFalse(TEXT("타지 않은 사람은 좌석을 옮기지 않는다."),
			Rear->GetVehicleOccupantComponent()->RequestSwitchSeat(3));
		TestFalse(TEXT("타지 않은 사람은 다음 좌석으로도 옮기지 않는다."),
			Rear->GetVehicleOccupantComponent()->RequestSwitchToNextSeat());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatSwitchKeepsViewTest,
	"PADO.Vehicle.Camera.SeatSwitchKeepsView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatSwitchKeepsViewTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerController* Controller = nullptr;
	APDPlayerCharacter* Rider =
		Rig.SpawnPossessedRider(FVector(50.0f, 300.0f, 0.0f), Controller);
	if (TestNotNull(TEXT("컨트롤러가 빙의한 탑승자를 준비한다."), Rider))
	{
		// 테스트 컨트롤러에는 좌석 매핑 에셋이 없다. 처음 앉을 때 한 번만 알린다.
		AddExpectedMessagePlain(
			TEXT("VehiclePassengerMappingContext"),
			ELogVerbosity::Warning,
			EAutomationExpectedMessageFlags::Contains,
			1);

		UPDVehicleOccupantComponent* Occupant = Rider->GetVehicleOccupantComponent();
		TestTrue(TEXT("동승석에 탄다."),
			Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("운전석으로 옮긴다."), Occupant->RequestSwitchSeat(1));
		TestTrue(TEXT("같은 차 안에서 옮기면 차량 카메라를 그대로 본다."),
			Controller->GetViewedVehicle() == Rig.Vehicle &&
				IsCameraHeadingTo(*Controller, Rig.Vehicle));
		TestTrue(TEXT("운전석으로 옮기면 그 차를 조종한다."),
			Controller->GetControlledVehicle() == Rig.Vehicle);

		TestTrue(TEXT("동승석으로 돌아간다."), Occupant->RequestSwitchSeat(2));
		TestNull(TEXT("동승석으로 옮기면 조종하지 않는다."),
			Controller->GetControlledVehicle());
		TestTrue(TEXT("카메라는 계속 차량이다."),
			Controller->GetViewedVehicle() == Rig.Vehicle);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatedHandsBlockedTest,
	"PADO.Vehicle.Hands.BlockedWhileSeated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatedHandsBlockedTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* EmptyHanded = Rig.SpawnRider(FVector(-80.0f, -300.0f, 0.0f));
	UPDItemDefinition* Definition =
		MakeHeldItemDefinition(GetTransientPackage(), 5);
	APDWorldItemActor* Weapon = SpawnHeldItem(*Rig.World, *Definition);
	APDWorldItemActor* GroundItem = SpawnHeldItem(*Rig.World, *Definition);
	if (TestNotNull(TEXT("무기를 든 탑승자를 준비한다."), Rider) &&
		TestNotNull(TEXT("빈손 탑승자를 준비한다."), EmptyHanded) &&
		TestNotNull(TEXT("무기를 준비한다."), Weapon) &&
		TestNotNull(TEXT("바닥 아이템을 준비한다."), GroundItem) &&
		TestTrue(TEXT("손 소켓을 구성한다."),
			PDCharacterTestUtils::ConfigureHolderSocket(Rider) &&
				PDCharacterTestUtils::ConfigureHolderSocket(EmptyHanded)))
	{
		UPDHeldItemComponent* HeldItems = Rider->GetHeldItemComponent();
		UPDWeaponMagazineComponent* Magazine = Weapon->GetMagazineComponent();
		UAbilitySystemComponent* AbilitySystem = Rider->GetAbilitySystemComponent();
		TestTrue(TEXT("무기를 든다."), HeldItems->TryPickUp(Weapon));
		TestTrue(TEXT("한 발을 쓴다."), Magazine && Magazine->TryConsumeRound());

		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("빈손으로 뒷좌석에 탄다."),
			Rig.Occupancy->TryEnter(EmptyHanded, Rig.RearPassengerSeat));
		TestTrue(TEXT("앉으면 손 사용 불가 상태다."),
			AbilitySystem && AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));

		// 탑승 직전에 보낸 요청이 앉은 뒤에 도착한 경우다.
		TestFalse(TEXT("앉은 뒤 도착한 재장전 요청은 거부한다."),
			HeldItems->TryReloadHeldItem());
		TestEqual(TEXT("거부된 재장전은 탄창을 채우지 않는다."),
			Magazine ? Magazine->GetCurrentMagazineAmmo() : -1, 4);
		HeldItems->TryDropHeldItem();
		TestTrue(TEXT("앉은 뒤 도착한 드롭 요청은 무시한다."), HeldItems->GetHeldItem() == Weapon);
		TestFalse(TEXT("앉아 있으면 바닥 아이템을 줍지 않는다."),
			EmptyHanded->GetHeldItemComponent()->TryPickUp(GroundItem));

		if (AbilitySystem)
		{
			TestFalse(TEXT("앉아 있으면 Single Action을 시작하지 않는다."),
				GetDefault<UPDGA_Action>()->DoesAbilitySatisfyTagRequirements(*AbilitySystem));
			TestFalse(TEXT("앉아 있으면 Channel Action을 시작하지 않는다."),
				GetDefault<UPDGA_ChannelAction>()->DoesAbilitySatisfyTagRequirements(*AbilitySystem));
			const UGameplayAbility* FireActionDefault = GetDefault<UPDGA_FireAction>();
			TestTrue(TEXT("무기를 든 동안 활성인 Fire Action은 태그 조건을 보지 않는다."),
				FireActionDefault->DoesAbilitySatisfyTagRequirements(*AbilitySystem));
		}

		TestTrue(TEXT("내린다."), Rig.Occupancy->TryExit(Rider));
		TestTrue(TEXT("빈손 탑승자도 내린다."), Rig.Occupancy->TryExit(EmptyHanded));
		TestFalse(TEXT("내리면 손 사용 불가 상태가 풀린다."),
			AbilitySystem && AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
		if (AbilitySystem)
		{
			TestTrue(TEXT("내리면 Single Action을 다시 시작할 수 있다."),
				GetDefault<UPDGA_Action>()->DoesAbilitySatisfyTagRequirements(*AbilitySystem));
		}
		TestTrue(TEXT("내리면 재장전한다."), HeldItems->TryReloadHeldItem());
		TestEqual(TEXT("재장전으로 탄창이 찬다."),
			Magazine ? Magazine->GetCurrentMagazineAmmo() : -1, 5);
		TestTrue(TEXT("내리면 바닥 아이템을 줍는다."),
			EmptyHanded->GetHeldItemComponent()->TryPickUp(GroundItem));
		HeldItems->TryDropHeldItem();
		TestNull(TEXT("내리면 드롭한다."), HeldItems->GetHeldItem());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleHandsUnblockedOnDestroyTest,
	"PADO.Vehicle.Hands.UnblockedWhenBodyDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleHandsUnblockedOnDestroyTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	if (TestNotNull(TEXT("탑승자를 준비한다."), Rider) &&
		TestTrue(TEXT("손 소켓을 구성한다."), PDCharacterTestUtils::ConfigureHolderSocket(Rider)))
	{
		// 플레이어의 ASC는 PlayerState에 있어 몸이 사라져도 남는다.
		UAbilitySystemComponent* AbilitySystem = Rider->GetAbilitySystemComponent();
		Rider->DispatchBeginPlay();
		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestTrue(TEXT("앉으면 손 사용 불가 상태다."),
			AbilitySystem && AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));

		Rider->Destroy();
		TestFalse(TEXT("탄 채로 몸이 사라져도 다음 몸이 손을 쓸 수 있게 태그를 뗀다."),
			AbilitySystem && AbilitySystem->HasMatchingGameplayTag(TAG_PD_State_HandsBlocked));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleBoardingCancelsActionTest,
	"PADO.Vehicle.Hands.BoardingCancelsAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleBoardingCancelsActionTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	UStaticMesh* SphereMesh =
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	if (!TestNotNull(TEXT("투사체 메시를 불러온다."), SphereMesh) ||
		!TestNotNull(TEXT("탑승자를 준비한다."), Rider) ||
		!TestTrue(TEXT("손 소켓을 구성한다."), PDCharacterTestUtils::ConfigureHolderSocket(Rider)))
	{
		Rig.TearDown();
		return false;
	}

	// 누르는 동안 충전하고 놓는 순간 던지는 Action이다. 탑승할 때 방아쇠만 놓으면
	// 놓는 순간 실행돼 차 안에서 던지게 된다.
	UPDItemDefinition* GrenadeDefinition = MakeHeldItemDefinition(GetTransientPackage(), 0);
	UPDSingleActionDefinition* Throw = NewObject<UPDSingleActionDefinition>(GrenadeDefinition);
	Throw->ActionTargeting = NewObject<UPDSelfTargeting>(Throw);
	UPDThrowProjectileFragment* ThrowFragment = NewObject<UPDThrowProjectileFragment>(Throw);
	ThrowFragment->LaunchConfig.ProjectileClass = APDActionProjectile::StaticClass();
	ThrowFragment->LaunchConfig.ProjectileMesh = SphereMesh;
	ThrowFragment->LaunchConfig.bEnableCharge = true;
	FPDActionHookStruct ThrowHook;
	ThrowHook.HookTag = TAG_PD_ActionHook_OnExecuteStart;
	ThrowHook.Fragments.Add(ThrowFragment);
	Throw->ActionHooks.Add(MoveTemp(ThrowHook));
	GrenadeDefinition->UseAction = Throw;

	APDWorldItemActor* Grenade = SpawnHeldItem(*Rig.World, *GrenadeDefinition);
	UAbilitySystemComponent* AbilitySystem = Rider->GetAbilitySystemComponent();
	UPDHeldItemComponent* HeldItems = Rider->GetHeldItemComponent();
	if (TestNotNull(TEXT("충전 투척 아이템을 준비한다."), Grenade) &&
		TestNotNull(TEXT("탑승자의 ASC가 있다."), AbilitySystem) &&
		TestTrue(TEXT("투척 아이템을 든다."), HeldItems->TryPickUp(Grenade)) &&
		TestTrue(TEXT("누르면 충전을 시작한다."), HeldItems->PressHeldItemUse()))
	{
		TestEqual(TEXT("놓기 전까지 투척 Action이 활성이다."),
			CountActiveActions(*AbilitySystem), 1);

		TestTrue(TEXT("충전 중에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		TestEqual(TEXT("타면 진행 중이던 Action을 취소한다."),
			CountActiveActions(*AbilitySystem), 0);

		// 탑승 뒤에 키를 떼도 이미 취소된 Action은 실행되지 않는다.
		HeldItems->ReleaseHeldItemUse();
		int32 ProjectileCount = 0;
		for (TActorIterator<APDActionProjectile> It(Rig.World); It; ++It)
		{
			++ProjectileCount;
		}
		TestEqual(TEXT("탑승 순간 충전하던 것을 던지지 않는다."), ProjectileCount, 0);
		TestTrue(TEXT("든 아이템은 그대로다."), HeldItems->GetHeldItem() == Grenade);
		TestFalse(TEXT("앉아 있으면 새로 눌러도 시작하지 않는다."),
			HeldItems->PressHeldItemUse());
		TestEqual(TEXT("앉아 있는 동안 활성 Action이 없다."),
			CountActiveActions(*AbilitySystem), 0);
		HeldItems->ReleaseHeldItemUse();
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleExitInheritsVelocityTest,
	"PADO.Vehicle.Occupancy.ExitInheritsVelocity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleExitInheritsVelocityTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	UPawnMovementComponent* VehicleMovement = Rig.Vehicle->GetMovementComponent();
	if (TestNotNull(TEXT("탑승자를 준비한다."), Rider) &&
		TestNotNull(TEXT("차량 무브먼트가 있다."), VehicleMovement))
	{
		UCharacterMovementComponent* Movement = Rider->GetCharacterMovement();

		// 테스트 World의 차체는 물리 시뮬레이션을 하지 않아 무브먼트의 속도가 차량 속도다.
		const FVector DrivingVelocity(1500.0f, 200.0f, 0.0f);
		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		VehicleMovement->Velocity = DrivingVelocity;
		TestTrue(TEXT("달리는 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestTrue(TEXT("차의 속도를 이어받는다."),
			Movement->Velocity.Equals(DrivingVelocity, 0.1f));
		TestEqual(TEXT("공중에서 시작해 착지는 무브먼트가 정한다."),
			static_cast<int32>(Movement->MovementMode.GetValue()),
			static_cast<int32>(MOVE_Falling));

		TestTrue(TEXT("다시 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		VehicleMovement->Velocity = FVector(5.0f, 0.0f, 0.0f);
		TestTrue(TEXT("거의 멈춘 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		// 멈춘 차에서 내릴 때 걷기·낙하는 엔진이 발밑 바닥으로 정한다(SetDefaultMovementMode).
		TestTrue(TEXT("미세한 떨림은 속도로 넘기지 않는다."), Movement->Velocity.IsNearlyZero());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleDriverlessNeutralInputTest,
	"PADO.Vehicle.Movement.DriverlessNeutralInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleDriverlessNeutralInputTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	UPDWheeledVehicleMovementComponent* Movement =
		Cast<UPDWheeledVehicleMovementComponent>(Rig.Vehicle->GetVehicleMovementComponent());
	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Rider =
		Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	if (TestNotNull(TEXT("차량 무브먼트가 운전자 없는 입력을 다루는 클래스다."), Movement) &&
		TestNotNull(TEXT("컨트롤러가 빙의한 탑승자를 준비한다."), Rider))
	{
		UPDVehicleOccupantComponent* Occupant = Rider->GetVehicleOccupantComponent();

		TestTrue(TEXT("운전자가 없으면 중립 입력이다."), Movement->IsDriverless());
		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.DriverSeat));
		TestFalse(TEXT("운전자가 있으면 운전자의 입력을 쓴다."), Movement->IsDriverless());

		TestTrue(TEXT("운전석에서 동승석으로 옮긴다."), Occupant->RequestSwitchSeat(2));
		TestTrue(TEXT("운전석을 비우면 중립 입력이다."), Movement->IsDriverless());

		TestTrue(TEXT("운전석으로 돌아온다."), Occupant->RequestSwitchSeat(1));
		TestFalse(TEXT("돌아오면 다시 운전자의 입력을 쓴다."), Movement->IsDriverless());

		TestTrue(TEXT("운전석에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestTrue(TEXT("내리면 중립 입력이다."), Movement->IsDriverless());
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleOneWayContactSetupTest,
	"PADO.Vehicle.Movement.OneWayContactSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleOneWayContactSetupTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	UPDWheeledVehicleMovementComponent* Movement =
		Cast<UPDWheeledVehicleMovementComponent>(Rig.Vehicle->GetVehicleMovementComponent());
	UPDVehicleContactSubsystem* VehicleContacts =
		UWorld::GetSubsystem<UPDVehicleContactSubsystem>(Rig.World);
	if (TestNotNull(TEXT("차량 무브먼트가 있다."), Movement) &&
		TestNotNull(TEXT("게임 월드에는 탈것 접촉 서브시스템이 있다."), VehicleContacts))
	{
		const FCollisionResponseContainer& WheelResponses = Movement->WheelTraceCollisionResponses;
		TestEqual(TEXT("바퀴는 물리로 움직이는 물체(래그돌, 떨어진 아이템)를 밟지 않는다."),
			static_cast<int32>(WheelResponses.GetResponse(ECC_PhysicsBody)),
			static_cast<int32>(ECR_Ignore));
		TestEqual(TEXT("바퀴는 사람(캡슐, 메시)을 밟지 않는다."),
			static_cast<int32>(WheelResponses.GetResponse(ECC_Pawn)),
			static_cast<int32>(ECR_Ignore));
		TestEqual(TEXT("바퀴는 다른 차도 밟지 않는다(엔진 기본값)."),
			static_cast<int32>(WheelResponses.GetResponse(ECC_Vehicle)),
			static_cast<int32>(ECR_Ignore));
		TestEqual(TEXT("바퀴는 지형과 구조물(WorldStatic)을 밟는다."),
			static_cast<int32>(WheelResponses.GetResponse(ECC_WorldStatic)),
			static_cast<int32>(ECR_Block));
		TestEqual(TEXT("바퀴는 움직이는 구조물(WorldDynamic)을 밟는다."),
			static_cast<int32>(WheelResponses.GetResponse(ECC_WorldDynamic)),
			static_cast<int32>(ECR_Block));

		// 물리 바디가 있는 몸을 밀리기만 하는 몸으로 등록했다가 지운다. 물리 스레드 목록은
		// 바디가 사라질 때 엔진 알림으로 비워진다. 월드를 정리할 때 콜백도 해제된다.
		UStaticMesh* SphereMesh =
			LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		AStaticMeshActor* Prop = Rig.World->SpawnActor<AStaticMeshActor>(FVector(0.0f, 600.0f, 200.0f), FRotator::ZeroRotator);
		if (TestNotNull(TEXT("구 메시를 불러온다."), SphereMesh) &&
			TestNotNull(TEXT("물리 소품을 스폰한다."), Prop))
		{
			UStaticMeshComponent* PropMesh = Prop->GetStaticMeshComponent();
			PropMesh->SetMobility(EComponentMobility::Movable);
			PropMesh->SetStaticMesh(SphereMesh);
			PropMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
			PropMesh->SetSimulatePhysics(true);
			VehicleContacts->RegisterPassiveBody(*PropMesh);
			VehicleContacts->RegisterVehicle(*Rig.Vehicle->GetMesh());
			Prop->Destroy();
		}
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleOccupantDiesTest,
	"PADO.Vehicle.Occupancy.DiesWhileSeated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleOccupantDiesTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Driver =
		Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	UAbilitySystemComponent* AbilitySystem = Driver ? Driver->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("운전자를 준비한다."), Driver) &&
		TestNotNull(TEXT("운전자의 ASC가 있다."), AbilitySystem) &&
		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Driver, Rig.DriverSeat)))
	{
		const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(
			UPDGE_Damage::StaticClass(),
			1.0f,
			AbilitySystem->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(TAG_PD_Data_Damage, 100.0f);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

		TestTrue(TEXT("탄 채로 죽는다."), Driver->IsDead());
		TestFalse(TEXT("죽은 몸은 좌석에 남지 않는다."),
			Driver->GetVehicleOccupantComponent()->IsSeated());
		TestNull(TEXT("좌석이 빈다."), Rig.Occupancy->GetSeatOccupant(Rig.DriverSeat));
		TestNull(TEXT("운전자가 죽으면 조종 권한을 거둔다."), Rig.Vehicle->GetVehicleController());
		TestNull(TEXT("몸이 좌석에서 떨어진다."), Driver->GetRootComponent()->GetAttachParent());
		TestEqual(TEXT("내린 죽은 몸은 다시 걷지 않는다."),
			static_cast<int32>(Driver->GetCharacterMovement()->MovementMode.GetValue()),
			static_cast<int32>(MOVE_None));
		TestEqual(TEXT("손 사용 불가 태그는 탑승이 뗀 뒤 사망이 붙인 하나만 남는다."),
			AbilitySystem->GetTagCount(TAG_PD_State_HandsBlocked),
			1);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleHealthAndDestructionTest,
	"PADO.Vehicle.Damage.HealthAndDestruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleHealthAndDestructionTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Driver = Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	APDPlayerCharacter* Passenger = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* Shooter = Rig.SpawnRider(FVector(-800.0f, 0.0f, 0.0f));
	APDPlayerCharacter* Walker = Rig.SpawnRider(FVector(0.0f, 600.0f, 0.0f));
	UAbilitySystemComponent* VehicleAbilitySystem = Rig.Vehicle->GetAbilitySystemComponent();
	UAbilitySystemComponent* ShooterAbilitySystem = Shooter ? Shooter->GetAbilitySystemComponent() : nullptr;
	UPDVehicleHealthComponent* VehicleHealth = Rig.Vehicle->GetHealthComponent();
	if (TestNotNull(TEXT("운전자를 준비한다."), Driver) &&
		TestNotNull(TEXT("동승자를 준비한다."), Passenger) &&
		TestNotNull(TEXT("타지 않은 사람을 준비한다."), Walker) &&
		TestNotNull(TEXT("차량에 ASC가 있다."), VehicleAbilitySystem) &&
		TestNotNull(TEXT("쏘는 사람의 ASC가 있다."), ShooterAbilitySystem) &&
		TestNotNull(TEXT("차량에 체력 컴포넌트가 있다."), VehicleHealth))
	{
		TestTrue(TEXT("피해 Fragment가 찾는 방식으로 차량의 ASC를 찾는다."),
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Rig.Vehicle) ==
				VehicleAbilitySystem);
		TestEqual(TEXT("차량의 최대 체력은 차량 설정(기본 1000)이다."),
			VehicleAbilitySystem->GetNumericAttribute(UPDHealthAttributeSet::GetMaxHealthAttribute()),
			1000.0f);
		TestEqual(TEXT("처음에는 체력이 가득 차 있다."), GetHealth(*VehicleAbilitySystem), 1000.0f);

		TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Driver, Rig.DriverSeat));
		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Passenger, Rig.FrontPassengerSeat));

		UPDGE_Damage::ApplyDamage(*ShooterAbilitySystem, *VehicleAbilitySystem, 300.0f, Shooter);
		TestEqual(TEXT("차량이 피해를 받는다."), GetHealth(*VehicleAbilitySystem), 700.0f);
		TestFalse(TEXT("체력이 남으면 파괴되지 않는다."), VehicleHealth->IsDestroyed());
		TestTrue(TEXT("차가 맞아도 탄 사람은 그대로다."),
			Driver->IsAlive() && Passenger->IsAlive() &&
				Driver->GetVehicleOccupantComponent()->IsSeated());

		UPDGE_Damage::ApplyDamage(*ShooterAbilitySystem, *VehicleAbilitySystem, 700.0f, Shooter);
		TestTrue(TEXT("체력이 0이 되면 파괴된다."), VehicleHealth->IsDestroyed());
		TestTrue(TEXT("파괴된 차량은 죽은 상태다."),
			VehicleAbilitySystem->HasMatchingGameplayTag(TAG_PD_State_Dead));

		TestTrue(TEXT("파괴되면 운전자가 죽는다."), Driver->IsDead());
		TestTrue(TEXT("파괴되면 동승자도 죽는다."), Passenger->IsDead());
		TestEqual(TEXT("죽은 탑승자의 체력은 0이다."),
			GetHealth(*Passenger->GetAbilitySystemComponent()),
			0.0f);
		TestFalse(TEXT("죽은 운전자는 내린다."), Driver->GetVehicleOccupantComponent()->IsSeated());
		TestFalse(TEXT("죽은 동승자도 내린다."), Passenger->GetVehicleOccupantComponent()->IsSeated());
		TestNull(TEXT("운전자가 사라져 조종 권한을 거둔다."), Rig.Vehicle->GetVehicleController());

		UPDGE_Damage::ApplyDamage(*ShooterAbilitySystem, *VehicleAbilitySystem, 100.0f, Shooter);
		TestEqual(TEXT("파괴된 뒤에는 체력이 그대로다."), GetHealth(*VehicleAbilitySystem), 0.0f);

		FPDInteractionContextStruct Context;
		Context.Instigator = Walker;
		TestFalse(TEXT("파괴된 차량에는 탈 수 없다."),
			IPDInteractable::Execute_CanInteract(Rig.Vehicle, Context));
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleSeatedNotTargetedTest,
	"PADO.Vehicle.Damage.SeatedNotTargeted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleSeatedNotTargetedTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	// 컨트롤러가 없는 사수는 눈높이에서 정면(+X)으로 쏜다. 앉은 동승자가 그 선 위에 있다.
	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* Shooter = Rig.SpawnRider(FVector(-1000.0f, 60.0f, 0.0f));
	UPDAimLineTraceTargeting* Targeting = NewObject<UPDAimLineTraceTargeting>(GetTransientPackage());
	Targeting->TraceDistance = 3000.0f;
	Targeting->TargetObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_Vehicle)};
	if (TestNotNull(TEXT("동승자를 준비한다."), Rider) &&
		TestNotNull(TEXT("사수를 준비한다."), Shooter) &&
		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat)))
	{
		FPDActionTargetingContext Context;
		Context.SourceActor = Shooter;

		FPDActionTargetingResult OpenVehicle;
		Targeting->GatherTargets(Context, OpenVehicle);
		TestEqual(TEXT("차체 충돌이 없는 곳이어도 앉은 사람은 맞지 않는다."),
			OpenVehicle.Targets.Num(), 0);

		UBoxComponent* Body = AddBodyCollision(*Rig.Vehicle);
		FPDActionTargetingResult ClosedVehicle;
		Targeting->GatherTargets(Context, ClosedVehicle);
		TestEqual(TEXT("차체를 맞힌다."), ClosedVehicle.Targets.Num(), 1);
		TestTrue(TEXT("맞은 대상은 차량이다."),
			ClosedVehicle.Targets.Num() == 1 && ClosedVehicle.Targets[0].Actor == Rig.Vehicle);
		TestTrue(TEXT("탄은 차체에서 멈춘다."),
			ClosedVehicle.ShotResult.GetComponent() == Body);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleExplosionSparesOccupantsTest,
	"PADO.Vehicle.Damage.ExplosionSparesOccupants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleExplosionSparesOccupantsTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* Walker = Rig.SpawnRider(FVector(0.0f, 250.0f, 0.0f));
	APDPlayerCharacter* Thrower = Rig.SpawnRider(FVector(-2000.0f, 0.0f, 0.0f));
	UStaticMesh* SphereMesh =
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("동승자를 준비한다."), Rider) ||
		!TestNotNull(TEXT("옆에 선 사람을 준비한다."), Walker) ||
		!TestNotNull(TEXT("던지는 사람을 준비한다."), Thrower) ||
		!TestNotNull(TEXT("투사체 메시를 불러온다."), SphereMesh) ||
		!TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat)))
	{
		Rig.TearDown();
		return false;
	}

	// 투사체 충돌 프로필 정의는 이 테스트 범위 밖이다.
	AddExpectedMessagePlain(
		TEXT("COLLISION PROFILE [PDProjectile] is not found"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		-1);

	// 반경 안의 대상마다 피해 40을 주는 폭발이다.
	UPDApplyGameplayEffectFragment* DamageFragment =
		NewObject<UPDApplyGameplayEffectFragment>(GetTransientPackage());
	DamageFragment->EffectRecipe.EffectClass = UPDGE_Damage::StaticClass();
	FPDSetByCallerValueStruct& DamageValue = DamageFragment->EffectRecipe.SetByCallers.AddDefaulted_GetRef();
	DamageValue.DataTag = TAG_PD_Data_Damage;
	DamageValue.Magnitude = 40.0f;
	const TArray<TObjectPtr<UPDActionFragment>> TargetFragments = {DamageFragment};
	const TArray<TObjectPtr<UPDActionFragment>> NoFragments;

	FPDProjectileLaunchConfigStruct LaunchConfig;
	LaunchConfig.ProjectileClass = APDActionProjectile::StaticClass();
	LaunchConfig.ProjectileMesh = SphereMesh;
	FPDProjectileExplosionConfigStruct ExplosionConfig;
	ExplosionConfig.ExplosionRadius = 600.0f;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Thrower;
	SpawnParameters.Instigator = Thrower;
	APDActionProjectile* Projectile = Rig.World->SpawnActor<APDActionProjectile>(
		FVector(0.0f, 150.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	UAbilitySystemComponent* VehicleAbilitySystem = Rig.Vehicle->GetAbilitySystemComponent();
	if (TestNotNull(TEXT("투사체를 스폰한다."), Projectile) &&
		TestTrue(TEXT("투사체를 준비한다."),
			Projectile->InitializeProjectile(
				LaunchConfig,
				ExplosionConfig,
				TargetFragments,
				NoFragments,
				Thrower->GetPDAbilitySystemComponent(),
				nullptr,
				Thrower,
				Thrower,
				FVector::ZeroVector)) &&
		TestTrue(TEXT("차 옆에서 터진다."), Projectile->Detonate()))
	{
		TestEqual(TEXT("앉은 사람은 폭발을 맞지 않는다."),
			GetHealth(*Rider->GetAbilitySystemComponent()), 100.0f);
		TestEqual(TEXT("옆에 선 사람은 맞는다."),
			GetHealth(*Walker->GetAbilitySystemComponent()), 60.0f);
		TestEqual(TEXT("차량은 맞는다."), GetHealth(*VehicleAbilitySystem), 960.0f);
	}

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleRammingTest,
	"PADO.Vehicle.Damage.Ramming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleRammingTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APlayerController* Controller = nullptr;
	APDPlayerCharacter* Driver = Rig.SpawnPossessedRider(FVector(50.0f, -300.0f, 0.0f), Controller);
	APDPlayerCharacter* Passenger = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	APDPlayerCharacter* Victim = Rig.SpawnRider(FVector(500.0f, 0.0f, 0.0f));
	APDPlayerCharacter* Bystander = Rig.SpawnRider(FVector(500.0f, 400.0f, 0.0f));
	APDPlayerCharacter* Runner = Rig.SpawnRider(FVector(500.0f, -400.0f, 0.0f));
	UPDVehicleImpactComponent* Impact = Rig.Vehicle->GetImpactComponent();
	UPawnMovementComponent* VehicleMovement = Rig.Vehicle->GetMovementComponent();
	if (!TestNotNull(TEXT("운전자를 준비한다."), Driver) ||
		!TestNotNull(TEXT("동승자를 준비한다."), Passenger) ||
		!TestNotNull(TEXT("치일 사람을 준비한다."), Victim) ||
		!TestNotNull(TEXT("밀려날 사람을 준비한다."), Bystander) ||
		!TestNotNull(TEXT("크게 치일 사람을 준비한다."), Runner) ||
		!TestNotNull(TEXT("차량에 들이받기 컴포넌트가 있다."), Impact) ||
		!TestNotNull(TEXT("차량 무브먼트가 있다."), VehicleMovement))
	{
		Rig.TearDown();
		return false;
	}

	StartWalking(*Victim);
	StartWalking(*Bystander);
	StartWalking(*Runner);
	UAbilitySystemComponent* VictimAbilitySystem = Victim->GetAbilitySystemComponent();
	UCharacterMovementComponent* VictimMovement = Victim->GetCharacterMovement();

	TestEqual(TEXT("캐릭터 캡슐은 차를 물리로 막지 않고 접촉만 알린다."),
		static_cast<int32>(Victim->GetCapsuleComponent()->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::QueryAndProbe));

	TestTrue(TEXT("운전석에 탄다."), Rig.Occupancy->TryEnter(Driver, Rig.DriverSeat));
	TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Passenger, Rig.FrontPassengerSeat));

	// 차가 앞(+X)으로 달리고 사람은 차 앞에 있다. 테스트 차체는 물리 시뮬레이션을 하지
	// 않아 무브먼트의 속도가 차량 속도다.
	const FVector FrontContact(450.0f, 0.0f, 0.0f);

	VehicleMovement->Velocity = FVector::ZeroVector;
	VictimMovement->Velocity = FVector(-400.0f, 0.0f, 0.0f);
	TestFalse(TEXT("멈춘 차로 걸어 들어가는 것은 치인 것이 아니다."),
		Impact->HandleCharacterContact(*Victim, FrontContact));
	VictimMovement->Velocity = FVector::ZeroVector;

	// 37.5km/h는 피해 구간(15~60km/h)의 한가운데라 피해 50이다.
	VehicleMovement->Velocity = FVector(1042.0f, 0.0f, 0.0f);
	TestTrue(TEXT("달리는 차가 사람을 친다."), Impact->HandleCharacterContact(*Victim, FrontContact));
	TestEqual(TEXT("다가오던 속도에 비례해 다친다."), GetHealth(*VictimAbilitySystem), 50.0f, 0.1f);
	TestEqual(TEXT("차가 가던 방향으로 차보다 빠르게 밀려난다."),
		VictimMovement->PendingLaunchVelocity.X, 1042.0 * 1.2, 1.0);
	TestEqual(TEXT("밀려날 때 뜬다."), VictimMovement->PendingLaunchVelocity.Z, 250.0, 1.0);

	TestFalse(TEXT("한 번 부딪힐 때 접촉이 여러 번 와도 한 번만 친다."),
		Impact->HandleCharacterContact(*Victim, FrontContact));
	TestEqual(TEXT("다시 다치지 않는다."), GetHealth(*VictimAbilitySystem), 50.0f, 0.1f);

	TestFalse(TEXT("탄 사람은 치지 않는다."),
		Impact->HandleCharacterContact(*Passenger, FrontContact));

	VehicleMovement->Velocity = FVector(300.0f, 0.0f, 0.0f);
	TestTrue(TEXT("느린 차도 사람을 밀어낸다."),
		Impact->HandleCharacterContact(*Bystander, FVector(450.0f, 400.0f, 0.0f)));
	TestEqual(TEXT("피해 속도보다 느리면 다치지 않는다."),
		GetHealth(*Bystander->GetAbilitySystemComponent()), 100.0f);
	TestFalse(TEXT("밀어내기는 건다."),
		Bystander->GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero());

	// 차체 피격 알림으로 온 접촉도 같은 판정을 거친다.
	VehicleMovement->Velocity = FVector(1667.0f, 0.0f, 0.0f);
	FHitResult Hit;
	Hit.ImpactPoint = FVector(450.0f, -400.0f, 0.0f);
	Rig.Vehicle->GetMesh()->OnComponentHit.Broadcast(
		Rig.Vehicle->GetMesh(),
		Runner,
		Runner->GetCapsuleComponent(),
		FVector::ZeroVector,
		Hit);
	TestTrue(TEXT("60km/h로 치면 즉사한다."), Runner->IsDead());

	Rig.TearDown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDVehicleExitFallDamageTest,
	"PADO.Vehicle.Damage.ExitFallDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDVehicleExitFallDamageTest::RunTest(const FString& Parameters)
{
	using namespace PDVehicleSystemTests;
	FVehicleRig Rig;
	if (!TestTrue(TEXT("차량을 준비한다."), Rig.SetUp()))
	{
		Rig.TearDown();
		return false;
	}

	APDPlayerCharacter* Rider = Rig.SpawnRider(FVector(50.0f, 300.0f, 0.0f));
	UPawnMovementComponent* VehicleMovement = Rig.Vehicle->GetMovementComponent();
	UAbilitySystemComponent* AbilitySystem = Rider ? Rider->GetAbilitySystemComponent() : nullptr;
	if (TestNotNull(TEXT("탑승자를 준비한다."), Rider) &&
		TestNotNull(TEXT("탑승자의 ASC가 있다."), AbilitySystem) &&
		TestNotNull(TEXT("차량 무브먼트가 있다."), VehicleMovement))
	{
		// 착지는 무브먼트가 처리한다. 테스트 World는 틱이 없어 착지를 직접 알린다.
		const FHitResult Ground;

		// 50km/h는 피해 구간(20~80km/h)의 한가운데라 피해 50이다.
		TestTrue(TEXT("동승석에 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		VehicleMovement->Velocity = FVector(1389.0f, 0.0f, 0.0f);
		TestTrue(TEXT("달리는 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestEqual(TEXT("내리는 순간에는 다치지 않는다."), GetHealth(*AbilitySystem), 100.0f);
		Rider->Landed(Ground);
		TestEqual(TEXT("착지할 때 하차 속도에 비례해 다친다."), GetHealth(*AbilitySystem), 50.0f, 0.1f);
		Rider->Landed(Ground);
		TestEqual(TEXT("하차 피해는 한 번이다."), GetHealth(*AbilitySystem), 50.0f, 0.1f);

		TestTrue(TEXT("다시 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		VehicleMovement->Velocity = FVector(400.0f, 0.0f, 0.0f);
		TestTrue(TEXT("천천히 가는 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		Rider->Landed(Ground);
		TestEqual(TEXT("20km/h보다 느리면 다치지 않는다."), GetHealth(*AbilitySystem), 50.0f, 0.1f);

		TestTrue(TEXT("다시 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		VehicleMovement->Velocity = FVector(2222.0f, 0.0f, 0.0f);
		TestTrue(TEXT("빠르게 달리는 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		TestTrue(TEXT("착지 전에 다시 탄다."), Rig.Occupancy->TryEnter(Rider, Rig.FrontPassengerSeat));
		Rider->Landed(Ground);
		TestEqual(TEXT("착지 전에 다시 타면 앞선 하차의 피해는 없다."),
			GetHealth(*AbilitySystem), 50.0f, 0.1f);

		VehicleMovement->Velocity = FVector(2222.0f, 0.0f, 0.0f);
		TestTrue(TEXT("80km/h로 달리는 차에서 내린다."), Rig.Occupancy->TryExit(Rider));
		Rider->Landed(Ground);
		TestTrue(TEXT("80km/h면 최대 피해(100)로 죽는다."), Rider->IsDead());
	}

	Rig.TearDown();
	return true;
}

#endif
