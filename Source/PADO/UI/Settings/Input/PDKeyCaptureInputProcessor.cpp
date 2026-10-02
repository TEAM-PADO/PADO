// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Input/PDKeyCaptureInputProcessor.h"

#include "Input/Events.h"

bool FPDKeyCaptureInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	HandlePressed(InKeyEvent.GetKey());
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	HandleReleased(InKeyEvent.GetKey());
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	// 스틱·트리거 입력이 메뉴 초점을 옮기지 않게 막는다.
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	HandlePressed(MouseEvent.GetEffectingButton());
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	HandleReleased(MouseEvent.GetEffectingButton());
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	HandlePressed(MouseEvent.GetEffectingButton());
	return true;
}

bool FPDKeyCaptureInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent)
{
	// 휠은 떼는 동작이 없으므로 굴리는 즉시 확정한다.
	if (!IsFinished() && !FMath::IsNearlyZero(InWheelEvent.GetWheelDelta()))
	{
		CapturedKey = InWheelEvent.GetWheelDelta() > 0.0f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown;
	}

	return true;
}

void FPDKeyCaptureInputProcessor::HandlePressed(const FKey& Key)
{
	if (!IsFinished() && Key.IsValid())
	{
		PressedKeys.Add(Key);
	}
}

void FPDKeyCaptureInputProcessor::HandleReleased(const FKey& Key)
{
	// 대기 전에 누르고 있던 키를 떼는 입력은 무시한다.
	if (IsFinished() || !PressedKeys.Contains(Key))
	{
		return;
	}

	if (Key == EKeys::Escape)
	{
		bIsCancelled = true;
		return;
	}

	// 게임패드 키는 게임패드 지원이 정해질 때까지 받지 않는다.
	if (Key.IsGamepadKey() || Key.IsTouch())
	{
		return;
	}

	CapturedKey = Key;
}
