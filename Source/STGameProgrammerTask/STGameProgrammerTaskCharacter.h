// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "STGameProgrammerTaskCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class ASTGameProgrammerTaskCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;
	
	

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;


public:
	ASTGameProgrammerTaskCharacter();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;


protected:
	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	
	void JumpButtonPressed();
	void JumpButtonReleased();


	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	

	/********************** Dash Section ********************/
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash", meta=(ClampMin=1, UIMin=1))
	float DashSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dash", meta=(ClampMin=1, UIMin=1))
	float DashDistance = 1000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dash")
	bool bIsDashing = false;

	FVector DashStartLocation;
	FVector DashEndLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	class UCurveFloat* DashCurve;

	FTimeline DashTimeline;

	UFUNCTION(Category="Dash")
	void Dash();

	UFUNCTION()
	void DashUpdate();

	UFUNCTION()
	void DashFinished();
	/********************** End of Dash Section ********************/


	/********************** Gravity Gun Section ********************/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gravity Gun")
	class USceneComponent* GrabbedObjectLocation;

	UPROPERTY()
	class UPrimitiveComponent* GrabbedObject;

	UFUNCTION()
	void SetGrabbedObject(UPrimitiveComponent* ObjectToGrab);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity Gun")
	float PickupTraceRange = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity Gun")
	float PickupTraceRadius = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gravity Gun")
	float FiringForce = 5000.f;

	void OnFire();

	void EndFire();
	
	/********************** End of Gravity Gun Section ********************/

	
	/********************** Jetpack Section ********************/
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jetpack")
	float JetpackMaxTime = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Jetpack")
	float JetpackTime = JetpackMaxTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Jetpack")
	float JetpackBoostForce = 120.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Jetpack")
	bool bIsJetpackActive = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Jetpack")
	class UCurveFloat* JetpackBoostCurve;
	
	void ToggleJetpack(const bool bReset, const bool bActivate);
	void UpdateJetpack(const float DeltaSeconds);
	
	/********************** End of Jetpack Section ********************/

};

