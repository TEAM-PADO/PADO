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

/**
 * 손을 쓸 수 없는 상태다. 붙어 있으면 Action 활성화, 재장전, 드롭, 줍기를
 * 거부한다. 무기를 든 동안 활성인 Fire Action은 끝내지 않는다. 탈것 좌석처럼
 * 이 상태를 만드는 쪽이 ASC에 루즈 태그로 붙이고 뗀다.
 */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_State_HandsBlocked);

/** 빈사 상태다. 체력이 0이 됐지만 아직 죽지 않았다. 손을 쓸 수 없다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_State_Downed);

/** 사망 상태다. 피해를 더 받지 않는다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_State_Dead);

/** 공용 Action 쿨다운 GE가 지속시간을 받는 SetByCaller 태그다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Data_Cooldown_Duration);

/** 이동 속도 배율 GE가 배율을 받는 SetByCaller 태그다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Data_MoveSpeed_Multiplier);

/** 피해 GE가 피해량을 받는 SetByCaller 태그다. */
PADO_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PD_Data_Damage);
