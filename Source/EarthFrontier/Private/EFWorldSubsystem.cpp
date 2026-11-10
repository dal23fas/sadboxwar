// UNVERIFIED IN THIS ENVIRONMENT (no UE5 toolchain available to compile this file).
#include "EFWorldSubsystem.h"
#include "EarthFrontierLog.h"
#include "EFWorldCore/ContentLoader.h"
#include "Misc/Paths.h"
#include <string>

namespace {
std::string ToStd(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }
FString ToF(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }
}

void UEFWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);
	Storage = std::make_unique<ef::FileSaveStorage>(ToStd(SaveDirectory()));
	Saves = std::make_unique<ef::SaveService>(*Storage);
	UE_LOG(LogEarthFrontier, Log, TEXT("[EF] WorldCore ready (save schema v%d, event schema v%d)"), ef::kSaveSchemaVersion, ef::kEventSchemaVersion);
}

void UEFWorldSubsystem::Deinitialize() {
	World.reset(); Saves.reset(); Storage.reset();
	bWorldReady = false;
	Super::Deinitialize();
}

FString UEFWorldSubsystem::SaveDirectory() const { return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("EFSaves")); }

bool UEFWorldSubsystem::StartNewGame(const FString& ContentRelativePath) {
	ef::WorldContent Content;
	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), ContentRelativePath);
	ef::Status S = ef::loadContentFromFile(ToStd(FullPath), Content);
	if (!S) { UE_LOG(LogEarthFrontier, Error, TEXT("[EF] content load failed: %s %s"), *ToF(S.code), *ToF(S.message)); return false; }
	World = std::make_unique<ef::WorldState>();
	S = World->initFromContent(Content);
	if (!S) { UE_LOG(LogEarthFrontier, Error, TEXT("[EF] init failed: %s %s"), *ToF(S.code), *ToF(S.message)); World.reset(); return false; }
	TickAccumulator = 0.f;
	bWorldReady = true;
	UE_LOG(LogEarthFrontier, Log, TEXT("[EF] New game: content=%s player=%s"), *ToF(Content.content_id), *ToF(World->player().entity_id));
	return true;
}

bool UEFWorldSubsystem::SaveGame(const FString& Slot) {
	if (!World) return false;
	ef::Status S = Saves->save(*World, ToStd(Slot), "ue5-phase0");
	if (!S) UE_LOG(LogEarthFrontier, Error, TEXT("[EF] save failed: %s %s"), *ToF(S.code), *ToF(S.message));
	return S.ok;
}

bool UEFWorldSubsystem::LoadGame(const FString& Slot, TArray<FString>& OutWarnings) {
	// Content must be loaded first so definitions exist; then the snapshot overlays the state.
	if (!World && !StartNewGame()) return false;
	std::vector<std::string> Warnings;
	ef::Status S = Saves->load(*World, ToStd(Slot), &Warnings);
	for (const auto& W : Warnings) { OutWarnings.Add(ToF(W)); UE_LOG(LogEarthFrontier, Warning, TEXT("[EF] load: %s"), *ToF(W)); }
	if (!S) UE_LOG(LogEarthFrontier, Error, TEXT("[EF] load failed: %s %s"), *ToF(S.code), *ToF(S.message));
	return S.ok;
}

void UEFWorldSubsystem::Tick(float DeltaTime) {
	if (!World || GameSecondsPerWorldTick <= 0.f) return;
	TickAccumulator += DeltaTime;
	const int32 Ticks = static_cast<int32>(TickAccumulator / GameSecondsPerWorldTick);
	if (Ticks <= 0) return;
	TickAccumulator -= Ticks * GameSecondsPerWorldTick;
	const size_t Before = World->journal().size();
	World->advanceTicks(Ticks);   // bounded catch-up inside
	BroadcastNewEvents(Before);
}

FEFCommandResult UEFWorldSubsystem::Submit(const ef::Command& Cmd) {
	FEFCommandResult R;
	if (!World) { R.Code = TEXT("no_world"); return R; }
	const size_t Before = World->journal().size();
	ef::CommandResult CR = World->apply(Cmd);
	R.bOk = CR.status.ok; R.Code = ToF(CR.status.code); R.Message = ToF(CR.status.message);
	R.CommittedEvents = static_cast<int32>(CR.committed_event_ids.size());
	if (R.bOk) BroadcastNewEvents(Before);
	else UE_LOG(LogEarthFrontier, Verbose, TEXT("[EF] command rejected: %s %s"), *R.Code, *R.Message);
	return R;
}

void UEFWorldSubsystem::BroadcastNewEvents(size_t PreviousJournalSize) {
	const auto& Events = World->journal().events();
	for (size_t i = PreviousJournalSize; i < Events.size(); ++i)
		OnWorldEvent.Broadcast(ToF(ef::eventTypeName(Events[i].event_type)), ToF(Events[i].event_id));
}

FEFCommandResult UEFWorldSubsystem::InteractNpc(const FString& NpcId) { return Submit(ef::CmdInteractNpc{ToStd(NpcId)}); }
FEFCommandResult UEFWorldSubsystem::AcceptMission(const FString& MissionId) { return Submit(ef::CmdAcceptMission{ToStd(MissionId)}); }
FEFCommandResult UEFWorldSubsystem::EnterVehicle(const FString& VehicleId, const FString& SeatId) { return Submit(ef::CmdEnterVehicle{ToStd(VehicleId), ToStd(SeatId)}); }
FEFCommandResult UEFWorldSubsystem::ExitVehicle() { return Submit(ef::CmdExitVehicle{}); }
FEFCommandResult UEFWorldSubsystem::SwitchSeat(const FString& SeatId) { return Submit(ef::CmdSwitchSeat{ToStd(SeatId)}); }
FEFCommandResult UEFWorldSubsystem::LoadCargo(const FString& NodeId, const FString& ItemId, int32 Quantity) { return Submit(ef::CmdLoadCargo{ToStd(NodeId), ToStd(ItemId), Quantity}); }
FEFCommandResult UEFWorldSubsystem::DeliverCargo(const FString& NodeId, const FString& ItemId, int32 Quantity) { return Submit(ef::CmdDeliverCargo{ToStd(NodeId), ToStd(ItemId), Quantity}); }

FEFCommandResult UEFWorldSubsystem::ReportPose(const FString& EntityId, const FTransform& T) {
	// UE: X forward, Y right, Z up, centimeters. Core: same axes in meters (content POIs are authored in meters).
	ef::Pose P;
	const FVector L = T.GetLocation(); const FRotator R = T.Rotator();
	P.position = {L.X / 100.0, L.Y / 100.0, L.Z / 100.0};
	P.rotation_euler_deg = {R.Roll, R.Pitch, R.Yaw};
	return Submit(ef::CmdReportPose{ToStd(EntityId), P});
}

int64 UEFWorldSubsystem::GetWorldTick() const { return World ? World->time().tick : 0; }
FString UEFWorldSubsystem::GetClockText() const {
	if (!World) return TEXT("--");
	const ef::WorldTime& t = World->time();
	return FString::Printf(TEXT("Day %d %02d:%02d"), t.day() + 1, t.hour(), t.minute());
}
int32 UEFWorldSubsystem::GetCredits() const { return World ? World->player().credits : 0; }
EEFPossessionMode UEFWorldSubsystem::GetPossessionMode() const { return (World && World->possession().mode == ef::PossessionMode::Seated) ? EEFPossessionMode::Seated : EEFPossessionMode::OnFoot; }
FString UEFWorldSubsystem::GetPossessedVehicleId() const { return World ? ToF(World->possession().vehicle_id) : FString(); }
FString UEFWorldSubsystem::GetRoleLabel() const { return World ? ToF(World->possession().role) : FString(); }
int32 UEFWorldSubsystem::GetLocalTrust(const FString& RegionId) const { const ef::RegionState* R = World ? World->region(ToStd(RegionId)) : nullptr; return R ? R->local_trust : 0; }
int32 UEFWorldSubsystem::GetFactionReputation(const FString& FactionId) const { const ef::FactionState* F = World ? World->faction(ToStd(FactionId)) : nullptr; return F ? F->reputation : 0; }
FString UEFWorldSubsystem::GetNpcDialogue(const FString& NpcId) const { return World ? ToF(World->npcDialogue(ToStd(NpcId))) : FString(); }
FString UEFWorldSubsystem::GetWeather(const FString& RegionId) const { const ef::WeatherState* W = World ? World->regionWeather(ToStd(RegionId)) : nullptr; return W ? ToF(W->condition) : FString(); }
FString UEFWorldSubsystem::GetDebugSummary() const { return World ? ToF(World->debugSummary()) : TEXT("no world"); }
TArray<FEFNewsItem> UEFWorldSubsystem::GetLatestNews(int32 Count) const {
	TArray<FEFNewsItem> Out;
	if (!World) return Out;
	const auto& News = World->news();
	for (int32 i = FMath::Max(0, static_cast<int32>(News.size()) - Count); i < static_cast<int32>(News.size()); ++i) {
		FEFNewsItem N; N.Headline = ToF(News[i].headline); N.Tick = News[i].tick; N.SourceEventId = ToF(News[i].source_event_id);
		Out.Add(N);
	}
	return Out;
}
