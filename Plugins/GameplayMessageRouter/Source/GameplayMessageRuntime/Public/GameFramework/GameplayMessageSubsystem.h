#pragma once

#include "GameplayMessageTypes2.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/WeakObjectPtr.h"

#include "GameplayMessageSubsystem.generated.h"

#define UE_API GAMEPLAYMESSAGERUNTIME_API

class UGameplayMessageSubsystem;
struct FFrame;

GAMEPLAYMESSAGERUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogGameplayMessageSubsystem, Log, All);

class UAsyncAction_ListenForGameplayMessage;

/**
 * FGameplayMessageListenerHandle
 * 등록된 메시지 리스너를 식별하고, 나중에 등록을 해제(Unregister)하기 위해 사용되는 불투명 핸들(Opaque Handle)입니다.
 *
 * [왜 생 포인터 대신 Handle 구조체를 사용하는가?]
 * - 서브시스템 내부의 리스너 배열(TArray)은 추가/삭제 시 메모리 재할당으로 인해 요소의 메모리 주소가 바뀔 수 있습니다.
 * - 따라서 포인터를 직접 들고 있는 대신, 채널(FGameplayTag)과 고유 발급 번호(int32 ID)의 조합으로 리스너를 안전하게 식별합니다.
 *
 * @see UGameplayMessageSubsystem::RegisterListener 및 UGameplayMessageSubsystem::UnregisterListener
 */
USTRUCT(BlueprintType)
struct FGameplayMessageListenerHandle
{
public:
	GENERATED_BODY()

	FGameplayMessageListenerHandle() {}

	/** 핸들에 보관된 정보를 바탕으로 서브시스템에서 즉시 리스너 등록을 해제합니다. */
	UE_API void Unregister();

	/** 유효한 핸들인지 확인 (ID가 0이 아니면 유효) */
	bool IsValid() const { return ID != 0; }

private:
	// 리스너가 등록된 서브시스템을 약참조(WeakPtr)로 보관 (서브시스템 소멸 시 댕글링 방지)
	UPROPERTY(Transient)
	TWeakObjectPtr<UGameplayMessageSubsystem> Subsystem;

	// 리스너가 구독 중인 대상 채널 태그
	UPROPERTY(Transient)
	FGameplayTag Channel;

	// 채널 내에서 발급된 리스너의 고유 정수 ID (0은 무효)
	UPROPERTY(Transient)
	int32 ID = 0;

	FDelegateHandle StateClearedHandle;

	friend UGameplayMessageSubsystem;

	// 서브시스템 내부에서만 핸들을 생성할 수 있도록 private 생성자 제공
	FGameplayMessageListenerHandle(UGameplayMessageSubsystem* InSubsystem, FGameplayTag InChannel, int32 InID) : Subsystem(InSubsystem), Channel(InChannel), ID(InID) {}
};

/** 
 * FGameplayMessageListenerData
 *
 * 서브시스템 내부의 ListenerMap에 보관되는 등록된 단일 리스너의 세부 실행 데이터입니다.
 */
USTRUCT()
struct FGameplayMessageListenerData
{
	GENERATED_BODY()

	/**
	 * 메시지 수신 시 실행될 타입 소거(Type-Erased) 콜백 함수
	 * 
	 * [왜 void*와 UScriptStruct*를 받는가?]
	 * - 서브시스템 클래스 자체는 특정 메시지 구조체(FLyraVerbMessage 등)에 종속되지 않는 범용 시스템이어야 합니다.
	 * - 따라서 내부에서는 제네릭 void* 포인터 형태로 콜백을 저장하고,
	 *   RegisterListener 템플릿 함수에서 타입 캐스팅 Thunk 래퍼를 생성하여 이 콜백에 전달합니다.
	 */
	TFunction<void(FGameplayTag, const UScriptStruct*, const void*)> ReceivedCallback;

	// 리스너 고유 식별 번호 (FGameplayMessageListenerHandle의 ID와 매칭)
	int32 HandleID;

	// 채널 매칭 규칙 (ExactMatch: 정확히 일치, PartialMatch: 부모 태그를 통한 하위 채널 수신)
	EGameplayMessageMatch MatchType;

	// 수신자가 기대하는 UScriptStruct 메타데이터 포인터 (런타임 타입 일치 검증용)
	TWeakObjectPtr<const UScriptStruct> ListenerStructType = nullptr;

	// 타입 정보가 유효하게 지정되었는지 여부
	bool bHadValidType = false;
};

/**
 * UGameplayMessageSubsystem
 *
 * Gameplay Tag 채널을 기반으로 메시지를 발행(Publish/Broadcast)하고 구독(Subscribe/Listen)할 수 있게 해주는
 * 경량 Pub-Sub 라우팅 서브시스템입니다.
 *
 * ------------------------------------------------------------------------------------------------------
 * 💡 [핵심 설계 및 특징]
 *
 * 1. 완벽한 디커플링 (Loose Coupling)
 *    - 발신자(이벤트 발생원)와 수신자(UI, 사운드, 업적 등)가 서로의 포인터나 클래스를 알 필요가 전혀 없습니다.
 *    - 오직 "채널(GameplayTag)"과 "데이터 포맷(USTRUCT)" 두 가지만 합의하면 통신이 가능합니다.
 *
 * 2. 게임 인스턴스 수명 주기 (UGameInstanceSubsystem)
 *    - 레벨(맵) 전환이나 심리스 트래블 시에도 유지되므로, UI 위젯 생성 전/후나 게임 모드 전환 시에도 안정적입니다.
 *
 * 3. 로컬 전용 서브시스템 (Local-Only Broadcast)
 *    - 이 서브시스템은 네트워크 패킷을 자동으로 전송하지 않습니다.
 *    - 멀티플레이어 환경에서는 RPC나 FastArraySerializer로 클라이언트에 데이터를 복제한 뒤,
 *      클라이언트 로컬의 GMS로 브로드캐스트하여 로컬 UI/이펙트를 반응시키는 방식으로 조합합니다.
 *
 * 4. 호출 순서 비보장 (Unordered Execution)
 *    - 동일한 채널에 여러 리스너가 존재할 때, 호출 순서는 보장되지 않으며 언제든 바뀔 수 있습니다.
 *    - 따라서 리스너 간의 상호 종속성을 만들지 않아야 합니다.
 * ------------------------------------------------------------------------------------------------------
 */
UCLASS(MinimalAPI)
class UGameplayMessageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	friend UAsyncAction_ListenForGameplayMessage;

public:

	/**
	 * 전달된 월드 컨텍스트 객체를 통해 해당 월드가 속한 GameInstance의 UGameplayMessageSubsystem 참조를 가져옵니다.
	 *
	 * [동작 흐름]
	 * WorldContextObject -> UWorld -> UGameInstance -> GetSubsystem<UGameplayMessageSubsystem>()
	 */
	static UE_API UGameplayMessageSubsystem& Get(const UObject* WorldContextObject);

	/**
	 * 제공된 월드 컨텍스트에서 유효한 GameplayMessageRouter 서브시스템이 활성화되어 있는지 여부를 안전하게 확인합니다.
	 */
	static UE_API bool HasInstance(const UObject* WorldContextObject);

	//~USubsystem interface
	/** 서브시스템 종료 시 등록된 모든 리스너 맵을 정리합니다. */
	UE_API virtual void Deinitialize() override;
	//~End of USubsystem interface

	/**
	 * 지정된 채널로 메시지 구조체를 브로드캐스트(발행)합니다.
	 *
	 * [동작 원리]
	 * 1. TBaseStructure<FMessageStructType>::Get()을 통해 엔진 리플렉션 UScriptStruct 포인터를 획득합니다.
	 * 2. 내부 BroadcastMessageInternal로 전달하여 채널 태그를 구독 중인 리스너들에게 페이로드를 전달합니다.
	 *
	 * @param Channel			메시지를 발행할 채널 태그 (예: TAG_Lyra_Elimination_Message)
	 * @param Message			전송할 메시지 구조체 인스턴스 (값 타입 USTRUCT)
	 */
	template <typename FMessageStructType>
	void BroadcastMessage(FGameplayTag Channel, const FMessageStructType& Message)
	{
		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		BroadcastMessageInternal(Channel, StructType, &Message);
	}

	/**
	 * [C++ 리스너 등록 - 람다/함수 객체 버전]
	 * 지정된 채널에서 메시지를 수신할 람다(Lambda) 또는 일반 콜백 함수를 등록합니다.
	 *
	 * [Thunk 변환 메커니즘]
	 * - 내부적으로 생성되는 ThunkCallback 람다가 로우레벨 void* 페이로드를 사용자가 기대하는
	 *   구체적 구조체 참조(*reinterpret_cast<const FMessageStructType*>(SenderPayload))로 자동 캐스팅해줍니다.
	 *
	 * @param Channel			수신 대기할 채널 태그
	 * @param Callback			메시지 수신 시 실행될 콜백 (void(FGameplayTag Channel, const FMessageStructType& Payload))
	 * @param MatchType			채널 매칭 규칙 (기본값: ExactMatch)
	 * @return 리스너 해제용 핸들 (FGameplayMessageListenerHandle)
	 */
	template <typename FMessageStructType>
	FGameplayMessageListenerHandle RegisterListener(FGameplayTag Channel, TFunction<void(FGameplayTag, const FMessageStructType&)>&& Callback, EGameplayMessageMatch MatchType = EGameplayMessageMatch::ExactMatch)
	{
		auto ThunkCallback = [InnerCallback = MoveTemp(Callback)](FGameplayTag ActualTag, const UScriptStruct* SenderStructType, const void* SenderPayload)
		{
			InnerCallback(ActualTag, *reinterpret_cast<const FMessageStructType*>(SenderPayload));
		};

		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		return RegisterListenerInternal(Channel, ThunkCallback, StructType, MatchType);
	}

	/**
	 * [C++ 리스너 등록 - UObject 멤버 함수 버전 (★가장 권장되는 패턴)]
	 * 특정 UObject 인스턴스의 멤버 함수를 메시지 수신 핸들러로 등록합니다.
	 *
	 * [약참조 안전성 보장]
	 * - TWeakObjectPtr<TOwner>를 사용하여 객체를 감싸므로, 수신 객체가 소멸(Destroy/GC)된 후
	 *   메시지가 오더라도 댕글링 포인터 크래시가 발생하지 않고 안전하게 무시됩니다.
	 *
	 * [사용 예시]
	 * Handle = MessageSystem.RegisterListener(TAG_MyMessage, this, &ThisClass::OnMyMessageReceived);
	 *
	 * @param Channel			수신 대기할 채널 태그
	 * @param Object			멤버 함수를 호출할 UObject 인스턴스 (this)
	 * @param Function			호출될 멤버 함수 포인터
	 * @return 리스너 해제용 핸들
	 */
	template <typename FMessageStructType, typename TOwner = UObject>
	FGameplayMessageListenerHandle RegisterListener(FGameplayTag Channel, TOwner* Object, void(TOwner::* Function)(FGameplayTag, const FMessageStructType&))
	{
		TWeakObjectPtr<TOwner> WeakObject(Object);
		return RegisterListener<FMessageStructType>(Channel,
			[WeakObject, Function](FGameplayTag Channel, const FMessageStructType& Payload)
			{
				if (TOwner* StrongObject = WeakObject.Get())
				{
					(StrongObject->*Function)(Channel, Payload);
				}
			});
	}

	/**
	 * [C++ 리스너 등록 - 파라미터 구조체 버전]
	 * FGameplayMessageListenerParams 구조체를 사용하여 고급 옵션과 함께 리스너를 등록합니다.
	 *
	 * @param Channel			수신 대기할 채널 태그
	 * @param Params			매칭 룰 및 콜백이 설정된 파라미터 구조체
	 * @return 리스너 해제용 핸들
	 */
	template <typename FMessageStructType>
	FGameplayMessageListenerHandle RegisterListener(FGameplayTag Channel, FGameplayMessageListenerParams<FMessageStructType>& Params)
	{
		FGameplayMessageListenerHandle Handle;

		if (Params.OnMessageReceivedCallback)
		{
			auto ThunkCallback = [InnerCallback = Params.OnMessageReceivedCallback](FGameplayTag ActualTag, const UScriptStruct* SenderStructType, const void* SenderPayload)
			{
				InnerCallback(ActualTag, *reinterpret_cast<const FMessageStructType*>(SenderPayload));
			};

			const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
			Handle = RegisterListenerInternal(Channel, ThunkCallback, StructType, Params.MatchType);
		}

		return Handle;
	}

	/**
	 * RegisterListener로부터 발급받은 핸들을 사용하여 등록된 리스너를 제거합니다.
	 * (보통 컴포넌트나 액터의 EndPlay()에서 호출합니다)
	 */
	UE_API void UnregisterListener(FGameplayMessageListenerHandle Handle);

protected:
	/**
	 * [블루프린트 전용 브로드캐스트 노드]
	 * CustomThunk 메타데이터를 사용하여 블루프린트 가상머신(VM) 수준에서 와일드카드 구조체 핀을 처리합니다.
	 * C++에서 직접 호출하는 함수가 아니므로 내부에서 checkNoEntry()를 발생시키며,
	 * 실제 실행은 아래 선언된 execK2_BroadcastMessage에서 처리됩니다.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category=Messaging, meta=(CustomStructureParam="Message", AllowAbstract="false", DisplayName="Broadcast Message"))
	UE_API void K2_BroadcastMessage(FGameplayTag Channel, const int32& Message);

	// 블루프린트 바이트코드 실행기 함수 (Unreal VM Thunk)
	DECLARE_FUNCTION(execK2_BroadcastMessage);

private:
	// 메시지 브로드캐스트의 실제 핵심 로직 (태그 트리 순회 및 콜백 호출)
	UE_API void BroadcastMessageInternal(FGameplayTag Channel, const UScriptStruct* StructType, const void* MessageBytes);

	// 리스너 등록의 실제 내부 구현 (ListenerMap에 엔트리 추가 및 고유 ID 발급)
	UE_API FGameplayMessageListenerHandle RegisterListenerInternal(
		FGameplayTag Channel, 
		TFunction<void(FGameplayTag, const UScriptStruct*, const void*)>&& Callback,
		const UScriptStruct* StructType,
		EGameplayMessageMatch MatchType);

	// 채널과 고유 HandleID를 기반으로 맵에서 리스너 엔트리를 제거하는 내부 헬퍼
	UE_API void UnregisterListenerInternal(FGameplayTag Channel, int32 HandleID);

private:
	// 특정 GameplayTag 채널 하나에 등록된 리스너들의 목록과 발급 번호 카운터
	struct FChannelListenerList
	{
		TArray<FGameplayMessageListenerData> Listeners;
		int32 HandleID = 0; // 새 리스너가 추가될 때마다 1씩 증가하여 고유 ID 부여
	};

private:
	// 채널 태그를 키로 하여 리스너 목록들을 관리하는 메인 컨테이너
	TMap<FGameplayTag, FChannelListenerList> ListenerMap;
};

#undef UE_API
