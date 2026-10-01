#pragma once

#include "CoreMinimal.h"
#include "PDBossRecipientInteractionType.generated.h"

/** 보스 처치 뒤 배송지에서 서버가 확정할 수취인 상호작용 흐름입니다. */
UENUM(BlueprintType)
enum class EPDBossRecipientInteractionType : uint8
{
	/** 아직 보스 수취인 상호작용을 정의하지 않은 상태입니다. */
	None UMETA(DisplayName = "None"),

	/** 수취인이 직접 나타나 배송을 완료하는 첫 번째 보스 흐름입니다. */
	DirectRecipient UMETA(DisplayName = "Direct Recipient"),

	/** 충전 중 안내와 확인 선택지 뒤 로봇 수취인으로 이어지는 두 번째 보스 흐름입니다. */
	ConfirmChargingThenRobot UMETA(DisplayName = "Confirm Charging Then Robot"),

	/** 충전 중 안내와 확인 선택지 뒤 인간 수취인으로 이어지는 세 번째 보스 흐름입니다. */
	ConfirmChargingThenHuman UMETA(DisplayName = "Confirm Charging Then Human")
};
