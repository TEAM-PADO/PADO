// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "MassProcessor.h"
#include "PDZombieMoveToTargetProcessor.generated.h"

/**
 * 
 */

class ATargetPoint;

UCLASS()
class PADO_API UPDZombieMoveToTargetProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UPDZombieMoveToTargetProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
private:
	FMassEntityQuery EntityQuery;
	TWeakObjectPtr<ATargetPoint> TargetPoint;
	
};
