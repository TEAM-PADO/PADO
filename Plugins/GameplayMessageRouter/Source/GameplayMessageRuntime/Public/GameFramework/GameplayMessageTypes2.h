#pragma once

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GameplayMessageTypes2.generated.h"

class UGameplayMessageRouter;

// 메시지 리스너의 매칭 규칙
UENUM(BlueprintType)
enum class EGameplayMessageMatch : uint8
{
	// 정확한 일치(ExactMatch): 완전히 동일한 채널로 발행된 메시지만 수신합니다.
	// (예: "A.B"를 등록하면 A.B로 발행된 메시지만 수신하며, A.B.C는 수신하지 않습니다)
	ExactMatch,

	// 부분 일치(PartialMatch): 동일한 채널을 루트(상위)로 하는 모든 하위 메시지를 수신합니다.
	// (예: "A.B"를 등록하면 A.B는 물론 A.B.C로 발행된 메시지도 함께 수신합니다)
	PartialMatch
};

/**
 * FGameplayMessageListenerParams
 *
 * 게임플레이 메시지 리스너를 등록할 때 필요한 옵션과 콜백을 패키징하는 매개변수 구조체(Parameter Object)입니다.
 *
 * ------------------------------------------------------------------------------------------------------
 * 💡 [왜 템플릿(template<typename FMessageStructType>) 형식으로 설계되었는가?]
 *
 * 1. 컴파일 타임 타입 안전성 (Type Safety)
 *    - 수신하고자 하는 메시지 구조체(예: FLyraVerbMessage, FPlayerScoreChangedMessage 등)의 타입을 템플릿 매개변수로 명시함으로써,
 *      잘못된 매개변수 타입을 받는 콜백 함수나 멤버 함수가 바인딩되는 실수를 컴파일 단계에서 방지합니다.
 *
 * 2. 타입 캐스팅 자동화 및 보일러플레이트 제거
 *    - GMS 서브시스템 내부는 어떤 USTRUCT도 다룰 수 있도록 내부적으로 일반화된 void* 포인터(FGameplayMessageListenerData)로 보관합니다.
 *    - 이 구조체가 템플릿으로 타입을 알고 있기 때문에, 서브시스템에 등록되는 순간 void* 페이로드를 사용자 정의 구조체 참조(const FMessageStructType&)로
 *      안전하게 변환(Thunk)해주는 래퍼 콜백을 자동으로 생성할 수 있습니다. (사용자가 수동으로 형변환할 필요가 없음)
 *
 * 3. 엔진 리플렉션(UScriptStruct) 자동 추출
 *    - 템플릿 인자를 통해 TBaseStructure<FMessageStructType>::Get()을 호출하여, 엔진의 UScriptStruct 메타데이터를
 *      런타임 타입 검증(IsChildOf 등)에 손쉽게 연동할 수 있습니다.
 *
 * 4. 확장성 (Parameter Object Pattern)
 *    - 단순한 (채널, 콜백) 인자 전달 방식 대신 이 구조체를 사용하면, 향후 필터링 조건, 실행 우선순위 등의
 *      새로운 고급 옵션이 추가되더라도 기존 API 시그니처를 깨뜨리지 않고 유연하게 확장할 수 있습니다.
 * ------------------------------------------------------------------------------------------------------
 */
template<typename FMessageStructType>
struct FGameplayMessageListenerParams
{
	/**
	 * 채널 매칭 규칙 (기본값: ExactMatch)
	 * - ExactMatch: 완전히 동일한 태그 채널로 발행된 메시지만 수신
	 * - PartialMatch: 해당 태그의 하위 자식 태그 채널로 발행된 메시지까지 모두 수신 (예: "Lyra.Damage" 구독 시 "Lyra.Damage.Headshot"도 수신)
	 */
	EGameplayMessageMatch MatchType = EGameplayMessageMatch::ExactMatch;

	/** 
	 * 메시지가 브로드캐스트되었을 때 실행될 타입 지정 콜백 함수 (TFunction)
	 * 람다(Lambda)나 일반 함수 포인터를 직접 할당할 수 있습니다.
	 */
	TFunction<void(FGameplayTag, const FMessageStructType&)> OnMessageReceivedCallback;

	/**
	 * 약참조(Weak Reference) 기반으로 UObject 멤버 함수를 OnMessageReceivedCallback에 안전하게 바인딩하는 헬퍼 함수
	 *
	 * [안전성 메커니즘]
	 * - 메시지 수신 객체(예: UI Widget, ActorComponent 등)가 게임 도중 소멸(Destroy/GC)되었을 때
	 *   댕글링 포인터(Dangling Pointer)로 인해 발생하는 크래시를 방지합니다.
	 * - TWeakObjectPtr로 대상 객체를 감싸두고, 실제 메시지가 도착했을 때 WeakObject.Get()으로
	 *   객체의 유효성을 먼저 확인한 뒤에만 멤버 함수를 호출합니다.
	 *
	 * @param Object     멤버 함수를 호출할 소유자 객체 포인터 (예: this)
	 * @param Function   메시지 수신 시 실행될 멤버 함수 포인터 (&ThisClass::OnMessageReceived)
	 */
	template<typename TOwner = UObject>
	void SetMessageReceivedCallback(TOwner* Object, void(TOwner::* Function)(FGameplayTag, const FMessageStructType&))
	{
		TWeakObjectPtr<TOwner> WeakObject(Object);
		OnMessageReceivedCallback = [WeakObject, Function](FGameplayTag Channel, const FMessageStructType& Payload)
		{
			if (TOwner* StrongObject = WeakObject.Get())
			{
				(StrongObject->*Function)(Channel, Payload);
			}
		};
	}
};

