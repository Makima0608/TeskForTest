// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Game/SPGame/Skill/Task/SPReFindTargetTask.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"

#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

USPReFindTargetTask::USPReFindTargetTask(const FObjectInitializer& Initializer)
	: Super(Initializer), m_Targeting(nullptr)
{
}

FString USPReFindTargetTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPReFindTargetTask");
}

void USPReFindTargetTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void USPReFindTargetTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{
	if (bUseGroup)
	{
		FindTargetGroup(Context);
	}
	else
	{
		FindTarget(Context);
	}
}

void USPReFindTargetTask::OnTaskTick(const TWeakObjectPtr<const UAbleAbilityContext>& Context, float deltaTime) const
{
	Super::OnTaskTick(Context, deltaTime);
	OnTaskTickBP(Context.Get(), deltaTime);
}

void USPReFindTargetTask::OnTaskTickBP_Implementation(const UAbleAbilityContext* Context, float deltaTime) const
{
	if (bUseGroup)
	{
		FindTargetGroup(Context);
	}
	else
	{
		FindTarget(Context);
	}
}

void USPReFindTargetTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
                                    const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void USPReFindTargetTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context,
                                                     const EAbleAbilityTaskResult result) const
{
}

void USPReFindTargetTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);
}

TStatId USPReFindTargetTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USPReFindTargetTask, STATGROUP_USPAbility);
}

bool USPReFindTargetTask::IsSingleFrameBP_Implementation() const
{
	return m_IsSingleFrame;
}

void USPReFindTargetTask::FindTarget(const UAbleAbilityContext* Context) const
{
	UAbleAbilityContext* AbilityContext = const_cast<UAbleAbilityContext*>(Context);
	if (!AbilityContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("USPReFindTargetTask::FindTarget Failed, Invalid Context !"))
			return;
	}

	if(m_Targeting)
	{
		if (m_Targeting->ShouldClearTargets())
		{
			AbilityContext->ClearTargetActors();
		}
		m_Targeting->FindTargets(*AbilityContext);
	}
}

void USPReFindTargetTask::FindTargetGroup(const UAbleAbilityContext* Context) const
{
	UAbleAbilityContext* AbilityContext = const_cast<UAbleAbilityContext*>(Context);
	if (!AbilityContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("USPReFindTargetTask::FindTargetGroup Failed, Invalid Context !"))
		return;
	}
	
	TArray<AActor*> ResultTargetActors;
	for (const UAbleTargetingBase* TargetingRule : TargetingList)
	{
		if (TargetingRule)
		{
			//查找过滤，记录第一个对象
			if (TargetingRule->ShouldClearTargets())
			{
				AbilityContext->ClearTargetActors();
			}
			TargetingRule->FindTargets(*AbilityContext);
			AActor* ResultActor = GetSingleActorFromTargetType(Context, EAbleAbilityTargetType::ATT_TargetActor, 0);
			ResultTargetActors.Add(ResultActor);
		}
	}

	//最终结果存入TargetActors（结果与TargetingRule一一对应，结果存在为nullptr元素）
	auto TargetActorsPtr = Context->GetTargetActorsWeakPtr();
	TargetActorsPtr.Empty();
	TargetActorsPtr.Append(ResultTargetActors);
}

EAbleAbilityTaskRealm USPReFindTargetTask::GetTaskRealmBP_Implementation() const
{
	return EAbleAbilityTaskRealm::ATR_ClientAndServer;
}
