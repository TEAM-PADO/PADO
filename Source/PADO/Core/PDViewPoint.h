#pragma once

#include "CoreMinimal.h"

class AActor;

namespace PDViewPoint
{
	/**
	 * 액터가 지금 보고 있는 시점이다.
	 *
	 * 컨트롤러가 있으면 그 시점을 쓴다. 플레이어는 카메라 시점이라, 3인칭
	 * 카메라가 캐릭터 뒤 위쪽에 있어도 화면 중앙이 가리키는 곳과 맞는다.
	 * 컨트롤러가 없으면 액터의 눈 높이 시점이다.
	 */
	PADO_API void GetActorViewPoint(
		const AActor& Actor,
		FVector& OutLocation,
		FRotator& OutRotation);
}
