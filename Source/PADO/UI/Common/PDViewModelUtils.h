// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMVVMViewModelBase;
class UUserWidget;

/**
 * C++이 만든 Viewmodel을 WBP의 MVVM View에 연결하는 공용 도우미다.
 * WBP의 Viewmodels 패널에 같은 클래스를 Creation Type "Manual"로 등록해 두어야 연결된다.
 */
namespace PDViewModelUtils
{
	/** 위젯의 MVVM View에 Viewmodel을 클래스 기준으로 연결한다. 연결하지 못하면 경고를 남기고 false를 반환한다. */
	PADO_API bool SetViewModel(UUserWidget* Widget, UMVVMViewModelBase* ViewModel);
}
