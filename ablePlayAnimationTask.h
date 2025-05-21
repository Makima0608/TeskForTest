// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#pragma once

#include "UnLuaInterface.h"
#include "ableAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "IAbleAbilityTask.h"
#include "UObject/ObjectMacros.h"

#include "ablePlayAnimationTask.generated.h"

#define LOCTEXT_NAMESPACE "AbleAbilityTask"

class UAnimationAsset;
class UAbleAbilityComponent;
class UAbleAbilityContext;

/* Scratchpad for our Task. */
UCLASS(Transient)
class ABLECORESP_API UAblePlayAnimationTaskScratchPad : public UAbleAbilityTaskScratchPad
{
	GENERATED_BODY()
public:
	UAblePlayAnimationTaskScratchPad();
	virtual ~UAblePlayAnimationTaskScratchPad();

	/* The Ability Components of all the actors we affected. */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<UAbleAbilityComponent>> AbilityComponents;

	/* The Skeletal Mesh Components of all the actors we affected (Single Node only). */
	UPROPERTY(transient)
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> SingleNodeSkeletalComponents;

	UPROPERTY(transient)
	TMap<TWeakObjectPtr<USkeletalMeshComponent>,EVisibilityBasedAnimTickOption> CachedVisibilityBasedAnimTickOptionMap;

	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> CurrentAnimMontage;
};

UENUM(BlueprintType)
enum EAblePlayAnimationTaskAnimMode
{
	SingleNode UMETA(DisplayName="Single Node"),
	AbilityAnimationNode UMETA(DisplayName = "Ability Animation Node"),
	DynamicMontage UMETA(DisplayName = "Dynamic Montage")
};

UCLASS()
class ABLECORESP_API UAblePlayAnimationTask : public UAbleAbilityTask, public IUnLuaInterface
{
	GENERATED_BODY()

protected:
	virtual FString GetModuleName_Implementation() const override;

public:
	UAblePlayAnimationTask(const FObjectInitializer& ObjectInitializer);
	virtual ~UAblePlayAnimationTask();

	/* Start our Task. */
	virtual void OnTaskStart(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskStart"))
	void OnTaskStartBP(const UAbleAbilityContext* Context) const;
	
	/* End our Task. */
	virtual void OnTaskEnd(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const EAbleAbilityTaskResult result) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "OnTaskEnd"))
	void OnTaskEndBP(const UAbleAbilityContext* Context, const EAbleAbilityTaskResult result) const;

	/* Check if our Task is finished. */
	virtual bool IsDone(const TWeakObjectPtr<const UAbleAbilityContext>& Context) const override;

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "IsDone"))
	bool IsDoneBP(const UAbleAbilityContext* Context) const;

	/* Returns the End time of our Task. */
	virtual float GetEndTime() const override;

	/* Returns which realm this Task belongs to. */
	virtual EAbleAbilityTaskRealm GetTaskRealm() const override { return GetTaskRealmBP(); }

	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "GetTaskRealm"))
	EAbleAbilityTaskRealm GetTaskRealmBP() const;

	/* Creates the Scratchpad for our Task. */
	virtual UAbleAbilityTaskScratchPad* CreateScratchPad(const TWeakObjectPtr<UAbleAbilityContext>& Context) const;

	/* Returns the Profiler Stat ID for our Task. */
	virtual TStatId GetStatId() const override;

	/* Called when our Time has been set. */
	virtual void OnAbilityTimeSet(const TWeakObjectPtr<const UAbleAbilityContext>& Context) override;

	/* Setup Dynamic Binding. */
	virtual void BindDynamicDelegates( UAbleAbility* Ability ) override;

#if WITH_EDITOR
	/* Returns the category of our Task. */
	virtual FText GetTaskCategory() const override { return LOCTEXT("AblePlayAnimationTaskCategory", "Animation"); }
	
	/* Returns the name of our Task. */
	virtual FText GetTaskName() const override { return LOCTEXT("AblePlayAnimationTask", "Play Animation"); }

	/* Returns the dynamic, descriptive name of our Task. */
	virtual FText GetDescriptiveTaskName() const override;
	
	/* Returns the Rich Text description, with mark ups. */
	virtual FText GetRichTextTaskSummary() const override;

	/* Returns the description of our Task. */
	virtual FText GetTaskDescription() const override { return LOCTEXT("AblePlayAnimationTaskDesc", "Plays an Animation asset (currently only Animation Montage and Animation Segments are supported)."); }
	
	/* Returns the color of our Task. */
	virtual FLinearColor GetTaskColor() const override { return FLinearColor(177.0f / 255.0f, 44.0f / 255.0f, 87.0f / 255.0f); } // Light Blue
	
	/* Returns the estimated runtime cost of our Task. */
	virtual float GetEstimatedTaskCost() const override { return ABLETASK_EST_SYNC; }

	/* Returns how to display the End time of our Task. */
	virtual EVisibility ShowEndTime() const override { return m_ManuallySpecifyAnimationLength ? EVisibility::Visible : EVisibility::Hidden; } // Hidden = Read only (it's still using space in the layout, so might as well display it).

    EDataValidationResult IsTaskDataValid(const UAbleAbility* AbilityContext, const FText& AssetName, TArray<FText>& ValidationErrors) override;
#endif
	/* Returns the Animation Asset. */
	FORCEINLINE const UAnimationAsset* GetAnimationAsset() const { return m_AnimationAsset.LoadSynchronous(); }

	FORCEINLINE TSoftObjectPtr<UAnimationAsset> GetSoftAnimationAsset() const { return m_AnimationAsset; }
	
    /* Returns the Ability State Name. */
    FORCEINLINE const FName& GetAnimationMontageSectionName() const { return m_AnimationMontageSection; }

	/* Sets the Animation Asset. */
	void SetAnimationAsset(UAnimationAsset* Animation);

	/* Returns the Animation Mode. */
	FORCEINLINE EAblePlayAnimationTaskAnimMode GetAnimationMode() const { return m_AnimationMode.GetValue(); }
	
	/* Returns the State Machine Name. */
	FORCEINLINE const FName& GetStateMachineName() const { return m_StateMachineName; }
	
	/* Returns the Ability State Name. */
	FORCEINLINE const FName& GetAbilityStateName() const { return m_AbilityStateName; }

	/* Sets animation mode. */
	FORCEINLINE void SetAnimationMode(EAblePlayAnimationTaskAnimMode Mode) { m_AnimationMode = Mode; }

	/* Sets is loop. */
	FORCEINLINE void SetLoop(bool Loop) { m_Loop = Loop; }

	/* Sets is reset animation state on end. */
	FORCEINLINE void SetResetAnimationStateOnEnd(bool Reset) { m_ResetAnimationStateOnEnd = Reset; }

	/* Sets dynamic montage blend. */
	FORCEINLINE void SetDynamicMontageBlend(const FAbleBlendTimes& BlendTimes) { m_DynamicMontageBlend = BlendTimes; }

	/* Sets slot name. */
	FORCEINLINE void SetSlotName(const FName& SlotName) { m_SlotName = SlotName; }

	/* Sets time to start montage at. */
	FORCEINLINE void SetTimeToStartMontageAt(float Time) { m_TimeToStartMontageAt = Time; }

	/* Sets blend out trigger time. */
	FORCEINLINE void SetBlendOutTriggerTime(float Time) { m_BlendOutTriggerTime = Time; }

	/* Sets number of loops. */
	FORCEINLINE void SetNumberOfLoops(int32 Loops) { m_NumberOfLoops = Loops; }

	/* Sets stop all montages. */
	FORCEINLINE void SetStopAllMontages(bool Stop) { m_StopAllMontages = Stop; }

	virtual void OnAbilityPlayRateChanged(const UAbleAbilityContext* Context, float NewPlayRate) override;

protected:
	/* Helper method to clean up code a bit. This method does the actual PlayAnimation/Montage_Play/etc call.*/
	void PlayAnimation(const TWeakObjectPtr<const UAbleAbilityContext>& Context, const UAnimationAsset* AnimationAsset, const FName& MontageSection, AActor& TargetActor, UAblePlayAnimationTaskScratchPad& ScratchPad, USkeletalMeshComponent& SkeletalMeshComponent, float PlayRate) const;

	void SetAnimationPlayRateInRunTime(const UAbleAbilityContext* Context, const USkeletalMeshComponent& MeshComp, float NewPlayRate) const;
	
	virtual UAnimMontage* PlayMontageBySequence(UAnimInstance* Instance, UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f, float BlendOutTime = 0.25f, float InPlayRate = 1.f, int32 LoopCount = 1, float BlendOutTriggerTime = -1.f, float InTimeToStartMontageAt = 0.f, bool bStopAllMontages = true, float Weight = 1.0f) const;
	
	/* Helper method to find the AbilityAnimGraph Node, if it exists. */
	struct FAnimNode_SPAbilityAnimPlayer* GetAbilityAnimGraphNode(const TWeakObjectPtr<const UAbleAbilityContext>& Context, USkeletalMeshComponent* MeshComponent) const;
	
	// The Animation to play.
    UPROPERTY(EditAnywhere, Category="Animation", meta = (DisplayName = "Animation", AbleBindableProperty, AbleDefaultBinding = "OnGetAnimationAssetBP", AllowedClasses = "AnimMontage,AnimSequence"))
	TSoftObjectPtr<UAnimationAsset> m_AnimationAsset;

	UPROPERTY()
	FGetAbleAnimation m_AnimationAssetDelegate;

	/* The animation montage section to jump to if using Dynamic Montages. */
    UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Montage Section", AbleBindableProperty))
    FName m_AnimationMontageSection;

	UPROPERTY()
	FGetAbleName m_AnimationMontageSectionDelegate;

	/* If set, Jump to this montage section when the Task ends, if using Dynamic Montages. Set it to Name_None to disable this feature.*/
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Montage Section on End", AbleBindableProperty ))
	FName m_OnEndAnimationMontageSection;

	UPROPERTY()
	FGetAbleName m_OnEndAnimationMontageSectionDelegate;

	/* What mode to use for this task. 
	*  Single Node - Plays the Animation as a Single Node Animation outside of the Animation Blueprint (if there is one).
	*  Ability Animation Node - Plays the Animation using the Ability Animation Node within an Animation State Machine.
	*  Dynamic Montage - Plays the Animation as a Dynamic Montage.
	*/
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Animation Mode", EditCondition = "m_AnimationAsset!=nullptr"))
	TEnumAsByte<EAblePlayAnimationTaskAnimMode> m_AnimationMode;

	/* The name of the State Machine we should look for our Ability State in.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "State Machine Name", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FName m_StateMachineName;

	UPROPERTY()
	FGetAbleName m_StateMachineNameDelegate;

	/* The name of the State that contains the Ability Animation Player Node*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Ability State Name", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FName m_AbilityStateName;

	UPROPERTY()
	FGetAbleName m_AbilityStateNameDelegate;

	/* If the node is already playing an animation, this is the blend used transition to this animation.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Blend In", EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FAlphaBlend m_BlendIn;

	/* If the node doesn't have any more animations to play, this is the blend used transition out of this animation. NOTE: Currently unused.*/
	UPROPERTY(EditAnywhere, Category = "Ability Node", meta = (DisplayName = "Blend Out", EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	FAlphaBlend m_BlendOut;

	// Whether or not to loop the animation.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Loop", EditCondition="m_AnimationAsset!=nullptr"))
	bool m_Loop;

	// Blend times to use when playing the animation as a dynamic montage.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Play Blend", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::DynamicMontage"))
	FAbleBlendTimes m_DynamicMontageBlend;

	UPROPERTY()
	FGetAbleBlendTimes m_DynamicMontageBlendDelegate;

	// Slot Name to use for the Sequence when playing as a dynamic montage.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Slot Name", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::DynamicMontage"))
	FName m_SlotName;

	UPROPERTY()
	FGetAbleName m_SlotNameDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage, or playing a Montage Asset (including as a Single Node Animation). Offsets the starting time of the animation.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Time To Start At", AbleBindableProperty, EditCondition = "m_AnimationMode != EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	float m_TimeToStartMontageAt;

	UPROPERTY()
	FGetAbleFloat m_TimeToStartMontageAtDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage. Offsets the Blend out time. 
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Blend Out Trigger Time", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::DynamicMontage"))
	float m_BlendOutTriggerTime;

	UPROPERTY()
	FGetAbleFloat m_BlendOutTriggerTimeDelegate;

	// Used when attempting to play a Slot Animation Asset as a Dynamic Montage. Loops the Animation N times.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Number Of Loops", AbleBindableProperty, EditCondition = "m_AnimationMode == EAblePlayAnimationTaskAnimMode::DynamicMontage"))
	int32 m_NumberOfLoops;

	UPROPERTY()
	FGetAbleInt m_NumberOfLoopsDelegate;

	// Used when attempting to play a Montage Asset (including as a Single Node Animation). Stops all existing montages.
	UPROPERTY(EditAnywhere, Category = "Dynamic Montage", meta = (DisplayName = "Stop All Montages", AbleBindableProperty, EditCondition = "m_AnimationMode != EAblePlayAnimationTaskAnimMode::AbilityAnimationNode"))
	bool m_StopAllMontages;

	UPROPERTY()
	FGetAbleBool m_StopAllMontagesDelegate;

	// Animation Play Rate
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Play Rate", AbleBindableProperty, EditCondition = "m_AnimationAsset!=nullptr"))
	float m_PlayRate;

	UPROPERTY()
	FGetAbleFloat m_PlayRateDelegate;

	// If true, we scale our Animation Play Rate by what our Ability Play Rate is. So, if your Ability Play Rate is 2.0, the animation play rate is multiplied by that same value.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Scale With Ability Play Rate", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ScaleWithAbilityPlayRate;

	// If true, stop the current playing animation mid blend if the owning Ability is interrupted. Only applies when Animation Mode is set to Ability Animation Node.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Stop on Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_StopAnimationOnInterrupt;

	// If true, any queued up (using the Ability Animation Node) animations will be removed as well. 
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Clear Queued Animation On Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ClearQueuedAnimationOnInterrupt;

	// If true, Able will try to reset you into whatever Animation State you were in when the Task started (e.g., Single Instance vs Animation Blueprint).
	// You can disable this if you see it causing issues, as this does reset the animation instance time when it's called.
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Reset Animation State On End", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ResetAnimationStateOnEnd;

	// If true, you can tell Able how long this task (and thus the animation) should play. Play rate still modifies this length. 
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Manually Specified Animation Length", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ManuallySpecifyAnimationLength;

	/* If true, we'll treat a manually specified length as an interrupt - so normal rules for stopping, clearing the queue, etc apply. */
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Manual Length Is Interrupt", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_ManualLengthIsInterrupt;

	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Stop Animation On BranchSegment", EditCondition = "m_AnimationAsset!=nullptr"))
	bool bStopAnimationOnBranch = false;

	/* A String identifier you can use to identify this specific task in the Ability blueprint, when GetSkeletalMeshComponentForActor is called on the Ability. */
	UPROPERTY(EditAnywhere, Category = "Animation|Dynamic", meta = (DisplayName = "Event Name"))
	FName m_EventName;
	
	/* If true, in a networked game, the animation will be played on the server. 
	*  You should only use this if you have collision queries that rely on bone positions
	*  or animation velocities.
	*/
	UPROPERTY(EditAnywhere, Category = "Network", meta=(DisplayName="Play On Server", EditCondition = "m_AnimationAsset!=nullptr"))
	bool m_PlayOnServer;

	/* If true, we'll treat a manually specified length as an interrupt - so normal rules for stopping, clearing the queue, etc apply. */
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "Override Visibility Based Anim Tick"))
	bool m_OverrideVisibilityBasedAnimTick;
	
	/* If true, we'll treat a manually specified length as an interrupt - so normal rules for stopping, clearing the queue, etc apply. */
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (DisplayName = "New Visibility Based Anim Tick", EditCondition = "m_OverrideVisibilityBasedAnimTick", EditConditionHides))
	EVisibilityBasedAnimTickOption m_VisibilityBasedAnimTick;
};

#undef LOCTEXT_NAMESPACE
