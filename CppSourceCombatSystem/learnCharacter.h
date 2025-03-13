// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "learnCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AlearnCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr <UInputMappingContext> DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr <UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr <UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> DodgeAction;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> GuardAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AttackAction;

public:
	AlearnCharacter();


	

protected:

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void PrintMessage(FString s);


	/** Dodge 函数（蓝图可调用） */
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void Dodge();

	/** Dodge 强度 */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	//float DodgeStrength = 600.f;

	/** Dodge 冷却时间 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeCooldown = 0.9f;

	/** 是否可以 Dodge */
	bool bCanDodge = true;

	FTimerHandle DodgeTimerHandle;

	FTimerHandle DodgeCDTimerHandle;

	FTimerHandle ComboTimerHandle;

	FTimerHandle AttackTimerHandle;

	FVector DodgeDirection;


	int const MaxComboCount = 5;


	bool bIsAttacking = false;

	bool bIsNotStuned = true;


	UPROPERTY(BlueprintReadOnly, Category = "Dodge", meta = (AllowPrivateAccess = "true"))
	bool bIsDoging = false;

	bool attackActionEnd = true;

	int ComboCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Guard", meta = (AllowPrivateAccess = "true"))
	bool bIsGuarding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Guard", meta = (AllowPrivateAccess = "true"))
	FVector GuardDirection = GetActorForwardVector();
	
	UPROPERTY(BlueprintReadOnly, Category = "Guard", meta = (AllowPrivateAccess = "true"))
	FRotator GuardRotation;
	

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void StartGuarding();

	UFUNCTION(BlueprintCallable, Category = "Guard")
	void StopGuarding();


	UFUNCTION(BlueprintCallable, Category = "Guard")
	void PlayGuardAnimation();

	


	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void DodgeCompleted();

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PlayDodgeAnimation();

	UFUNCTION(BlueprintCallable, Category = "Attack_combo")
	void AttackCombo();

	void PlayComboAnimation();

	void ResetCombo();

	void resetAttackActionEnd();
	
	/** 处理 Dodge 冷却 */
	void ResetDodge();

	/** Dodge Input Action */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	//TObjectPtr<UInputAction> DodgeAction;


	//combo function

	
	UAnimMontage* GetComboMontage(int ComboIndex);

	
	


	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UAnimMontage> GuardMontage;
	


	// Attack_combo part

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ComboMontage1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ComboMontage2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ComboMontage3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ComboMontage4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> ComboMontage5;


	

	

	

			

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

