#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PDGameMode.generated.h"

/**
 * PADO의 기본 GameMode다.
 * 규칙과 기본 Pawn·Controller 지정은 파생 Blueprint에서 저작한다.
 */
UCLASS(Blueprintable)
class PADO_API APDGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	explicit APDGameMode(const FObjectInitializer& ObjectInitializer);
};
