#pragma once

#include "CoreMinimal.h"

class AActor;
class UObject;
struct FCollisionQueryParams;

namespace PDTargetingCollision
{
	/**
	 * 행동 판정이 맞히지 않을 자기 쪽 액터를 무시 목록에 넣는다. 행동 주체,
	 * 소스 오브젝트를 가진 액터(손에 든 아이템 등), 주체가 앉아 있는 탈것이다.
	 *
	 * 탈것의 차체 충돌은 좌석을 감싼다. 탈것을 빼지 않으면 좌석에서 시작하거나
	 * 차량 카메라에서 차를 지나가는 판정이 모두 자기 차에 먼저 막힌다.
	 */
	PADO_API void AddIgnoredSourceActors(
		FCollisionQueryParams& QueryParams,
		const AActor& SourceActor,
		const UObject* SourceObject);

	/** 주체가 앉아 있는 탈것이다. 타고 있지 않으면 null이다. */
	PADO_API AActor* FindSourceVehicle(const AActor& SourceActor);
}
