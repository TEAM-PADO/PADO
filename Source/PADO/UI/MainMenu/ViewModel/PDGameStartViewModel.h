// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PDGameStartViewModel.generated.h"

/**
 * 게임 시작(저장 데이터 선택) 화면의 상태다.
 * 저장 데이터 목록·이어하기·삭제는 네트워크 코어의 저장 계약이 정해진 뒤 이 Viewmodel에 추가한다.
 * 그 전까지는 저장 데이터가 없는 상태만 표시한다.
 */
UCLASS(BlueprintType)
class PADO_API UPDGameStartViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	bool HasSaveData() const { return bHasSaveData; }

private:
	/** 표시할 저장 데이터가 있을 때 true다. 빈 목록 안내 문구의 표시 여부에 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|GameStart", meta = (AllowPrivateAccess = "true"))
	bool bHasSaveData = false;
};
