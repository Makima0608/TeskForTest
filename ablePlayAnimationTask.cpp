// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "Tasks/ablePlayAnimationTask.h"

#include "ableAbility.h"
#include "ableAbilityBlueprintLibrary.h"
#include "ableAbilityComponent.h"
#include "ableSubSystem.h"
#include "AbleCoreSPPrivate.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNode_SPAbilityAnimPlayer.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimStateMachineTypes.h"
#include "Animation/AnimNode_StateMachine.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

UAblePlayAnimationTaskScratchPad::UAblePlayAnimationTaskScratchPad()
{

}

UAblePlayAnimationTaskScratchPad::~UAblePlayAnimationTaskScratchPad()
{

}

UAblePlayAnimationTask::UAblePlayAnimationTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	m_AnimationAsset(nullptr),
	m_AnimationMontageSection(NAME_None),
	m_OnEndAnimationMontageSection(NAME_None),
	m_AnimationMode(EAblePlayAnimationTaskAnimMode::DynamicMontage),
	m_StateMachineName(),
	m_AbilityStateName(),
	m_Loop(false),
	m_SlotName(TEXT("DefaultSlot")),
	m_TimeToStartMontageAt(0.0f),
	m_BlendOutTriggerTime(-1.0f),
	m_NumberOfLoops(1),
	m_StopAllMontages(false),
	m_PlayRate(1.0f),
	m_ScaleWithAbilityPlayRate(true),
	m_StopAnimationOnInterrupt(true),
	m_ClearQueuedAnimationOnInterrupt(false),
	m_ResetAnimationStateOnEnd(false),
	m_ManuallySpecifyAnimationLength(false),
	m_ManualLengthIsInterrupt(true),
	m_EventName(NAME_None),
	m_PlayOnServer(false),
	m_OverrideVisibilityBasedAnimTick(false)
{

}

UAblePlayAnimationTask::~UAblePlayAnimationTask()
{

}

FString UAblePlayAnimationTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.AblePlayAnimationTask");
}

void UAblePlayAnimationTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP(Context.Get());
}

void UAblePlayAnimationTask::OnTaskStartBP_Implementation(const UAbleAbilityContext* Context) const
{

	const UAnimationAsset* AnimationAsset = m_AnimationAsset.LoadSynchronous();

	if (!AnimationAsset)
	{
		UE_LOG(LogAbleSP, Warning, TEXT("No Animation set for PlayAnimationTask in Ability [%s]"), *Context->GetAbility()->GetAbilityName());
		return;
	}

	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);

	UAblePlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblePlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	ScratchPad->AbilityComponents.Empty();
	ScratchPad->SingleNodeSkeletalComponents.Empty();
	ScratchPad->CachedVisibilityBasedAnimTickOptionMap.Empty();

	float BasePlayRate = m_PlayRate;
	float PlayRate = BasePlayRate * (m_ScaleWithAbilityPlayRate ? Context->GetAbility()->GetPlayRate(Context) : 1.0f);
	FName MontageSection = m_AnimationMontageSection;

	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (Target.IsValid())
		{
			if (USkeletalMeshComponent* PreferredComponent = Context->GetAbility()->GetSkeletalMeshComponentForActor(Context, Target.Get(), m_EventName))
			{
				PlayAnimation(Context, AnimationAsset, MontageSection, *Target.Get(), *ScratchPad, *PreferredComponent, PlayRate);
			}
			else
			{
				TInlineComponentArray<USkeletalMeshComponent*> InSkeletalComponents(Target.Get());

				for (UActorComponent* SkeletalComponent : InSkeletalComponents)
				{
					PlayAnimation(Context, AnimationAsset, MontageSection, *Target.Get(), *ScratchPad, *Cast<USkeletalMeshComponent>(SkeletalComponent), PlayRate);
				}
			}
			
			if (m_OverrideVisibilityBasedAnimTick)
			{
				// Cached origin VisibilityBasesAnimTickOption for restore when animation ended.
				if (USkeletalMeshComponent* TargetMesh = Target.Get()->FindComponentByClass<USkeletalMeshComponent>())
				{
					ScratchPad->CachedVisibilityBasedAnimTickOptionMap.Add(TargetMesh,TargetMesh->VisibilityBasedAnimTickOption);
					TargetMesh->VisibilityBasedAnimTickOption = m_VisibilityBasedAnimTick;
					if (UKismetSystemLibrary::IsDedicatedServer(this))
					{
						UAbleAbilityBlueprintLibrary::SetComponentTickEnableImplicitRef(TEXT("DynamicSetAnimationTickable"), TargetMesh, Context);
					}
				}
			}
		}
	}
}

void UAblePlayAnimationTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP(Context.Get(), result);
}

void UAblePlayAnimationTask::OnTaskEndBP_Implementation(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const
{

	if (!Context)
	{
		return;
	}

	UAblePlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblePlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	
	// Reset any Single Node instances that were previous AnimBlueprint mode.
	for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
	{
		if(SkeletalComponent == nullptr)
			continue;
		if (m_ResetAnimationStateOnEnd)
		{
			SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}

		// If we're in Animation Node mode, and our Ability was interrupted - tell the Animation Node.
		if (m_StopAnimationOnInterrupt && (
			(result == EAbleAbilityTaskResult::Interrupted || (result == EAbleAbilityTaskResult::BranchSegment && bStopAnimationOnBranch) || result == EAbleAbilityTaskResult::StopAcross) ||
			(m_ManuallySpecifyAnimationLength && m_ManualLengthIsInterrupt) ) )
		{
			switch (m_AnimationMode)
			{
			case EAblePlayAnimationTaskAnimMode::AbilityAnimationNode:
			{
				if (FAnimNode_SPAbilityAnimPlayer* AnimPlayer = GetAbilityAnimGraphNode(Context, SkeletalComponent.Get()))
				{
					AnimPlayer->OnAbilityInterrupted(m_ClearQueuedAnimationOnInterrupt);
				}
			}
			break;
			case EAblePlayAnimationTaskAnimMode::DynamicMontage:
			{
				if (UAnimInstance* Instance = SkeletalComponent->GetAnimInstance())
				{
					FName MontageOnEnd = m_OnEndAnimationMontageSection;
					if (MontageOnEnd != NAME_None)
					{
						Instance->Montage_JumpToSection(MontageOnEnd);
					}
					else
					{
						Instance->Montage_Stop(m_DynamicMontageBlend.m_BlendOut);
					}
				}
			}
			break;
			case EAblePlayAnimationTaskAnimMode::SingleNode:
			{
				SkeletalComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
				if (UAnimSingleNodeInstance* SingleNode = SkeletalComponent->GetSingleNodeInstance())
				{
					SingleNode->StopAnim();
				}
			}
			break;
			default:
				checkNoEntry();
				break;
			}

		}

		if (m_OverrideVisibilityBasedAnimTick && !ScratchPad->CachedVisibilityBasedAnimTickOptionMap.IsEmpty())
		{
			for (auto MeshIt = ScratchPad->CachedVisibilityBasedAnimTickOptionMap.CreateIterator(); MeshIt;++MeshIt)
			{
				if (MeshIt.Key().IsValid())
				{
					MeshIt.Key()->VisibilityBasedAnimTickOption = MeshIt.Value();
					if (UKismetSystemLibrary::IsDedicatedServer(this))
					{
						UAbleAbilityBlueprintLibrary::SetComponentTickDisableImplicitRef(TEXT("DynamicSetAnimationTickable"), MeshIt.Key().Get(), Context);
					}
				}
			}
		}
	}
}


bool UAblePlayAnimationTask::IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	return Super::IsDone(Context);
}

bool UAblePlayAnimationTask::IsDoneBP_Implementation(const UAbleAbilityContext* Context) const
{
	float PlayRate = m_PlayRate;
	return Context->GetCurrentTime() * PlayRate > GetEndTime();
}

float UAblePlayAnimationTask::GetEndTime() const
{
	if (m_ManuallySpecifyAnimationLength)
	{
		return Super::GetEndTime();
	}
	const UAnimationAsset* AnimationAsset = m_AnimationAsset.LoadSynchronous();
	float PlayRate = m_PlayRate; // Assume a flat playrate. 
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(AnimationAsset))
	{
        if (m_AnimationMontageSection != NAME_None)
        {
            int32 sectionIndex = Montage->GetSectionIndex(m_AnimationMontageSection);
            if (sectionIndex != INDEX_NONE)
            {
                const FCompositeSection& section = Montage->CompositeSections[sectionIndex];
                float sequenceLength = (Montage->GetSectionLength(sectionIndex) * (1.0f / PlayRate));
                
                float endTime = GetStartTime() + sequenceLength;
                return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
            }
        }

        float endTime = GetStartTime() + (const_cast<UAnimMontage*>(Montage)->GetPlayLength() * (1.0f / PlayRate));
        return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
	}
    else if (const UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(AnimationAsset))
    {
        float endTime = GetStartTime() + (const_cast<UAnimSequenceBase*>(Sequence)->GetPlayLength() * (1.0f / PlayRate));
        return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
    }
    
    // fallback
    float endTime = GetStartTime() + 1.0f;
    return m_Loop ? FMath::Max(Super::GetEndTime(), endTime) : endTime;
}

UAbleAbilityTaskScratchPad* UAblePlayAnimationTask::CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const
{
	if (UAbleAbilityUtilitySubsystem* Subsystem = Context->GetUtilitySubsystem())
	{
		static TSubclassOf<UAbleAbilityTaskScratchPad> ScratchPadClass = UAblePlayAnimationTaskScratchPad::StaticClass();
		return Subsystem->FindOrConstructTaskScratchPad(ScratchPadClass);
	}

	return NewObject<UAblePlayAnimationTaskScratchPad>(Context.Get());
}

TStatId UAblePlayAnimationTask::GetStatId() const
{
	 RETURN_QUICK_DECLARE_CYCLE_STAT(UAblePlayAnimationTask, STATGROUP_Able);
}

void UAblePlayAnimationTask::SetAnimationAsset(UAnimationAsset* Animation)
{
	check(Animation->IsA<UAnimSequenceBase>() || Animation->IsA<UAnimMontage>());
	m_AnimationAsset = Animation;
}

void UAblePlayAnimationTask::OnAbilityPlayRateChanged(const UAbleAbilityContext* Context, float NewPlayRate)
{
	Super::OnAbilityPlayRateChanged(Context, NewPlayRate);
	if (!Context)
	{
		return;
	}
	TArray<TWeakObjectPtr<AActor>> TargetArray;
	GetActorsForTask(Context, TargetArray);
	for (TWeakObjectPtr<AActor>& Target : TargetArray)
	{
		if (Target.IsValid())
		{
			if (USkeletalMeshComponent* PreferredComponent = Context->GetAbility()->GetSkeletalMeshComponentForActor(Context, Target.Get(), m_EventName))
			{
				SetAnimationPlayRateInRunTime(Context, *PreferredComponent, NewPlayRate);
			}
			else
			{
				TInlineComponentArray<USkeletalMeshComponent*> InSkeletalComponents(Target.Get());

				for (USkeletalMeshComponent* SkeletalComponent : InSkeletalComponents)
				{
					SetAnimationPlayRateInRunTime(Context, *SkeletalComponent, NewPlayRate);
				}
			}
		}
	}
}

void UAblePlayAnimationTask::PlayAnimation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const UAnimationAsset* AnimationAsset, const FName& MontageSection, AActor& TargetActor, UAblePlayAnimationTaskScratchPad& ScratchPad, USkeletalMeshComponent& SkeletalMeshComponent, float PlayRate) const
{
	switch (m_AnimationMode.GetValue())
	{
		case EAblePlayAnimationTaskAnimMode::SingleNode:
		{
			// Make a note of these so we can reset to Animation Blueprint mode.
			if (SkeletalMeshComponent.GetAnimationMode() == EAnimationMode::AnimationBlueprint)
			{
				ScratchPad.SingleNodeSkeletalComponents.Add(&SkeletalMeshComponent);
			}

			if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
			{
				if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Montage %s (Section %s) on Target %s"), 
                            *MontageAsset->GetName(), *MontageSection.ToString(), *TargetActor.GetName()));
					}
#endif
					float StartMontageAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TimeToStartMontageAt);

					ScratchPad.CurrentAnimMontage = const_cast<UAnimMontage*>(MontageAsset);
					Instance->Montage_Play(ScratchPad.CurrentAnimMontage.Get(), PlayRate, EMontagePlayReturnType::MontageLength, StartMontageAt, ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StopAllMontages));

                    if (MontageSection != NAME_None)
                    {
#if !(UE_BUILD_SHIPPING)
                        if (!MontageAsset->IsValidSectionName(MontageSection))
                        {
                            PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Montage %s (Section %s) on Target %s Invalid Montage Section"),
                                *MontageAsset->GetName(), *MontageSection.ToString(), *TargetActor.GetName()));
                        }
#endif

                        Instance->Montage_JumpToSection(MontageSection, MontageAsset);
                    }
				}
			}
			else if (UAnimSingleNodeInstance* SingleNode = SkeletalMeshComponent.GetSingleNodeInstance()) // See if we already have an instance instantiated.
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Animation %s on Target %s"), *AnimationAsset->GetName(), *TargetActor.GetName()));
				}
#endif
				SingleNode->SetAnimationAsset(const_cast<UAnimationAsset*>(AnimationAsset), m_Loop, PlayRate);
				SingleNode->PlayAnim(m_Loop);
			}
			else // Nope, start a new one.
			{
#if !(UE_BUILD_SHIPPING)
				if (IsVerbose())
				{
					PrintVerbose(Context, FString::Printf(TEXT("Playing Single Node Animation %s on Target %s"), *AnimationAsset->GetName(), *TargetActor.GetName()));
				}
#endif
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationSingleNode);
				SkeletalMeshComponent.SetAnimation(const_cast<UAnimationAsset*>(AnimationAsset));
				SkeletalMeshComponent.SetPlayRate(PlayRate);
				SkeletalMeshComponent.Play(m_Loop);
			}
		}
		break;
		case EAblePlayAnimationTaskAnimMode::AbilityAnimationNode:
		{
			if (SkeletalMeshComponent.GetAnimationMode() != EAnimationMode::AnimationBlueprint)
			{
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationBlueprint);
			}
			
			if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
			{
				const UWorld* World= GetWorld();
				if (World && World->IsGameWorld())
				{
					const UAnimSequence* AnimationSequence = Cast<UAnimSequence>(AnimationAsset);
					if (AnimationSequence)
					{
						if (FAnimNode_SPAbilityAnimPlayer* AbilityPlayerNode = GetAbilityAnimGraphNode(Context, &SkeletalMeshComponent))
						{
#if !(UE_BUILD_SHIPPING)
							if (IsVerbose())
							{
								PrintVerbose(Context, FString::Printf(TEXT("Playing Animation %s on Target %s using Ability Animation Node"), *AnimationAsset->GetName(), *TargetActor.GetName()));
							}
#endif
							AbilityPlayerNode->PlayAnimationSequence(AnimationSequence, PlayRate, m_BlendIn, m_BlendOut);

							if (UAbleAbilityComponent* AbilityComponent = TargetActor.FindComponentByClass<UAbleAbilityComponent>())
							{
								ScratchPad.AbilityComponents.Add(AbilityComponent);

								AbilityComponent->SetAbilityAnimationNode(AbilityPlayerNode);
							}
						}
						else
						{
							UE_LOG(LogAbleSP, Warning, TEXT("Failed to find Ability Animation Node using State Machine Name %s and Node Name %s"), *m_StateMachineName.ToString(), *m_AbilityStateName.ToString());
						}
					}
					else
					{
						UE_LOG(LogAbleSP, Warning, TEXT("Ability Animation Node only supports Animation Sequences. Unable to play %s"), *AnimationAsset->GetName());
					}
				}
				else
				{
					// Use play animation single node instead since no matter what, this timing is just for preview.
					SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationSingleNode);
					SkeletalMeshComponent.SetAnimation(const_cast<UAnimationAsset*>(AnimationAsset));
					SkeletalMeshComponent.SetPlayRate(PlayRate);
					SkeletalMeshComponent.Play(m_Loop);
				}
			}
		}
		break;
		case EAblePlayAnimationTaskAnimMode::DynamicMontage:
		{
			if (SkeletalMeshComponent.GetAnimationMode() != EAnimationMode::AnimationBlueprint)
			{
				SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationBlueprint);
			}

			if (UAnimInstance* Instance = SkeletalMeshComponent.GetAnimInstance())
			{
				float StartMontageAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_TimeToStartMontageAt);
				float BlendOutTimeAt = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_BlendOutTriggerTime);
				int32 NumLoops = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_NumberOfLoops);
				FName SlotName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_SlotName);

				if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Montage Animation %s on Target %s using Dynamic Montage"), *MontageAsset->GetName(), *TargetActor.GetName()));
					}
#endif
					ScratchPad.CurrentAnimMontage = const_cast<UAnimMontage*>(MontageAsset);
					Instance->Montage_Play(ScratchPad.CurrentAnimMontage.Get(), PlayRate, EMontagePlayReturnType::MontageLength, StartMontageAt, ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StopAllMontages));
				}
				else if (const UAnimSequenceBase* SequenceAsset = Cast<UAnimSequenceBase>(AnimationAsset))
				{
#if !(UE_BUILD_SHIPPING)
					if (IsVerbose())
					{
						PrintVerbose(Context, FString::Printf(TEXT("Playing Slot Animation %s on Target %s on Slot %s using Dynamic Montage"), *SequenceAsset->GetName(), *TargetActor.GetName(), *SlotName.ToString()));
					}
#endif
					
					const UWorld* World= GetWorld();
					if (World && World->IsGameWorld())
					{
						//loop次数默认值为1
						if (m_Loop && NumLoops <= 1)
						{
							//使用蒙太奇播放不会使用m_Loop，需要使用loop次数才能保持loop，动画会在TaskEnd结束时结束
							NumLoops = INT_MAX;
						}
						const FAbleBlendTimes DynamicMontageBlend = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_DynamicMontageBlend);
						ScratchPad.CurrentAnimMontage = PlayMontageBySequence(Instance, const_cast<UAnimSequenceBase*>(SequenceAsset), SlotName, DynamicMontageBlend.m_BlendIn, DynamicMontageBlend.m_BlendOut, PlayRate, NumLoops, BlendOutTimeAt, StartMontageAt, ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StopAllMontages));
						// ScratchPad.CurrentAnimMontage = Instance->PlaySlotAnimationAsDynamicMontage(const_cast<UAnimSequenceBase*>(SequenceAsset), SlotName, DynamicMontageBlend.m_BlendIn, DynamicMontageBlend.m_BlendOut, PlayRate, NumLoops, BlendOutTimeAt, StartMontageAt);
					}
					else
					{
						// Use play animation single node instead since PlaySlotAnimationAsDynamicMontage() does not play animation in editor preview.
						SkeletalMeshComponent.SetAnimationMode(EAnimationMode::AnimationSingleNode);
						SkeletalMeshComponent.SetAnimation(const_cast<UAnimationAsset*>(AnimationAsset));
						SkeletalMeshComponent.SetPlayRate(PlayRate);
						SkeletalMeshComponent.Play(m_Loop);
					}
				}
			}
		}
		break;
		default:
		{
			checkNoEntry();
		}
		break;
	}

	if (m_StopAnimationOnInterrupt)
	{
		ScratchPad.SingleNodeSkeletalComponents.Add(&SkeletalMeshComponent);
	}
}

void UAblePlayAnimationTask::SetAnimationPlayRateInRunTime(const UAbleAbilityContext* Context,
	const USkeletalMeshComponent& MeshComp, float NewPlayRate) const
{
	UAblePlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblePlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;
	
	switch (m_AnimationMode.GetValue())
	{
		case EAblePlayAnimationTaskAnimMode::SingleNode:
		{
			if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(GetAnimationAsset()))
			{
				if (UAnimInstance* AnimInstance = MeshComp.GetAnimInstance())
				{
					if(AnimInstance->GetCurrentActiveMontage() == MontageAsset)
					{
						AnimInstance->Montage_SetPlayRate(MontageAsset, NewPlayRate);
					}
				}
			}
			else
			{
				if (UAnimSingleNodeInstance* SingleNode = MeshComp.GetSingleNodeInstance())
				{
					SingleNode->SetPlayRate(NewPlayRate);
				}
			}
			break;
		}
		case EAblePlayAnimationTaskAnimMode::DynamicMontage:
		{
			if (UAnimInstance* AnimInstance = MeshComp.GetAnimInstance())
			{
				if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(GetAnimationAsset()))
				{
					if(AnimInstance->GetCurrentActiveMontage() == MontageAsset)
					{
						AnimInstance->Montage_SetPlayRate(MontageAsset, NewPlayRate);
					}
				}
				else
				{
					if (ScratchPad->CurrentAnimMontage.IsValid())
					{
						if(AnimInstance->GetCurrentActiveMontage() == ScratchPad->CurrentAnimMontage)
						{
							AnimInstance->Montage_SetPlayRate(ScratchPad->CurrentAnimMontage.Get(), NewPlayRate);
						}
					}
					else
					{
						//Editor技能预览模式
						if (UAnimSingleNodeInstance* SingleNode = MeshComp.GetSingleNodeInstance())
						{
							SingleNode->SetPlayRate(NewPlayRate);
						}
					}
				}
			}
			break;
		}
	default: ;
	}
}

UAnimMontage* UAblePlayAnimationTask::PlayMontageBySequence(UAnimInstance* Instance, UAnimSequenceBase* Asset, FName SlotNodeName,
	float BlendInTime, float BlendOutTime, float InPlayRate, int32 LoopCount, float BlendOutTriggerTime,
	float InTimeToStartMontageAt, bool bStopAllMontages, float Weight) const
{
	if (Asset && Instance)
	{
		return Instance->PlaySlotAnimationAsDynamicMontage(Asset, SlotNodeName, BlendInTime, BlendOutTime, InPlayRate, LoopCount, BlendOutTriggerTime, InTimeToStartMontageAt);
	}
	return nullptr;
}

FAnimNode_SPAbilityAnimPlayer* UAblePlayAnimationTask::GetAbilityAnimGraphNode(const TWeakObjectPtr<const UAbleAbilityContext>& Context, USkeletalMeshComponent* MeshComponent) const
{
	if (UAnimInstance* Instance = MeshComponent->GetAnimInstance())
	{
		FAnimInstanceProxy InstanceProxy(Instance);
		FName StateMachineName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_StateMachineName);
		FName AbilityStateName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AbilityStateName);

		FAnimNode_StateMachine* StateMachineNode = InstanceProxy.GetStateMachineInstanceFromName(StateMachineName);
		if (StateMachineNode)
		{
			const FBakedAnimationStateMachine* BakedStateMachine = InstanceProxy.GetMachineDescription(InstanceProxy.GetAnimClassInterface(), StateMachineNode);

			if (BakedStateMachine)
			{
				for (const FBakedAnimationState& State : BakedStateMachine->States)
				{
					if (State.StateName == AbilityStateName)
					{
						for (const int32& PlayerNodeIndex : State.PlayerNodeIndices)
						{
							FAnimNode_SPAbilityAnimPlayer* AbilityPlayerNode = (FAnimNode_SPAbilityAnimPlayer*)InstanceProxy.GetNodeFromIndexUntyped(PlayerNodeIndex, FAnimNode_SPAbilityAnimPlayer::StaticStruct());
							if (AbilityPlayerNode)
							{
								return AbilityPlayerNode;
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

void UAblePlayAnimationTask::OnAbilityTimeSet(const TWeakObjectPtr<const UAbleAbilityContext>& Context)
{
	UAblePlayAnimationTaskScratchPad* ScratchPad = CastChecked<UAblePlayAnimationTaskScratchPad>(Context->GetScratchPadForTask(this));
	if (!ScratchPad) return;

	const UAnimationAsset* AnimationAsset = m_AnimationAsset.LoadSynchronous();
	switch (m_AnimationMode.GetValue())
	{
	case EAblePlayAnimationTaskAnimMode::AbilityAnimationNode:
	{
		// Reset any Single Node instances that were previous AnimBlueprint mode.
		for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
		{
			if (FAnimNode_SPAbilityAnimPlayer* AbilityNode = GetAbilityAnimGraphNode(Context, SkeletalComponent.Get()))
			{
				AbilityNode->SetAnimationTime(Context->GetCurrentTime() - GetStartTime());
			}
		}
	}
	case EAblePlayAnimationTaskAnimMode::SingleNode:
	{
		for (TWeakObjectPtr<USkeletalMeshComponent>& SkeletalComponent : ScratchPad->SingleNodeSkeletalComponents)
		{
            if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
            {
                if (UAnimInstance* Instance = SkeletalComponent->GetAnimInstance())
                {
                    if (m_AnimationMontageSection != NAME_None)
                    {
						FName MontageName = ABL_GET_DYNAMIC_PROPERTY_VALUE(Context, m_AnimationMontageSection);
                        FAnimMontageInstance* MontageInstance = Instance->GetActiveInstanceForMontage(MontageAsset);
                        if (MontageInstance)
                        {
                            bool const bEndOfSection = (MontageInstance->GetPlayRate() < 0.f);
                            if (MontageInstance->JumpToSectionName(MontageName, bEndOfSection))
                            {
								const int32 SectionID = MontageAsset->GetSectionIndex(MontageName);

                                if (MontageAsset->IsValidSectionIndex(SectionID))
                                {
                                    FCompositeSection & CurSection = const_cast<UAnimMontage*>(MontageAsset)->GetAnimCompositeSection(SectionID);
                                    MontageInstance->SetPosition(CurSection.GetTime() + Context->GetCurrentTime() - GetStartTime());
                                }
                            }
                        }
                    }
                    else
                    {
                        Instance->Montage_SetPosition(MontageAsset, Context->GetCurrentTime() - GetStartTime());
                    }
                }
            }
            else
            {
                if (UAnimSingleNodeInstance* SingleNode = SkeletalComponent->GetSingleNodeInstance())
                {
                    SingleNode->SetPosition(Context->GetCurrentTime() - GetStartTime());
                }
            }
		}
	}
	break;
	default:
		break;
	}
}

void UAblePlayAnimationTask::BindDynamicDelegates(UAbleAbility* Ability)
{
	Super::BindDynamicDelegates(Ability);

	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AnimationAsset, TEXT("Animation"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_DynamicMontageBlend, TEXT("Play Blend"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_PlayRate, TEXT("Play Rate"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_TimeToStartMontageAt, TEXT("Time To Start At"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_BlendOutTriggerTime, TEXT("Blend Out Trigger Time"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_NumberOfLoops, TEXT("Number Of Loops"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_StopAllMontages, TEXT("Stop All Montages"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_SlotName, TEXT("Slot Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_StateMachineName, TEXT("State Machine Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AbilityStateName, TEXT("Ability State Name"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_AnimationMontageSection, TEXT("Montage Section"));
	ABL_BIND_DYNAMIC_PROPERTY(Ability, m_OnEndAnimationMontageSection, TEXT("Montage Section on End"));
}

#if WITH_EDITOR

FText UAblePlayAnimationTask::GetDescriptiveTaskName() const
{
	const FText FormatText = LOCTEXT("AblePlayAnimationTaskFormat", "{0}: {1}");
	FString AnimationName = TEXT("<null>");
	const UAnimationAsset* AnimationAsset = m_AnimationAsset.LoadSynchronous();
	if (AnimationAsset)
	{
        AnimationName = m_AnimationAsset->GetName();

        if (const UAnimMontage* MontageAsset = Cast<UAnimMontage>(AnimationAsset))
        {
            if (m_AnimationMontageSection != NAME_None)
            {
                if(MontageAsset->IsValidSectionName(m_AnimationMontageSection))
                    AnimationName = FString::Format(TEXT("{0}({1})"), { m_AnimationAsset->GetName(), m_AnimationMontageSection.ToString() });
                else
                    AnimationName = FString::Format(TEXT("{0}({1})"), { m_AnimationAsset->GetName(), TEXT("<InvalidSection>") });
            }
        }
	}
	else if (m_AnimationAssetDelegate.IsBound())
	{
		AnimationName = TEXT("Dynamic");
	}

	return FText::FormatOrdered(FormatText, GetTaskName(), FText::FromString(AnimationName));
}

FText UAblePlayAnimationTask::GetRichTextTaskSummary() const
{
	FTextBuilder StringBuilder;

	StringBuilder.AppendLine(Super::GetRichTextTaskSummary());

	FString AnimationName = TEXT("NULL");
	if (m_AnimationAssetDelegate.IsBound())
	{
		AnimationName = FString::Format(TEXT("<a id=\"AbleTextDecorators.GraphReference\" style=\"RichText.Hyperlink\" GraphName=\"{0}\">Dynamic</>"), { m_AnimationAssetDelegate.GetFunctionName().ToString() });
	}
	else
	{
		AnimationName = FString::Format(TEXT("<a id=\"AbleTextDecorators.AssetReference\" style=\"RichText.Hyperlink\" PropertyName=\"m_AnimationAsset\" Filter=\"AnimationAsset\">{0}</>"), { !m_AnimationAsset.ToString().IsEmpty() ? m_AnimationAsset.ToString() : AnimationName });
	}
	StringBuilder.AppendLineFormat(LOCTEXT("AblePlayAnimationTaskRichFmt", "\t- Animation: {0}"), FText::FromString(AnimationName));

	return StringBuilder.ToText();
}

EDataValidationResult UAblePlayAnimationTask::IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors)
{
    EDataValidationResult result = EDataValidationResult::Valid;

    return result;
}

#endif

EAbleAbilityTaskRealm UAblePlayAnimationTask::GetTaskRealmBP_Implementation() const { return m_PlayOnServer ? EAbleAbilityTaskRealm::ATR_ClientAndServer : EAbleAbilityTaskRealm::ATR_Client; }

#undef LOCTEXT_NAMESPACE

