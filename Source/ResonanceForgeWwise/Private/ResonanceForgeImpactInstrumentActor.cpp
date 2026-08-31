#include "ResonanceForgeImpactInstrumentActor.h"

#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "MIDIDeviceManager.h"
#include "ResonanceForgeSynthComponent.h"
#include "ResonanceForgeWwiseBridgeComponent.h"

AResonanceForgeImpactInstrumentActor::AResonanceForgeImpactInstrumentActor()
{
    PrimaryActorTick.bCanEverTick = false;

    InstrumentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("共振体"));
    SetRootComponent(InstrumentMesh);
    InstrumentMesh->SetCollisionProfileName(TEXT("BlockAll"));
    InstrumentMesh->SetNotifyRigidBodyCollision(true);
    InstrumentMesh->OnComponentHit.AddDynamic(this, &AResonanceForgeImpactInstrumentActor::HandleMeshHit);

    NativeSynth = CreateDefaultSubobject<UResonanceForgeSynthComponent>(TEXT("原生模态合成"));
    NativeSynth->SetupAttachment(InstrumentMesh);

    WwiseBridge = CreateDefaultSubobject<UResonanceForgeWwiseBridgeComponent>(TEXT("Wwise桥接"));

}

void AResonanceForgeImpactInstrumentActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    NativeSynth->SetSynthesisModel(SynthesisModel);
    NativeSynth->LoadBuiltInPreset(ResonancePreset);
}

void AResonanceForgeImpactInstrumentActor::BeginPlay()
{
    Super::BeginPlay();
    NativeSynth->SetSynthesisModel(SynthesisModel);
    NativeSynth->LoadBuiltInPreset(ResonancePreset);

    if (bEnableKeyboardTrigger)
    {
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            EnableInput(PlayerController);
            if (InputComponent)
            {
                InputComponent->BindKey(KeyboardTriggerKey, IE_Pressed, this,
                    &AResonanceForgeImpactInstrumentActor::HandleKeyboardTrigger);
            }
        }
    }
}

void AResonanceForgeImpactInstrumentActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisconnectMidiInput();
    Super::EndPlay(EndPlayReason);
}

float AResonanceForgeImpactInstrumentActor::ComputeImpactEnergy(
    const float ImpulseMagnitude,
    const float MinimumRequiredImpulse,
    const float Sensitivity)
{
    return FMath::Clamp((ImpulseMagnitude - FMath::Max(0.0f, MinimumRequiredImpulse)) * FMath::Max(0.0f, Sensitivity), 0.0f, 1.0f);
}

float AResonanceForgeImpactInstrumentActor::ComputeImpactBrightness(const float RelativeSpeed)
{
    return FMath::Clamp(FMath::Sqrt(FMath::Max(0.0f, RelativeSpeed) / 2400.0f), 0.12f, 1.0f);
}

bool AResonanceForgeImpactInstrumentActor::ListenModeIncludesNative(const EResonanceForgeListenMode Mode)
{
    return Mode == EResonanceForgeListenMode::NativeOnly || Mode == EResonanceForgeListenMode::Layered;
}

bool AResonanceForgeImpactInstrumentActor::ListenModeIncludesWwise(const EResonanceForgeListenMode Mode)
{
    return Mode == EResonanceForgeListenMode::WwiseOnly || Mode == EResonanceForgeListenMode::Layered;
}

float AResonanceForgeImpactInstrumentActor::ShapePerformanceVelocity(
    const float NormalizedVelocity,
    const EResonanceVelocityCurve Curve)
{
    const float Input = FMath::Clamp(NormalizedVelocity, 0.0f, 1.0f);
    switch (Curve)
    {
    case EResonanceVelocityCurve::SoftTouch:
        return FMath::Pow(Input, 0.62f);
    case EResonanceVelocityCurve::HeavyHand:
        return FMath::Pow(Input, 1.75f);
    default:
        return Input;
    }
}

int32 AResonanceForgeImpactInstrumentActor::TriggerInstrument(
    const float Energy,
    const float Brightness,
    const int32 MidiNote,
    const float StrikePosition,
    const bool bHoldNativeNote,
    const float BowPressureOverride)
{
    FResonanceForgeImpactParameters Parameters;
    Parameters.Energy = FMath::Clamp(Energy, 0.0f, 1.0f);
    Parameters.Brightness = FMath::Clamp(Brightness, 0.0f, 1.0f);
    Parameters.ObjectSize = FMath::Clamp(ObjectSize, 0.0f, 1.0f);
    Parameters.MidiNote = FMath::Clamp(MidiNote, 0, 127);
    Parameters.StrikePosition = FMath::Clamp(StrikePosition < 0.0f ? ManualStrikePosition : StrikePosition, 0.0f, 1.0f);
    LastStrikePosition = Parameters.StrikePosition;
    LastImpactEnergy = Parameters.Energy;
    LastImpactBrightness = Parameters.Brightness;
    ++ImpactSerial;
    Parameters.MaterialPreset = NativeSynth && NativeSynth->MaterialProfile
        ? NativeSynth->MaterialProfile->SourcePreset
        : ResonancePreset;

    NativeSynth->PitchScale = FMath::Lerp(1.35f, 0.72f, Parameters.ObjectSize);

    LastTriggerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    LastImpactWorldSeconds = static_cast<float>(LastTriggerTime);
    int32 PlayingId = 0;
    if (ListenModeIncludesNative(ListenMode) && NativeSynth)
    {
        if (bHoldNativeNote)
        {
            NativeSynth->NoteOn(
                Parameters.Energy,
                Parameters.Brightness,
                Parameters.MidiNote,
                Parameters.StrikePosition,
                MidiBowPressure);
        }
        else
        {
            NativeSynth->Strike(
                Parameters.Energy,
                Parameters.Brightness,
                Parameters.MidiNote,
                Parameters.StrikePosition,
                BowPressureOverride);
        }
    }
    if (ListenModeIncludesWwise(ListenMode) && WwiseBridge)
    {
        PlayingId = WwiseBridge->TriggerImpact(Parameters, nullptr);
    }
    const FLinearColor FlashColor = ResonancePreset == TEXT("硬木")
        ? FLinearColor(1.0f, 0.24f, 0.035f)
        : ResonancePreset == TEXT("薄玻璃")
            ? FLinearColor(0.04f, 1.0f, 0.76f)
            : FLinearColor(0.02f, 0.48f, 1.0f);
    if (GetWorld())
    {
        if (APointLight* Flash = GetWorld()->SpawnActor<APointLight>(GetActorLocation() + FVector(0.0f, 0.0f, 95.0f), FRotator::ZeroRotator))
        {
            Flash->PointLightComponent->SetLightColor(FlashColor);
            Flash->PointLightComponent->SetIntensity(FMath::Lerp(1800.0f, 9500.0f, Parameters.Energy));
            Flash->PointLightComponent->SetAttenuationRadius(520.0f);
            Flash->PointLightComponent->SetCastShadows(false);
            Flash->SetLifeSpan(0.18f);
        }
    }
    OnImpactTriggered.Broadcast(Parameters.Energy, Parameters.Brightness, Parameters.ObjectSize, Parameters.MidiNote);
    return PlayingId;
}

float AResonanceForgeImpactInstrumentActor::ComputeNormalizedStrikePosition(const FVector& WorldImpactPoint) const
{
    if (!InstrumentMesh || !InstrumentMesh->GetStaticMesh())
    {
        return 0.5f;
    }
    const FBoxSphereBounds LocalBounds = InstrumentMesh->GetStaticMesh()->GetBounds();
    const FVector LocalPoint = InstrumentMesh->GetComponentTransform().InverseTransformPosition(WorldImpactPoint);
    const bool bUseX = LocalBounds.BoxExtent.X >= LocalBounds.BoxExtent.Y;
    const float HalfLength = FMath::Max(1.0f, bUseX ? LocalBounds.BoxExtent.X : LocalBounds.BoxExtent.Y);
    const float CenteredPosition = bUseX
        ? LocalPoint.X - LocalBounds.Origin.X
        : LocalPoint.Y - LocalBounds.Origin.Y;
    return FMath::Clamp((CenteredPosition + HalfLength) / (2.0f * HalfLength), 0.0f, 1.0f);
}

void AResonanceForgeImpactInstrumentActor::HandleMeshHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    const FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!GetWorld() || GetWorld()->GetTimeSeconds() - LastTriggerTime < RetriggerCooldownSeconds)
    {
        return;
    }

    const float Energy = ComputeImpactEnergy(NormalImpulse.Size(), MinimumImpulse, ImpulseSensitivity);
    if (Energy <= 0.0f)
    {
        return;
    }

    const FVector OtherVelocity = OtherComponent ? OtherComponent->GetComponentVelocity() : FVector::ZeroVector;
    const float RelativeSpeed = (OtherVelocity - InstrumentMesh->GetComponentVelocity()).Size();
    TriggerInstrument(Energy, ComputeImpactBrightness(RelativeSpeed), 60, ComputeNormalizedStrikePosition(Hit.ImpactPoint));
}

void AResonanceForgeImpactInstrumentActor::HandleKeyboardTrigger()
{
    TriggerInstrument(0.78f, 0.58f, 60);
}

bool AResonanceForgeImpactInstrumentActor::ConnectMidiInput(const int32 DeviceId)
{
    DisconnectMidiInput();
    MidiController = UMIDIDeviceManager::CreateMIDIDeviceController(DeviceId, 1024);
    if (!MidiController)
    {
        return false;
    }
    MidiController->OnMIDIEvent.AddDynamic(this, &AResonanceForgeImpactInstrumentActor::HandleMidiEvent);
    return true;
}

void AResonanceForgeImpactInstrumentActor::DisconnectMidiInput()
{
    if (MidiController)
    {
        MidiController->OnMIDIEvent.RemoveDynamic(this, &AResonanceForgeImpactInstrumentActor::HandleMidiEvent);
        MidiController->ShutdownDevice();
        MidiController = nullptr;
    }
    bHasMidiAftertouch = false;
    LastMidiPressure = -1;
    MidiBowPressure = MidiBrightness;
}

bool AResonanceForgeImpactInstrumentActor::IsMidiConnected() const
{
    return MidiController != nullptr;
}

FString AResonanceForgeImpactInstrumentActor::GetConnectedMidiDeviceName() const
{
    return MidiController ? MidiController->GetDeviceName() : FString();
}

void AResonanceForgeImpactInstrumentActor::HandleMidiEvent(
    UMIDIDeviceController* Controller,
    const int32 Timestamp,
    const EMIDIEventType EventType,
    const int32 Channel,
    const int32 ControlId,
    const int32 Velocity,
    const int32 RawEventType)
{
    if (EventType == EMIDIEventType::ControlChange && ControlId == 1)
    {
        LastMidiControl = ControlId;
        LastMidiControlValue = Velocity;
        MidiBrightness = FMath::Clamp(Velocity / 127.0f, 0.0f, 1.0f);
        if (ListenModeIncludesNative(ListenMode) && NativeSynth)
        {
            NativeSynth->SetBowSpeed(MidiBrightness);
            if (!bHasMidiAftertouch)
            {
                MidiBowPressure = MidiBrightness;
                NativeSynth->SetBowPressure(MidiBowPressure);
            }
        }
        if (ListenModeIncludesWwise(ListenMode) && WwiseBridge)
        {
            WwiseBridge->SetLiveBrightness(MidiBrightness);
        }
    }
    else if (EventType == EMIDIEventType::ChannelAfterTouch || EventType == EMIDIEventType::NoteAfterTouch)
    {
        const int32 PressureValue = EventType == EMIDIEventType::ChannelAfterTouch ? ControlId : Velocity;
        MidiBowPressure = FMath::Clamp(PressureValue / 127.0f, 0.0f, 1.0f);
        LastMidiPressure = PressureValue;
        bHasMidiAftertouch = true;
        if (ListenModeIncludesNative(ListenMode) && NativeSynth)
        {
            const int32 TargetNote = EventType == EMIDIEventType::NoteAfterTouch ? ControlId : -1;
            NativeSynth->SetBowPressure(MidiBowPressure, TargetNote);
        }
    }
    else if (EventType == EMIDIEventType::NoteOn && Velocity > 0)
    {
        LastMidiNote = ControlId;
        LastMidiVelocity = Velocity;
        TriggerInstrument(ShapePerformanceVelocity(Velocity / 127.0f, VelocityCurve), MidiBrightness, ControlId, -1.0f, true);
    }
    else if (EventType == EMIDIEventType::NoteOff || (EventType == EMIDIEventType::NoteOn && Velocity == 0))
    {
        LastMidiNote = ControlId;
        LastMidiVelocity = 0;
        if (ListenModeIncludesNative(ListenMode) && NativeSynth)
        {
            NativeSynth->NoteOff(ControlId);
        }
    }
}
