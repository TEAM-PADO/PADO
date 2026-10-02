#include "PADO/Vehicle/PDVehicleContactSubsystem.h"

#include "Chaos/ContactModification.h"
#include "Chaos/ParticleHandle.h"
#include "Chaos/SimCallbackObject.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Queue.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "PhysicsPublic.h"

namespace PDVehicleContact
{
	/** 물리 스레드가 읽는다. 끄면 등록은 유지하고 접촉만 고치지 않아 엔진 기본 양방향 충돌이 된다. */
	bool bOneWayRagdollContact = true;
	FAutoConsoleVariableRef CVarOneWayRagdollContact(
		TEXT("pd.Vehicle.OneWayRagdollContact"),
		bOneWayRagdollContact,
		TEXT("탈것과 래그돌의 접촉을 한쪽 방향으로 고친다. 차는 래그돌을 밀고 래그돌은 차를 밀지 못한다. 0이면 엔진 기본 양방향 충돌이다."));

	/** 게임 스레드가 물리 스레드로 넘기는 등록 요청이다. 바디는 프록시로만 구분한다. */
	struct FRegistration
	{
		const IPhysicsProxyBase* Proxy = nullptr;
		bool bVehicle = false;
	};

	/**
	 * 물리 스레드에서 차량과 밀리기만 하는 몸의 접촉을 고친다.
	 *
	 * 목록에는 프록시 주소만 두고 읽지 않는다. 접촉 쌍의 바디(살아 있는 것)에서 얻은 프록시와
	 * 비교만 한다. 바디가 물리에서 사라지면 엔진 알림으로 목록에서 뺀다.
	 */
	class FContactCallback final : public Chaos::TSimCallbackObject<
		Chaos::FSimCallbackNoInput,
		Chaos::FSimCallbackNoOutput,
		Chaos::ESimCallbackOptions::ContactModification |
			Chaos::ESimCallbackOptions::ParticleUnregister>
	{
	public:
		/** 게임 스레드에서 부른다. 다음 물리 스텝부터 반영된다. */
		void Enqueue(const FRegistration& Registration)
		{
			PendingRegistrations.Enqueue(Registration);
		}

	private:
		virtual void OnContactModification_Internal(Chaos::FCollisionContactModifier& Modifier) override
		{
			ApplyPendingRegistrations();
			if (!bOneWayRagdollContact || VehicleProxies.IsEmpty() || PassiveProxies.IsEmpty())
			{
				return;
			}

			for (Chaos::FContactPairModifier& Pair : Modifier)
			{
				const Chaos::TVec2<Chaos::FGeometryParticleHandle*> Particles = Pair.GetParticlePair();
				const IPhysicsProxyBase* Proxy0 = Particles[0] ? Particles[0]->PhysicsProxy() : nullptr;
				const IPhysicsProxyBase* Proxy1 = Particles[1] ? Particles[1]->PhysicsProxy() : nullptr;

				int32 VehicleIndex = INDEX_NONE;
				if (VehicleProxies.Contains(Proxy0) && PassiveProxies.Contains(Proxy1))
				{
					VehicleIndex = 0;
				}
				else if (VehicleProxies.Contains(Proxy1) && PassiveProxies.Contains(Proxy0))
				{
					VehicleIndex = 1;
				}

				if (VehicleIndex == INDEX_NONE)
				{
					continue;
				}

				// 이 접촉에서만 차를 무한히 무거운 물체로 본다. 몸은 밀려나고 차는 그대로다.
				Pair.ModifyInvMassScale(0.0f, VehicleIndex);
				Pair.ModifyInvInertiaScale(0.0f, VehicleIndex);
			}
		}

		virtual void OnParticleUnregistered_Internal(
			TArray<TTuple<Chaos::FUniqueIdx, Chaos::FSingleParticlePhysicsProxy*>>& UnregisteredProxies) override
		{
			// 등록은 바디가 사라지기 전에 큐에 들어왔다. 먼저 반영해야 사라진 바디가 남지 않는다.
			ApplyPendingRegistrations();
			for (const TTuple<Chaos::FUniqueIdx, Chaos::FSingleParticlePhysicsProxy*>& Unregistered : UnregisteredProxies)
			{
				const IPhysicsProxyBase* Proxy = Unregistered.Get<1>();
				VehicleProxies.Remove(Proxy);
				PassiveProxies.Remove(Proxy);
			}
		}

		void ApplyPendingRegistrations()
		{
			FRegistration Registration;
			while (PendingRegistrations.Dequeue(Registration))
			{
				(Registration.bVehicle ? VehicleProxies : PassiveProxies).Add(Registration.Proxy);
			}
		}

		/** 게임 스레드가 넣고 물리 스레드가 꺼낸다. */
		TQueue<FRegistration, EQueueMode::Spsc> PendingRegistrations;

		/** 물리 스레드만 읽고 쓴다. */
		TSet<const IPhysicsProxyBase*> VehicleProxies;
		TSet<const IPhysicsProxyBase*> PassiveProxies;
	};

	/** 컴포넌트의 물리 바디마다 부른다. 스켈레탈 메시는 물리 에셋의 바디가 여럿이다. */
	void ForEachBodyProxy(
		UPrimitiveComponent& Body,
		TFunctionRef<void(const IPhysicsProxyBase*)> Visit)
	{
		if (const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(&Body))
		{
			for (const FBodyInstance* BodyInstance : SkeletalMesh->Bodies)
			{
				if (const Chaos::FSingleParticlePhysicsProxy* Proxy =
					BodyInstance ? BodyInstance->GetPhysicsActor() : nullptr)
				{
					Visit(Proxy);
				}
			}
			return;
		}

		if (const FBodyInstance* BodyInstance = Body.GetBodyInstance())
		{
			if (const Chaos::FSingleParticlePhysicsProxy* Proxy = BodyInstance->GetPhysicsActor())
			{
				Visit(Proxy);
			}
		}
	}
}

void UPDVehicleContactSubsystem::Deinitialize()
{
	ReleaseCallback();
	Super::Deinitialize();
}

void UPDVehicleContactSubsystem::RegisterVehicle(UPrimitiveComponent& Body)
{
	Register(Body, true);
}

void UPDVehicleContactSubsystem::RegisterPassiveBody(UPrimitiveComponent& Body)
{
	Register(Body, false);
}

bool UPDVehicleContactSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// 차와 래그돌이 실제로 도는 게임 월드다.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UPDVehicleContactSubsystem::Register(UPrimitiveComponent& Body, bool bVehicle)
{
	PDVehicleContact::FContactCallback* ContactCallback = EnsureCallback();
	if (!ContactCallback)
	{
		return;
	}

	PDVehicleContact::ForEachBodyProxy(
		Body,
		[ContactCallback, bVehicle](const IPhysicsProxyBase* Proxy)
		{
			ContactCallback->Enqueue({Proxy, bVehicle});
		});
}

PDVehicleContact::FContactCallback* UPDVehicleContactSubsystem::EnsureCallback()
{
	if (Callback)
	{
		return Callback;
	}

	const UWorld* World = GetWorld();
	FPhysScene_Chaos* PhysScene = World ? World->GetPhysicsScene() : nullptr;
	Chaos::FPhysicsSolver* Solver = PhysScene ? PhysScene->GetSolver() : nullptr;
	if (!Solver)
	{
		return nullptr;
	}

	Callback = Solver->CreateAndRegisterSimCallbackObject_External<PDVehicleContact::FContactCallback>();
	CallbackScene = PhysScene;
	PhysSceneTermHandle = FPhysicsDelegates::OnPhysSceneTerm.AddUObject(
		this,
		&UPDVehicleContactSubsystem::HandlePhysSceneTerm);
	return Callback;
}

void UPDVehicleContactSubsystem::HandlePhysSceneTerm(FPhysScene_Chaos* PhysScene)
{
	if (PhysScene == CallbackScene)
	{
		ReleaseCallback();
	}
}

void UPDVehicleContactSubsystem::ReleaseCallback()
{
	if (Callback && CallbackScene)
	{
		if (Chaos::FPhysicsSolver* Solver = CallbackScene->GetSolver())
		{
			Solver->UnregisterAndFreeSimCallbackObject_External(Callback);
		}
	}

	Callback = nullptr;
	CallbackScene = nullptr;
	if (PhysSceneTermHandle.IsValid())
	{
		FPhysicsDelegates::OnPhysSceneTerm.Remove(PhysSceneTermHandle);
		PhysSceneTermHandle.Reset();
	}
}
