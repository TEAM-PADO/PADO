// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PDSettingKeyBindingViewModel.generated.h"

/**
 * 동작 하나의 키보드·마우스 키를 바꾸는 옵션 줄이다.
 * 키 버튼을 누르면 OnKeyCaptureRequested를 보내고, 옵션 화면이 키 입력 대기 창을 띄운 뒤
 * 받은 키를 옵션 Viewmodel에 넘긴다(중복 키 확인은 옵션 Viewmodel이 한다).
 */
UCLASS(BlueprintType)
class PADO_API UPDSettingKeyBindingViewModel : public UPDSettingItemViewModel
{
	GENERATED_BODY()

public:
	/** Player Mappable Key Settings의 매핑 이름과 지금 키를 읽는 함수를 지정한다. */
	void SetMapping(FName InMappingName, TFunction<FKey()> InKeyGetter);

	/** 키 입력 대기를 요청한다. 편집할 수 없는 상태면 무시한다. */
	void RequestKeyCapture();

	FName GetMappingName() const { return MappingName; }
	const FKey& GetKey() const { return Key; }
	const FText& GetKeyText() const { return KeyText; }

	/** 키 입력 대기를 요청할 때 보낸다. */
	FPDSettingItemSignature OnKeyCaptureRequested;

protected:
	virtual void RefreshValue() override;

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText KeyText;

	FName MappingName;
	FKey Key;
	TFunction<FKey()> KeyGetter;
};
