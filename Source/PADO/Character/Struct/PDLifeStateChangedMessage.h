#pragma once

#include "CoreMinimal.h"
#include "PADO/Character/Enum/PDLifeState.h"
#include "PDLifeStateChangedMessage.generated.h"

class APDCharacterBase;

/**
 * 몸의 생명 상태가 바뀌었다는 Gameplay Message의 Payload다.
 *
 * `Message.Character.LifeStateChanged` 채널로 서버와 각 클라이언트에서 그 머신에
 * 상태가 적용될 때 보낸다. 관전, 게임 흐름(전멸), 킬 표시가 이 메시지를 받는다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDLifeStateChangedMessage
{
	GENERATED_BODY()

	/** 상태가 바뀐 몸이다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	TObjectPtr<APDCharacterBase> Character = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	EPDLifeState PreviousState = EPDLifeState::Alive;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	EPDLifeState NewState = EPDLifeState::Alive;

	/** 이 상태로 만든 논리적 주체다(플레이어면 PlayerState). 없을 수 있다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	TObjectPtr<AActor> Instigator = nullptr;

	/** 이 상태로 만든 물리적 원인이다(아이템·투사체). 이미 사라졌으면 없다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	TObjectPtr<AActor> EffectCauser = nullptr;
};
