// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Tasks/IAbleAbilityTask.h"
#include "Tasks/ableTurnToTask.h"
#include "SPTurnToTask.generated.h"


#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"
/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPTurnToTask : public UAbleTurnToTask
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:

	USPTurnToTask(const FObjectInitializer& ObjectInitializer);
	virtual ~USPTurnToTask() override;
	
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;
	UFUNCTION(BlueprintNativeEvent)
	void OnTaskStartBP_Override(const UAbleAbilityContext* Context) const;

	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,const EAbleAbilityTaskResult result) const override;
	UFUNCTION(BlueprintNativeEvent)
	void OnTaskEndBP_Override(const UAbleAbilityContext* Context,const EAbleAbilityTaskResult result) const;

	virtual void TurnToSetActorRotation(const TWeakObjectPtr<AActor> TargetActor, const FRotator& TargetRotation) const override;
	
	virtual void TurnAnimation(const TWeakObjectPtr<AActor> TargetActor, const float DeltaYaw) const override;
	
	
	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "ABP Anim State"))
	bool m_MoveAnimState;

	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "RotateByMasterFaceTo"))
	bool m_RotateByMasterFaceTo;

	UPROPERTY(EditAnywhere, Category = "Turn To", meta = (DisplayName = "bTurnto"))
	bool m_bTurnto;
#if WITH_EDITOR

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPTurnToTask", "Movement"); }

	virtual FText GetTaskName() const override { return LOCTEXT("USPTurnToTask", "SP Turn To"); }

#endif

protected:
	virtual FRotator GetTargetRotation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const override;
	FRotator GetTargetRotationByMasterFaceTo(const AActor* Source) const;
};

#undef LOCTEXT_NAMESPACE


