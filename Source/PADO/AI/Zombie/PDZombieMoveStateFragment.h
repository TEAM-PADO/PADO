// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityElementTypes.h"
#include "PDZombieMoveStateFragment.generated.h"

UENUM()
enum class EPDZombieMoveState : uint8
{
	Idle,
	Move
};

USTRUCT()
struct FPDZombieMoveStateFragment : public FMassFragment
{
	GENERATED_BODY()

	EPDZombieMoveState State = EPDZombieMoveState::Idle;
};
