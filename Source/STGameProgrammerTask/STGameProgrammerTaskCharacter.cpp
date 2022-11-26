// Copyright Epic Games, Inc. All Rights Reserved.

#include "STGameProgrammerTaskCharacter.h"
#include "STGameProgrammerTaskProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASTGameProgrammerTaskCharacter::ASTGameProgrammerTaskCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	//FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	
	GrabbedObjectLocation = CreateDefaultSubobject<USceneComponent>(TEXT("GrabbedObjectLocation"));
	GrabbedObjectLocation->SetupAttachment(FP_Gun);
}

void ASTGameProgrammerTaskCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Binding dash functions
	if(DashCurve)
	{
		FOnTimelineFloat TimelineUpdateCallback;
		FOnTimelineEventStatic TimelineFinishedCallback;

		TimelineUpdateCallback.BindUFunction(this, FName("DashUpdate"));
		TimelineFinishedCallback.BindUFunction(this, FName("DashFinished"));
		DashTimeline.AddInterpFloat(DashCurve, TimelineUpdateCallback);
		DashTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
	}

	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DashCurve not assigned in character BP. Please assign it!"));
	}

	
	
	// Jetpack curve assign check
	if(JetpackBoostCurve == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("JetpackBoostCurve is not assigned in character BP!"));
	}
	JetpackTime = JetpackMaxTime;
}

void ASTGameProgrammerTaskCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASTGameProgrammerTaskCharacter::JumpButtonPressed);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASTGameProgrammerTaskCharacter::JumpButtonReleased);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASTGameProgrammerTaskCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASTGameProgrammerTaskCharacter::EndFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ASTGameProgrammerTaskCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASTGameProgrammerTaskCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASTGameProgrammerTaskCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASTGameProgrammerTaskCharacter::LookUpAtRate);

	// Dash input binding
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ASTGameProgrammerTaskCharacter::Dash);
}


void ASTGameProgrammerTaskCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DashTimeline.TickTimeline(DeltaSeconds);
	UpdateJetpack(DeltaSeconds);
}

void ASTGameProgrammerTaskCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	ToggleJetpack(true, false);
}

void ASTGameProgrammerTaskCharacter::OnFire()
{
	const FVector PickupTraceStart = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector PickupTraceEnd = PickupTraceStart + GetFirstPersonCameraComponent()->GetForwardVector() * PickupTraceRange;
	FHitResult PickupTraceHitResult;

	const bool bHit = UKismetSystemLibrary::SphereTraceSingle(this, PickupTraceStart, PickupTraceEnd, PickupTraceRadius, UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false, {this}, EDrawDebugTrace::None, PickupTraceHitResult, true);

	if(bHit)
	{
		// If the Hit Component is using physics simulation, we grap it.
		if(UPrimitiveComponent* Prim = PickupTraceHitResult.GetComponent())
		{
			if(Prim->IsSimulatingPhysics())
			{
				SetGrabbedObject(Prim);
			}
		}
	}

	
}

void ASTGameProgrammerTaskCharacter::EndFire()
{
	// Releasing the grabbed object if there is one.
	if(GrabbedObject)
	{
		const FVector ShootVelocity = GetFirstPersonCameraComponent()->GetForwardVector() * FiringForce;

		GrabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedObject->SetSimulatePhysics(true);
		GrabbedObject->AddImpulse(ShootVelocity, NAME_None, true);
		SetGrabbedObject(nullptr);
	}
}

void ASTGameProgrammerTaskCharacter::JumpButtonPressed()
{
	if(GetCharacterMovement())
	{
		switch (GetCharacterMovement()->MovementMode)
		{
		case EMovementMode::MOVE_Falling:
			ToggleJetpack(false, true);
		case EMovementMode::MOVE_Walking:
			Jump();
		default: ;
		}
	
	}
}

void ASTGameProgrammerTaskCharacter::JumpButtonReleased()
{
	StopJumping();
	ToggleJetpack(false, false);
}

void ASTGameProgrammerTaskCharacter::ToggleJetpack(const bool bReset, const bool bActivate)
{
	if(bReset)
	{
		JetpackTime = JetpackMaxTime;
	}

	bIsJetpackActive = bActivate;

	if(GetCharacterMovement())
	{
		GetCharacterMovement()->AirControl = bIsJetpackActive ? 5.f : 1.f;
	}
}

void ASTGameProgrammerTaskCharacter::UpdateJetpack(const float DeltaSeconds)
{
	if(bIsJetpackActive)
	{
		JetpackTime -= DeltaSeconds;

		if(JetpackBoostCurve)
		{
			const float BoostAmount = JetpackBoostCurve->GetFloatValue(UKismetMathLibrary::NormalizeToRange(JetpackMaxTime - JetpackTime, 0.f, JetpackMaxTime)) * JetpackBoostForce;
			LaunchCharacter(FVector(0.f, 0.f, BoostAmount), false, true);
		}

		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JetpackBoostCurve is not assigned in character BP!"));
		}

		if(JetpackTime <= 0.f)
		{
			ToggleJetpack(false, false);
		}
		
	}
}

// Dash implementation
void ASTGameProgrammerTaskCharacter::Dash()
{
	if(bIsDashing) return;
	
	if(GetVelocity().Size() <= 0.f) return;

	if(DashCurve == nullptr) return;

	FVector DashDirection = GetVelocity();
	DashDirection.Z = 0.f;
	DashDirection.Normalize();
	
	DashStartLocation = GetActorLocation();
	DashEndLocation = DashStartLocation + DashDirection * DashDistance;
	
	DashTimeline.SetPlayRate(1 / (DashDistance / DashSpeed));
	DashTimeline.PlayFromStart();
	bIsDashing = true;
}

void ASTGameProgrammerTaskCharacter::DashUpdate()
{
	const float TimelineValue = DashTimeline.GetPlaybackPosition();
	const float CurveFloatValue = DashCurve->GetFloatValue(TimelineValue);

	const FVector TargetLocation = FMath::Lerp(DashStartLocation, DashEndLocation, CurveFloatValue);
	SetActorLocation(TargetLocation, true);
}

void ASTGameProgrammerTaskCharacter::DashFinished()
{
	bIsDashing = false;
}


void ASTGameProgrammerTaskCharacter::SetGrabbedObject(UPrimitiveComponent* ObjectToGrab)
{
	GrabbedObject = ObjectToGrab;

	if(GrabbedObject)
	{
		GrabbedObject->SetSimulatePhysics(false);
		GrabbedObject->AttachToComponent(GrabbedObjectLocation, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

// Moving and Looking functions
void ASTGameProgrammerTaskCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ASTGameProgrammerTaskCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ASTGameProgrammerTaskCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASTGameProgrammerTaskCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}
