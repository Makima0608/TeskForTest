// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SPGame/Skill/Task/SPTurnToTask.h"
#include "Game/SPGame/Character/SPGameCharacterBase.h"
#include "Game/SPGame/Character/SPGameMonsterBase.h"
#include "Game/SPGame/Monster/AnimInstance/SPMonsterAnimInstance.h"
#include "Game/SPGame/State/StateData/SPStunStateData.h"
#include "Game/SPGame/Utils/SPGameLibrary.h"
#include "Game/SPGame/Utils/SPCharacterLibrary.h"


#define LOCTEXT_NAMESPACE "SPSkillAbilityTask"

USPTurnToTask::USPTurnToTask(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer),
	m_MoveAnimState(false),
	m_RotateByMasterFaceTo(false)
{

}

USPTurnToTask::~USPTurnToTask()
{
}

FString USPTurnToTask::GetModuleName_Implementation() const
{
	return TEXT("Feature.StarP.Script.System.Ability.Task.SPTurnToTask");
}

void USPTurnToTask::OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const
{
	Super::OnTaskStart(Context);
	OnTaskStartBP_Override(Context.Get());
}

void USPTurnToTask::OnTaskStartBP_Override_Implementation(const UAbleAbilityContext* Context) const
{
	if (m_MoveAnimState)
	{
		UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		for (FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
		{
			if (Entry.Actor.IsValid())
			{
				if (const USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(Entry.Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass())))
				{
					UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
					if (USPMonsterAnimInstance* SPMonsterAnimInstance = Cast<USPMonsterAnimInstance>(AnimInstance))
					{
						SPMonsterAnimInstance->bMonsterCanTurn = true;
					}
				}
			}
		}
	}
}

void USPTurnToTask::OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context,
	const EAbleAbilityTaskResult result) const
{
	Super::OnTaskEnd(Context, result);
	OnTaskEndBP_Override(Context.Get(), result);
}

void USPTurnToTask::TurnToSetActorRotation(const TWeakObjectPtr<AActor> TargetActor, const FRotator& TargetRotation) const
{
	if (TargetActor.IsValid())
	{
		if (ISPActorInterface* SPActor = Cast<ISPActorInterface>(TargetActor))
		{
			FHitResult Result = FHitResult();
			SPActor->SPSetActorRotation(TargetRotation, false, Result);
		}
		else
		{
			Super::TurnToSetActorRotation(TargetActor, TargetRotation);
		}
	}
}

void USPTurnToTask::TurnAnimation(const TWeakObjectPtr<AActor> TargetActor, const float DeltaYaw) const
{
    if(m_bTurnto)
    {
    	if (ASPGameMonsterBase * MonsterPawn =Cast<ASPGameMonsterBase>(TargetActor))
    	{
    		MonsterPawn->SetEnableRotateOnSpot(true);
    		MonsterPawn->SetRotateOnSpotDirection(DeltaYaw);
    	}
    }
	
}

void USPTurnToTask::OnTaskEndBP_Override_Implementation(const UAbleAbilityContext* Context,
                                                        const EAbleAbilityTaskResult result) const
{
	if (m_MoveAnimState)
	{
		UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		for (FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
		{
			if (Entry.Actor.IsValid())
			{
				if (const USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(Entry.Actor->GetComponentByClass(USkeletalMeshComponent::StaticClass())))
				{
					UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
					if (USPMonsterAnimInstance* SPMonsterAnimInstance = Cast<USPMonsterAnimInstance>(AnimInstance))
					{
						if (SPMonsterAnimInstance->bMonsterCanTurn)
						{
							SPMonsterAnimInstance->bMonsterCanTurn = false;
						}
					}
				}
			}
		}
	}
	if (m_bTurnto)
	{
		UAbleTurnToTaskScratchPad* ScratchPad = Cast<UAbleTurnToTaskScratchPad>(Context->GetScratchPadForTask(this));
		if (!ScratchPad) return;
		for (FTurnToTaskEntry& Entry : ScratchPad->InProgressTurn)
		{
			if (Entry.Actor.IsValid())
			{
				if (ASPGameMonsterBase * MonsterPawn = Cast<ASPGameMonsterBase>(Entry.Actor))
				{
					MonsterPawn->SetEnableRotateOnSpot(false);
					MonsterPawn->SetRotateOnSpotDirection(0);
				}
			}
		}
	}
}

FRotator USPTurnToTask::GetTargetRotation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const AActor* Source, const AActor* Destination) const
{
	if (m_RotateByMasterFaceTo)
	{
		return GetTargetRotationByMasterFaceTo(Source);
	}
	return Super::GetTargetRotation(Context, Source, Destination);
}

FRotator USPTurnToTask::GetTargetRotationByMasterFaceTo(const AActor* Source) const
{
	FVector SourceLocation = Source->GetActorLocation();
	FRotator SourceRotation = Source->GetActorRotation();

	if (USkeletalMeshComponent* MeshComponent = Source->FindComponentByClass<USkeletalMeshComponent>())
	{
		SourceLocation = MeshComponent->GetComponentLocation();
		SourceRotation = MeshComponent->GetComponentRotation();
	}

	if (ISPActorInterface* SPActor = Cast<ISPActorInterface>(const_cast<AActor*>(Source)))
	{
		if (AActor* MasterActor = SPActor->GetMaster())
		{
			if (ISPActorInterface* MasterSPActor = Cast<ISPActorInterface>(MasterActor))
			{
				SourceRotation = USPCharacterLibrary::GetActorRotation(MasterSPActor);
			}
		}
	}

	return SourceRotation;
}

#undef LOCTEXT_NAMESPACE