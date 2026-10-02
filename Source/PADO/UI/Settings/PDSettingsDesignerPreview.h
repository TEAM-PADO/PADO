// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UPDSettingsViewModel;

#if WITH_EDITOR
/**
 * 위젯 디자이너에서 옵션 창을 미리 보여 주는 데이터다. 에디터의 디자이너에서만 쓴다.
 * 실행 중인 설정 객체·설정 파일·엔진 화면은 건드리지 않는다.
 */
namespace PDSettingsDesignerPreview
{
	/** 기본값과 예시 키 설정으로 채운 옵션 상태를 만든다. */
	PADO_API UPDSettingsViewModel* CreateViewModel(UObject* Outer);
}
#endif
