// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "CoreMinimal.h"
#include "Tasks/IAbleAbilityTask.h"
#include "SPReFindTargetTask.generated.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

/**
 * 
 */
UCLASS()
class FEATURE_SP_API USPReFindTargetTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;
	
public:
	USPReFindTargetTask(const FObjectInitializer& Initializer);

	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;

	virtual void OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskTick"))
	void OnTaskTickBP(const UAbleAbilityContext* Context, float deltaTime) const;

	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	                       const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context,
					 const EAbleAbilityTaskResult result) const;

	virtual bool IsSingleFrame() const override { return IsSingleFrameBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsSingleFrame"))
	bool IsSingleFrameBP() const;

	void FindTarget(const UAbleAbilityContext* Context) const;
	void FindTargetGroup(const UAbleAbilityContext* Context) const;

	virtual bool IsAsyncFriendly() const override { return false; }

	virtual void BindDynamicDelegates(UAbleAbility* Ability) override;

	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	virtual TStatId GetStatId() const override;

#if WITH_EDITOR

	virtual FText GetTaskCategory() const override { return LOCTEXT("USPReFindTargetTask", "Target"); }

	virtual FText GetTaskName() const override { return LOCTEXT("USPReFindTargetTask", "Find Target"); }

#endif

protected:
	UPROPERTY(EditAnywhere, Instanced, Category = "Targeting", meta = (DisplayName = "Target Logic", EditCondition = "!bUseGroup", EditConditionHides))
	UAbleTargetingBase* m_Targeting;

	UPROPERTY(EditAnywhere, Instanced, Category = "Targeting", meta = (DisplayName = "Target Group Logic", EditCondition = "bUseGroup", EditConditionHides))
	TArray<UAbleTargetingBase*> TargetingList;

	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (DisplayName = "多类型查找目标"))
	bool bUseGroup = false;
	
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (DisplayName = "IsSingleFrame"))
	bool m_IsSingleFrame = true;
};

#undef LOCTEXT_NAMESPACE
