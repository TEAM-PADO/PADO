#include "GameFramework/GameplayMessageSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ScriptMacros.h"
#include "UObject/Stack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayMessageSubsystem)

DEFINE_LOG_CATEGORY(LogGameplayMessageSubsystem);

namespace UE
{
	namespace GameplayMessageSubsystem
	{
		// 콘솔 변수(CVar): 인게임 콘솔(~)이나 콘솔창에서 'GameplayMessageSubsystem.LogMessages 1'을 입력하면
		// 라우터를 통과하는 모든 메시지와 페이로드 상세 내용이 실시간 출력됩니다.
		static int32 ShouldLogMessages = 0;
		static FAutoConsoleVariableRef CVarShouldLogMessages(TEXT("GameplayMessageSubsystem.LogMessages"),
			ShouldLogMessages,
			TEXT("게임플레이 메시지 서브시스템을 통해 브로드캐스트되는 메시지를 로그에 출력할지 여부 (1=활성화, 0=비활성화)"));
	}
}

//////////////////////////////////////////////////////////////////////
// FGameplayMessageListenerHandle

void FGameplayMessageListenerHandle::Unregister()
{
	// 핸들이 보관 중인 서브시스템 약참조가 아직 유효한지 확인 후 등록 해제 요청
	if (UGameplayMessageSubsystem* StrongSubsystem = Subsystem.Get())
	{
		StrongSubsystem->UnregisterListener(*this);
		// 중복 해제 방지를 위해 내부 필드 초기화
		Subsystem.Reset();
		Channel = FGameplayTag();
		ID = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// UGameplayMessageSubsystem

UGameplayMessageSubsystem& UGameplayMessageSubsystem::Get(const UObject* WorldContextObject)
{
	// 1. 전달받은 컨텍스트 객체(액터, 컴포넌트, 위젯 등)로부터 UWorld를 추출합니다.
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);

	// 2. 해당 월드가 속한 UGameInstance로부터 UGameplayMessageSubsystem 인스턴스를 가져옵니다.
	UGameplayMessageSubsystem* Router = UGameInstance::GetSubsystem<UGameplayMessageSubsystem>(World->GetGameInstance());
	check(Router);
	return *Router;
}

bool UGameplayMessageSubsystem::HasInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	UGameplayMessageSubsystem* Router = World != nullptr ? UGameInstance::GetSubsystem<UGameplayMessageSubsystem>(World->GetGameInstance()) : nullptr;
	return Router != nullptr;
}

void UGameplayMessageSubsystem::Deinitialize()
{
	// 게임 인스턴스/서브시스템 종료 시 메모리 누수 방지를 위해 모든 리스너 맵을 비웁니다.
	ListenerMap.Reset();

	Super::Deinitialize();
}

void UGameplayMessageSubsystem::BroadcastMessageInternal(FGameplayTag Channel, const UScriptStruct* StructType, const void* MessageBytes)
{
	// 1. [디버그 로깅] CVar(GameplayMessageSubsystem.LogMessages 1) 활성화 시 구조체 텍스트 직렬화 출력
	if (UE::GameplayMessageSubsystem::ShouldLogMessages != 0)
	{
		FString* pContextString = nullptr;
#if WITH_EDITOR
		if (GIsEditor)
		{
			extern ENGINE_API FString GPlayInEditorContextString;
			pContextString = &GPlayInEditorContextString;
		}
#endif

		// ExportText: 리플렉션을 이용해 메시지 구조체(값)를 사람이 읽을 수 있는 텍스트 포맷으로 자동 변환
		FString HumanReadableMessage;
		StructType->ExportText(/*out*/ HumanReadableMessage, MessageBytes, /*Defaults=*/ nullptr, /*OwnerObject=*/ nullptr, PPF_None, /*ExportRootScope=*/ nullptr);
		UE_LOG(LogGameplayMessageSubsystem, Log, TEXT("BroadcastMessage(%s, %s, %s)"), pContextString ? **pContextString : *GetPathNameSafe(this), *Channel.ToString(), *HumanReadableMessage);
	}

	// 2. [계층형 태그 순회 브로드캐스트]
	// 전달된 채널 태그부터 시작하여 상위 부모 태그로 거슬러 올라가며 리스너를 탐색합니다.
	// 예: "Lyra.Damage.Headshot" -> "Lyra.Damage" -> "Lyra"
	bool bOnInitialTag = true;
	for (FGameplayTag Tag = Channel; Tag.IsValid(); Tag = Tag.RequestDirectParent())
	{
		if (const FChannelListenerList* pList = ListenerMap.Find(Tag))
		{
			/**
			 * [재진입성(Re-entrancy) 안전 보장]
			 * 콜백 실행 도중 리스너 함수 내부에서 새로운 리스너를 등록하거나, 기존 리스너를 Unregister하여
			 * 원본 pList->Listeners 배열이 수정/재할당될 수 있습니다.
			 * 이를 방지하기 위해 배열의 복사본(ListenerArray)을 만들어 안전하게 순회합니다.
			 */
			TArray<FGameplayMessageListenerData> ListenerArray(pList->Listeners);

			for (const FGameplayMessageListenerData& Listener : ListenerArray)
			{
				// 첫 번째 태그(정확한 태그)이거나, 부모 태그의 경우 PartialMatch(부분 일치 허용)로 등록된 리스너만 호출
				if (bOnInitialTag || (Listener.MatchType == EGameplayMessageMatch::PartialMatch))
				{
					// 리스너가 기대하는 구조체 타입이 소멸되었는지 검사
					if (Listener.bHadValidType && !Listener.ListenerStructType.IsValid())
					{
						UE_LOG(LogGameplayMessageSubsystem, Warning, TEXT("Listener struct type has gone invalid on Channel %s. Removing listener from list"), *Channel.ToString());
						UnregisterListenerInternal(Channel, Listener.HandleID);
						continue;
					}

					/**
					 * [런타임 타입 안전성 검증 (IsChildOf)]
					 * 발신자가 보낸 구조체 타입(StructType)이 수신자가 기대한 구조체 타입(ListenerStructType)과 일치하거나,
					 * 자식 구조체인 경우에만 콜백을 실행하여 메모리 오염 및 크래시를 방지합니다.
					 */
					if (!Listener.bHadValidType || StructType->IsChildOf(Listener.ListenerStructType.Get()))
					{
						Listener.ReceivedCallback(Channel, StructType, MessageBytes);
					}
					else
					{
						UE_LOG(LogGameplayMessageSubsystem, Error, TEXT("Struct type mismatch on channel %s (broadcast type %s, listener at %s was expecting type %s)"),
							*Channel.ToString(),
							*StructType->GetPathName(),
							*Tag.ToString(),
							*Listener.ListenerStructType->GetPathName());
					}
				}
			}
		}
		bOnInitialTag = false;
	}
}

void UGameplayMessageSubsystem::K2_BroadcastMessage(FGameplayTag Channel, const int32& Message)
{
	// 블루프린트 노드는 CustomThunk로 선언되어 있으므로, 이 C++ 함수는 절대 직접 호출되지 않습니다.
	// 대신 아래의 execK2_BroadcastMessage가 실행됩니다.
	checkNoEntry();
}

/**
 * execK2_BroadcastMessage
 *
 * 블루프린트 가상머신(Unreal VM)에서 'Broadcast Message' 노드가 실행될 때 호출되는 CustomThunk 바이트코드 실행기입니다.
 * 블루프린트 그래프에서 와일드카드로 연결된 임의의 구조체(FStructProperty)를 동적으로 파싱하여 브로드캐스트합니다.
 */
DEFINE_FUNCTION(UGameplayMessageSubsystem::execK2_BroadcastMessage)
{
	// 1. 첫 번째 파라미터(Channel: FGameplayTag) 추출
	P_GET_STRUCT(FGameplayTag, Channel);

	// 2. 두 번째 파라미터(와일드카드 구조체 Message)의 스택 주소 및 FStructProperty 리플렉션 메타데이터 추출
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* MessagePtr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_FINISH;

	// 3. 유효성 검사 후 내부 C++ 브로드캐스트 함수로 전달
	if (ensure((StructProp != nullptr) && (StructProp->Struct != nullptr) && (MessagePtr != nullptr)))
	{
		P_THIS->BroadcastMessageInternal(Channel, StructProp->Struct, MessagePtr);
	}
}

FGameplayMessageListenerHandle UGameplayMessageSubsystem::RegisterListenerInternal(
	FGameplayTag Channel, 
	TFunction<void(FGameplayTag, const UScriptStruct*, const void*)>&& Callback, 
	const UScriptStruct* StructType, 
	EGameplayMessageMatch MatchType)
{
	// 해당 채널의 리스너 목록을 가져오거나 새로 생성
	FChannelListenerList& List = ListenerMap.FindOrAdd(Channel);

	// 새로운 리스너 엔트리 추가 및 정보 설정
	FGameplayMessageListenerData& Entry = List.Listeners.AddDefaulted_GetRef();
	Entry.ReceivedCallback = MoveTemp(Callback);
	Entry.ListenerStructType = StructType;
	Entry.bHadValidType = StructType != nullptr;
	Entry.HandleID = ++List.HandleID; // 1씩 증가하는 채널 내 고유 ID 발급
	Entry.MatchType = MatchType;

	// 외부에서 해제 시 사용할 핸들 반환
	return FGameplayMessageListenerHandle(this, Channel, Entry.HandleID);
}

void UGameplayMessageSubsystem::UnregisterListener(FGameplayMessageListenerHandle Handle)
{
	if (Handle.IsValid())
	{
		check(Handle.Subsystem == this);

		UnregisterListenerInternal(Handle.Channel, Handle.ID);
	}
	else
	{
		UE_LOG(LogGameplayMessageSubsystem, Warning, TEXT("Trying to unregister an invalid Handle."));
	}
}

void UGameplayMessageSubsystem::UnregisterListenerInternal(FGameplayTag Channel, int32 HandleID)
{
	if (FChannelListenerList* pList = ListenerMap.Find(Channel))
	{
		// 고유 HandleID로 리스너 탐색
		int32 MatchIndex = pList->Listeners.IndexOfByPredicate([ID = HandleID](const FGameplayMessageListenerData& Other) { return Other.HandleID == ID; });
		if (MatchIndex != INDEX_NONE)
		{
			// RemoveAtSwap: 순서 유지가 필요 없으므로 O(1)의 빠른 스왑 삭제 수행
			pList->Listeners.RemoveAtSwap(MatchIndex);
		}

		// 해당 채널에 더 이상 등록된 리스너가 없으면 Map에서 채널 키 자체를 제거하여 메모리 절약
		if (pList->Listeners.Num() == 0)
		{
			ListenerMap.Remove(Channel);
		}
	}
}

