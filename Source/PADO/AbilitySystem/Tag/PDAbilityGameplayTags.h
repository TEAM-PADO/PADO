#pragma once

#include "NativeGameplayTags.h"

// 행동의 어느 순간인가. 대상 종류는 ActionTargeting과 Fragment Scope가 정한다.
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_ActionHook_OnStart);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_ActionHook_OnExecuteStart);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_ActionHook_OnExecute);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_ActionHook_OnFirstHit);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_ActionHook_OnComplete);

PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_GameplayEvent_AbilitySource_Trigger);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_GameplayEvent_Action_Execute);
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Ability_Action);

/** 공용 Action 쿨다운 GE가 지속시간을 받는 SetByCaller 태그다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Data_Cooldown_Duration);

/** 이동 속도 배율 GE가 배율을 받는 SetByCaller 태그다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Data_MoveSpeed_Multiplier);
