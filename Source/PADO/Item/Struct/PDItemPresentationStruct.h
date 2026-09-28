#pragma once

#include "CoreMinimal.h"
#include "PDItemPresentationStruct.generated.h"

class USkeletalMesh;
class UStaticMesh;

/** 아이템을 들고 있는 동안 AnimBP가 선택할 공용 자세 키다. */
UENUM(BlueprintType)
enum class EPDHeldPose : uint8
{
	Default,
	OneHanded,
	TwoHanded,
	Heavy,
	ThrowReady
};

/** 월드와 손에서 아이템을 표현하는 불변 데이터다. */
USTRUCT(BlueprintType)
struct PADO_API FPDItemPresentationStruct
{
	GENERATED_BODY()

	/** StaticMesh와 SkeletalMesh 중 정확히 하나만 지정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FTransform MeshRelativeTransform = FTransform::Identity;

	/** 이 메시 소켓을 Holder 소켓에 일치시킨다. 비우면 Actor 원점을 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FName GripSocketName = TEXT("Grip");

	/** 비우면 Holder Component의 기본 HandSocketName을 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FName HolderSocketNameOverride = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	EPDHeldPose HeldPose = EPDHeldPose::Default;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "World",
		meta = (CollisionProfileName = true))
	FName WorldCollisionProfile = TEXT("PhysicsActor");

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "World",
		meta = (ClampMin = "1.0", Units = "cm"))
	float CollisionRadius = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	bool bSimulatePhysicsInWorld = true;

	bool Validate(FString& OutError) const;
};
