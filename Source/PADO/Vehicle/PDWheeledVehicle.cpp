#include "PADO/Vehicle/PDWheeledVehicle.h"

#include "Camera/CameraComponent.h"
#include "ChaosVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Character/PDPlayerController.h"
#include "PADO/Vehicle/Component/PDVehicleOccupancyComponent.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"
#include "PADO/Vehicle/Component/PDWheeledVehicleMovementComponent.h"
#include "Physics/NetworkPhysicsComponent.h"

namespace PDWheeledVehicleDefaults
{
	constexpr float CameraArmLength = 650.0f;
	const FVector CameraSocketOffset(0.0f, 0.0f, 150.0f);
}

APDWheeledVehicle::APDWheeledVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPDWheeledVehicleMovementComponent>(
		AWheeledVehiclePawn::VehicleMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	// 서버 권한 물리를 운전자 머신이 예측하고, 어긋나면 되감아 다시 계산한다.
	SetPhysicsReplicationMode(EPhysicsReplicationMode::Resimulation);

	// 탈것에는 아무도 빙의하지 않는다. 엔진 Pawn은 맵에 놓이면 AI 컨트롤러가
	// 자동으로 빙의하는데, 네트워크 물리는 Owner보다 Pawn의 컨트롤러를 먼저 본다.
	// AI가 붙어 있으면 서버가 자기 입력을 쓰며 운전자의 입력을 무시하고 덮어쓴다.
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	// 차량은 언제나 물리로 움직인다. 엔진 기본값은 꺼져 있다.
	GetMesh()->BodyInstance.bSimulatePhysics = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = PDWheeledVehicleDefaults::CameraArmLength;
	CameraBoom->SocketOffset = PDWheeledVehicleDefaults::CameraSocketOffset;

	// 빙의하지 않으므로 Pawn의 컨트롤 회전을 쓸 수 없다. 이 차를 보는 탑승자
	// 컨트롤러의 회전을 Tick에서 직접 넣는다.
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetUsingAbsoluteRotation(true);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OccupancyComponent =
		CreateDefaultSubobject<UPDVehicleOccupancyComponent>(TEXT("Occupancy"));
}

void APDWheeledVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 카메라 컴포넌트는 복제되지 않아 머신마다 따로 돈다. 운전자든 동승자든
	// 이 머신에서 이 차를 보는 탑승자의 시점을 따라간다.
	if (const APDPlayerController* Viewer = FindLocalViewer())
	{
		CameraBoom->SetWorldRotation(Viewer->GetControlRotation());
	}
}

bool APDWheeledVehicle::CanInteract_Implementation(
	const FPDInteractionContextStruct& Context) const
{
	const UPDVehicleOccupantComponent* Occupant = Context.Instigator
		? Context.Instigator->GetVehicleOccupantComponent()
		: nullptr;
	return Occupant &&
		!Occupant->IsSeated() &&
		OccupancyComponent &&
		OccupancyComponent->HasFreeSeat();
}

bool APDWheeledVehicle::Interact_Implementation(
	const FPDInteractionContextStruct& Context)
{
	return OccupancyComponent &&
		OccupancyComponent->TryEnter(
			Context.Instigator,
			ResolvePreferredSeat(Context.AimedComponent));
}

void APDWheeledVehicle::SetVehicleController(AController* NewController)
{
	AController* OldController = VehicleController;
	if (!HasAuthority() || OldController == NewController)
	{
		return;
	}

	VehicleController = NewController;

	// 차량 RPC와 물리 입력이 조종자의 연결로 오간다. 네트워크 물리는 빙의가
	// 없으면 Owner를 컨트롤러로 보고, 차량 무브먼트는 OverrideController를 본다.
	SetOwner(NewController);

	// 빙의할 때 엔진이 하는 일 중 네트워크 역할만 따라 한다(APawn::PossessedBy).
	// 엔진의 물리 복제와 예측은 조종하는 머신의 Pawn을 자율 프록시로 가정하므로
	// 빙의한 운전과 같은 조건을 만든다.
	const APlayerController* NewPlayer = Cast<APlayerController>(NewController);
	SetAutonomousProxy(NewPlayer && !NewPlayer->IsLocalController());

	if (UChaosVehicleMovementComponent* Movement = GetVehicleMovementComponent())
	{
		Movement->SetOverrideController(NewController);
	}

	ApplyServerInputOwnership(NewController == nullptr);
	ForceNetUpdate();

	// 리슨 서버 호스트가 조종자면 복제 알림이 오지 않으므로 여기서 알린다.
	NotifyLocalControlChanged(OldController, NewController);
}

void APDWheeledVehicle::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !VehicleController)
	{
		ApplyServerInputOwnership(true);
	}
}

void APDWheeledVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 조종 중에 차량이 사라지면 운전자의 입력을 되돌린다.
	NotifyLocalControlChanged(VehicleController, nullptr);

	// 이 차를 보던 탑승자의 카메라도 되돌린다. 하차 상태가 뒤늦게 도착해도
	// 다시 처리하지 않는다.
	if (APDPlayerController* Viewer = FindLocalViewer())
	{
		Viewer->EndVehicleView();
	}
	Super::EndPlay(EndPlayReason);
}

void APDWheeledVehicle::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDWheeledVehicle, VehicleController);
}

void APDWheeledVehicle::OnRep_VehicleController(AController* OldController)
{
	NotifyLocalControlChanged(OldController, VehicleController);
}

void APDWheeledVehicle::ApplyServerInputOwnership(bool bServerProducesInput)
{
	// 엔진 차량은 로컬 PlayerController가 없으면 게임 스레드 입력을 물리에 넘기지
	// 않고 마지막 입력을 쓴다. 데디케이티드 서버에는 로컬 PlayerController가 없으므로
	// 운전자가 없으면 무브먼트가 물리 시뮬레이션에서 입력을 직접 비운다.
	if (UPDWheeledVehicleMovementComponent* Movement =
		Cast<UPDWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Movement->SetDriverless(bServerProducesInput);
	}

	// 네트워크 물리의 입력 생산자도 함께 넘긴다. 운전자가 있으면 그 머신이 만든다.
	if (UNetworkPhysicsComponent* NetworkPhysics =
		FindComponentByClass<UNetworkPhysicsComponent>())
	{
		NetworkPhysics->SetIsRelayingLocalInputs(bServerProducesInput);
	}
}

void APDWheeledVehicle::NotifyLocalControlChanged(
	AController* OldController,
	AController* NewController)
{
	if (OldController == NewController)
	{
		return;
	}

	APDPlayerController* OldPlayer = Cast<APDPlayerController>(OldController);
	if (OldPlayer && OldPlayer->IsLocalController())
	{
		OldPlayer->EndVehicleControl(this);
	}

	APDPlayerController* NewPlayer = Cast<APDPlayerController>(NewController);
	if (NewPlayer && NewPlayer->IsLocalController())
	{
		NewPlayer->BeginVehicleControl(this);
	}
}

APDPlayerController* APDWheeledVehicle::FindLocalViewer() const
{
	UWorld* World = GetWorld();
	if (!World || !CameraBoom || GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	// 카메라는 머신당 하나다. 한 머신에 로컬 플레이어가 둘이면(화면 분할)
	// 나눠 쓸 수 없으므로 먼저 찾은 쪽만 쓴다.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APDPlayerController* PlayerController = Cast<APDPlayerController>(It->Get());
		if (PlayerController &&
			PlayerController->IsLocalController() &&
			PlayerController->GetViewedVehicle() == this)
		{
			return PlayerController;
		}
	}
	return nullptr;
}

UPDVehicleSeatComponent* APDWheeledVehicle::ResolvePreferredSeat(
	UPrimitiveComponent* AimedComponent) const
{
	// 문 충돌체처럼 좌석 아래에 붙은 컴포넌트를 조준했으면 그 좌석이 희망 좌석이다.
	for (USceneComponent* Current = AimedComponent;
		Current && Current->GetOwner() == this;
		Current = Current->GetAttachParent())
	{
		if (UPDVehicleSeatComponent* Seat = Cast<UPDVehicleSeatComponent>(Current))
		{
			return Seat;
		}
	}
	return nullptr;
}
