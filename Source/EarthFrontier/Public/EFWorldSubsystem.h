// UNVERIFIED IN THIS ENVIRONMENT (no UE5 toolchain available to compile this file).
//
// UEFWorldSubsystem owns the single authoritative ef::WorldState for the game instance and is the
// ONLY place engine code talks to it. Actors/HUD submit commands and read projections through it.
// World ticks are driven by accumulated game-time seconds (1 world tick = 1 game minute by default),
// decoupled from the render frame rate (master prompt §06, §40).
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/SaveService.h"
#include <memory>
#include "EFWorldSubsystem.generated.h"

UENUM(BlueprintType)
enum class EEFPossessionMode : uint8 { OnFoot, Seated };

USTRUCT(BlueprintType)
struct FEFCommandResult {
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bOk = false;
	UPROPERTY(BlueprintReadOnly) FString Code;
	UPROPERTY(BlueprintReadOnly) FString Message;
	UPROPERTY(BlueprintReadOnly) int32 CommittedEvents = 0;
};

USTRUCT(BlueprintType)
struct FEFNewsItem {
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Headline;
	UPROPERTY(BlueprintReadOnly) int64 Tick = 0;
	UPROPERTY(BlueprintReadOnly) FString SourceEventId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEFOnWorldEvent, const FString&, EventType, const FString&, EventId);

UCLASS(Config=Game)
class EARTHFRONTIER_API UEFWorldSubsystem : public UGameInstanceSubsystem, public FTickableGameObject {
	GENERATED_BODY()
public:
	// ---- lifecycle ----
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UEFWorldSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return bWorldReady; }

	// ---- game flow ----
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|World")
	bool StartNewGame(const FString& ContentRelativePath = TEXT("Data/asterion_island_v1.json"));
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Save")
	bool SaveGame(const FString& Slot);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Save")
	bool LoadGame(const FString& Slot, TArray<FString>& OutWarnings);

	// ---- commands (validated by the core; UI never mutates state directly) ----
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult InteractNpc(const FString& NpcId);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult AcceptMission(const FString& MissionId);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult EnterVehicle(const FString& VehicleId, const FString& SeatId);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult ExitVehicle();
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult SwitchSeat(const FString& SeatId);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult LoadCargo(const FString& NodeId, const FString& ItemId, int32 Quantity);
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult DeliverCargo(const FString& NodeId, const FString& ItemId, int32 Quantity);
	// Presentation reports authoritative transforms at a low rate (e.g. 2 Hz), in core units (1 uu = 1 cm -> core meters).
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|Commands") FEFCommandResult ReportPose(const FString& EntityId, const FTransform& Transform);

	// ---- read-only projections ----
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") int64 GetWorldTick() const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetClockText() const;      // "Day 1 08:30"
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") int32 GetCredits() const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") EEFPossessionMode GetPossessionMode() const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetPossessedVehicleId() const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetRoleLabel() const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") int32 GetLocalTrust(const FString& RegionId) const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") int32 GetFactionReputation(const FString& FactionId) const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetNpcDialogue(const FString& NpcId) const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetWeather(const FString& RegionId) const;
	UFUNCTION(BlueprintPure, Category = "EarthFrontier|World") FString GetDebugSummary() const;
	UFUNCTION(BlueprintCallable, Category = "EarthFrontier|World") TArray<FEFNewsItem> GetLatestNews(int32 Count) const;

	// Fired for every committed event (including derived ones) so presentation can react (VFX, HUD toasts).
	UPROPERTY(BlueprintAssignable, Category = "EarthFrontier|Events") FEFOnWorldEvent OnWorldEvent;

	// Direct access for C++ gameplay code (vehicles, pawns). Const view only; mutate via commands.
	const ef::WorldState* GetWorldState() const { return World.get(); }

	// Game seconds per world tick. 1 real second = 1 game minute by default (configurable in DefaultGame.ini later).
	UPROPERTY(EditAnywhere, Config, Category = "EarthFrontier|Time") float GameSecondsPerWorldTick = 1.0f;

private:
	FEFCommandResult Submit(const ef::Command& Cmd);
	void BroadcastNewEvents(size_t PreviousJournalSize);
	FString SaveDirectory() const;

	std::unique_ptr<ef::WorldState> World;
	std::unique_ptr<ef::FileSaveStorage> Storage;
	std::unique_ptr<ef::SaveService> Saves;
	float TickAccumulator = 0.f;
	bool bWorldReady = false;
};
