// Copyright Epic Games, Inc. All Rights Reserved.

#include "learnCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

//not sure if this is needed
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"



DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AlearnCharacter

AlearnCharacter::AlearnCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 750.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AlearnCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AlearnCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AlearnCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AlearnCharacter::Look);


		// Dodge 定义
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &AlearnCharacter::Dodge);
		
		// 这个可以定义Start，End 从而判断是否是长按或者短按
		//EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Completed, this, &AlearnCharacter::DodgeCompleted);


		// guarding 判断

		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, this, &AlearnCharacter::StartGuarding);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Completed, this, &AlearnCharacter::StopGuarding);

		//attack or combo

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AlearnCharacter::AttackCombo);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}




void AlearnCharacter::PrintMessage(FString s)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,                     // 消息 ID（-1 代表新的消息）
			5.f,                    // 显示 5 秒
			FColor::Green,          // 颜色
			s // 显示的内容	
		);
	}
}




void AlearnCharacter::StartGuarding()
{
	if (attackActionEnd == false) {
		//PrintMessage("attack not finished");
		return;
	}

	//PrintMessage("Start Guarding");
	if (bIsGuarding)
	{
		//PrintMessage("Already Guarding");
		return;
	}

	if (!bIsNotStuned)
	{
		//PrintMessage("Stunned, cannot guard");
		return;
	}

	// 计算举盾朝向（基于玩家的移动输入）
	GuardDirection = GetLastMovementInputVector();
	if (GuardDirection.IsNearlyZero())
	{
		GuardDirection = GetActorForwardVector();
	}

	GuardDirection.Z = 0; // 防止 Yaw 旋转错误
	GuardDirection.Normalize();

	// 立即调整角色朝向
	GuardRotation = GuardDirection.Rotation();
	SetActorRotation(GuardRotation);

	// 进入举盾状态
	bIsGuarding = true;

	// 播放举盾动画
	PlayGuardAnimation();
}

void AlearnCharacter::StopGuarding()
{
	if (!bIsGuarding)
	{
		//PrintMessage("Not Guarding");
		return;
	}

	// 退出举盾状态
	bIsGuarding = false;
}




void AlearnCharacter::PlayGuardAnimation()
{
	if (GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(GuardMontage);
	}
}



void AlearnCharacter::Dodge()
{
	if (attackActionEnd == false) {
		//PrintMessage("attack not finished");
		return;
	}
	

	if (!bCanDodge) {
		//PrintMessage("Cannot Dodge");
		return;
	}

	if (!bIsNotStuned) {
		//PrintMessage("Stunned, cannot guard");
		return;
	}

	if (bIsGuarding) {
		//PrintMessage("Guarding, cannot dodge");
		return;
	}

    DodgeDirection = GetLastMovementInputVector();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector();

		
	}

	
	DodgeDirection.Z = 0; // 防止 Dodge 方向带有 Z 分量
	DodgeDirection.Normalize();
	
	FRotator DodgeRotation = DodgeDirection.Rotation();
	SetActorRotation(DodgeRotation);

	// 禁用输入，避免 Dodge 过程中其他操作
	GetCharacterMovement()->DisableMovement();

	// 触发 Root Motion Dodge
	PlayDodgeAnimation();  // 需要用 AnimMontage 处理 Root Motion

	// 触发冷却
	bCanDodge = false;

	float DodgeDuration = 1.0f; // Dodge 动画时长
	if (DodgeMontage)
	{
		DodgeDuration = DodgeMontage->GetPlayLength();
	}

	bIsDoging = true;

	GetWorldTimerManager().SetTimer(
		DodgeCDTimerHandle, this, &AlearnCharacter::ResetDodge, DodgeCooldown, false);

	GetWorldTimerManager().SetTimer(DodgeTimerHandle, this, &AlearnCharacter::DodgeCompleted, DodgeDuration, false);
	UE_LOG(LogTemp, Display, TEXT("Dodge Duration: %f seconds, %d"), DodgeDuration, bCanDodge);

	DodgeCompleted();

}


void AlearnCharacter::DodgeCompleted()
{
	bIsDoging = false;
	
	//PrintMessage(FString::Printf(TEXT("Dodge Completed: %s"), bIsDoging ? TEXT("True") : TEXT("False")));

	
	// 恢复移动
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	

	// 允许再次 Dodge
	
	//PrintMessage("Dodge Completed");
	
}


void AlearnCharacter::PlayDodgeAnimation()
{
	if (DodgeMontage && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DodgeMontage);
	}
}

void AlearnCharacter::ResetDodge()
{
	bCanDodge = true;
}

void AlearnCharacter::AttackCombo()
{
	
	if (!attackActionEnd) {
		//PrintMessage("attack not finished");
		return;
	}
	//PrintMessage("Start attacking");
	attackActionEnd = false;

	// **如果正在攻击，玩家手动触发下一次攻击**
	if (bIsAttacking)
	{
		if (ComboCount < MaxComboCount)
		{
			//PrintMessage(FString::Printf(TEXT("combo count : %d"), ComboCount));

			ComboCount++;  // 增加连击数
			PlayComboAnimation();  // 播放下一个 Combo
		}
		return;
	}

	// **检查 Combo 是否超时**
	if (GetWorldTimerManager().IsTimerActive(ComboTimerHandle))  // 仍然在连击时间内
	{
		ComboCount++;  // 继续连击
	}
	else  // **超时，重新开始攻击**
	{
		ComboCount = 1;
	}

	PlayComboAnimation();  // 播放 Combo 动画
}

void AlearnCharacter::PlayComboAnimation()
{
	if (!GetMesh() || !GetMesh()->GetAnimInstance()) {
		resetAttackActionEnd();
		return;
	}

	UAnimMontage* CurrentMontage = GetComboMontage(ComboCount);
	if (!CurrentMontage) { 
		resetAttackActionEnd();
		ResetCombo();
		return; 
	}

	bIsAttacking = true;

	float MontageDuration = GetMesh()->GetAnimInstance()->Montage_Play(CurrentMontage);

	// **设置 ResetCombo 计时器 = 动画时长 + 0.5s**
	float ComboResetTime = MontageDuration + 1.0f;
	GetCharacterMovement()->DisableMovement();
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AlearnCharacter::resetAttackActionEnd, MontageDuration, false);
	GetWorldTimerManager().SetTimer(ComboTimerHandle, this, &AlearnCharacter::ResetCombo, ComboResetTime, false);
}




// **继续连击（不再自动触发动画）**
void AlearnCharacter::resetAttackActionEnd()
{
	bIsAttacking = false;
	attackActionEnd = true;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

// **重置 Combo**
void AlearnCharacter::ResetCombo()
{
	//PrintMessage("Combo Reset");
	bIsAttacking = false;
	ComboCount = 0;
	GetWorldTimerManager().ClearTimer(ComboTimerHandle);
}


// 获取对应 Combo 的 Montage
UAnimMontage* AlearnCharacter::GetComboMontage(int ComboIndex)
{
	//PrintMessage(FString::Printf(TEXT("Combo Index: %d"), ComboIndex));
	switch (ComboIndex)
	{
	case 1: return ComboMontage1;
	case 2: return ComboMontage2;
	case 3: return ComboMontage3;
	case 4: return ComboMontage4;
	case 5: return ComboMontage5;
	default: 
		
		return nullptr;
	}
}


void AlearnCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AlearnCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
