// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Common/Enum/PDConfirmDialogResult.h"

/** 공용 확인창에 보여 줄 내용과 닫힌 뒤 실행할 함수다. */
struct FPDConfirmDialogArgs
{
	/** 비어 있으면 제목 줄을 숨긴다. */
	FText Title;
	FText Body;
	FText ConfirmText;
	FText CancelText;

	/** 0보다 크면 이 시간(초)이 지난 뒤 취소로 닫는다. */
	int32 TimeoutSeconds = 0;

	/** 남은 시간 문구다. {Seconds}를 남은 초로 바꾼다. */
	FText TimeoutFormat;

	/** 확인창이 닫힌 뒤 한 번 호출한다. */
	TFunction<void(EPDConfirmDialogResult)> OnClosed;
};
