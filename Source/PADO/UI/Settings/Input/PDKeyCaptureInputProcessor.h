// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

/**
 * 키 입력 대기 중 다음 키보드·마우스 입력 하나를 가로챈다.
 * CommonUI 입력 처리보다 먼저 받도록 가장 앞 순서로 등록해, Esc나 탭 넘김 키가 옵션 창에 전달되지 않게 한다.
 * 누름은 막기만 하고 뗄 때 키를 확정한다. 대기 전에 누르고 있던 키(버튼을 누른 Enter 등)는 무시한다.
 * Esc는 취소로, 게임패드 입력은 무시한다. 결과는 위젯이 다음 Tick에 읽어 간다.
 */
class FPDKeyCaptureInputProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("PDKeyCapture"); }

	/** 확정한 키다. 아직 없으면 비어 있다. */
	const TOptional<FKey>& GetCapturedKey() const { return CapturedKey; }

	bool IsCancelled() const { return bIsCancelled; }

private:
	void HandlePressed(const FKey& Key);
	void HandleReleased(const FKey& Key);
	bool IsFinished() const { return CapturedKey.IsSet() || bIsCancelled; }

	TSet<FKey> PressedKeys;
	TOptional<FKey> CapturedKey;
	bool bIsCancelled = false;
};
