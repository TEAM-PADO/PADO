// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/Settings/Struct/PDKeyBindingEntry.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PDKeyBindingSubsystem.generated.h"

class UEnhancedInputUserSettings;
struct FKeyMappingRow;
struct FPlayerKeyMapping;

/**
 * 플레이어별 키 재지정을 Enhanced Input 사용자 설정으로 읽고 쓴다.
 * 로컬 플레이어가 만들어질 때 PADO Controls 설정의 입력 매핑 컨텍스트를 등록해,
 * 게임 화면에서는 저장한 키가 바로 적용되고 메인 메뉴에서도 키 목록을 볼 수 있게 한다.
 * 키보드·마우스 키만 다룬다. 게임패드 키는 게임패드 지원이 정해지면 추가한다.
 */
UCLASS()
class PADO_API UPDKeyBindingSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 사용자 설정을 쓸 수 있는지다. 프로젝트 설정의 Enhanced Input Enable User Settings가 꺼져 있으면 false다. */
	bool IsAvailable() const;

	/** 키 설정 탭에 보여 줄 매핑을 컨텍스트·매핑 순서대로 반환한다. */
	TArray<FPDKeyBindingEntry> GetKeyBindings() const;

	/** 매핑 이름별 새 키를 사용자 설정에 쓰고 적용·저장한다. 지금 키와 같은 항목은 건너뛴다. */
	void ApplyKeyBindings(const TMap<FName, FKey>& Keys);

private:
	UEnhancedInputUserSettings* GetUserSettings() const;
	void RegisterMappingContexts() const;

	/** 컨텍스트에 적힌 순서대로 바꿀 수 있는 매핑 이름을 모은다. */
	TArray<FName> GetOrderedMappingNames() const;

	/** 행에서 키보드·마우스 기본 키를 쓰는 매핑 중 슬롯이 가장 앞선 것을 찾는다. */
	static const FPlayerKeyMapping* FindKeyboardMapping(const FKeyMappingRow& Row);
	static bool IsKeyboardOrMouseButton(const FKey& Key);
};
