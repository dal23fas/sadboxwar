# EFEngineAudio — procedural engine/rotor/propeller sound (no audio files needed). Pitch follows speed/throttle,
# timbre follows the vehicle class. Attached to the possessed vehicle; silent otherwise. Placeholder for real
# sound layers (master prompt §35).
class_name EFEngineAudio
extends AudioStreamPlayer

var vehicle_class := "ground"
var level := 0.0           # 0..1 target intensity (throttle / rotor state)
var _phase := 0.0
var _phase2 := 0.0
var _gen: AudioStreamGenerator
var _playback: AudioStreamGeneratorPlayback
var _smooth := 0.0

func _ready() -> void:
	_gen = AudioStreamGenerator.new()
	_gen.mix_rate = 22050.0
	_gen.buffer_length = 0.12
	stream = _gen
	volume_db = -14.0
	bus = "Master"

func start(vclass: String) -> void:
	vehicle_class = vclass
	if not playing:
		play()
	_playback = get_stream_playback()

func stop_engine() -> void:
	stop()
	_playback = null

func _process(delta: float) -> void:
	if _playback == null:
		return
	_smooth = lerp(_smooth, clampf(level, 0.0, 1.0), minf(1.0, delta * 3.0))
	var base := 55.0
	var mix := 0.5
	match vehicle_class:
		"ground":
			base = 40.0 + _smooth * 110.0
			mix = 0.6
		"armored":
			base = 28.0 + _smooth * 60.0
			mix = 0.8
		"rotorcraft":
			base = 18.0 + _smooth * 14.0
			mix = 0.9
		"fixed_wing":
			base = 60.0 + _smooth * 160.0
			mix = 0.4
		"naval":
			base = 30.0 + _smooth * 50.0
			mix = 0.7
		"submarine":
			base = 24.0 + _smooth * 30.0
			mix = 0.95
	var amp := 0.08 + _smooth * 0.35
	var frames := _playback.get_frames_available()
	var inc := base / _gen.mix_rate
	var inc2 := base * 2.01 / _gen.mix_rate
	for i in range(frames):
		_phase = fmod(_phase + inc, 1.0)
		_phase2 = fmod(_phase2 + inc2, 1.0)
		var saw := _phase * 2.0 - 1.0
		var s := sin(_phase * TAU)
		var chop := 1.0 if vehicle_class == "rotorcraft" and _phase < 0.5 else 0.6
		var v: float = (s * (1.0 - mix) + saw * mix * 0.5 + sin(_phase2 * TAU) * 0.2) * amp * chop
		_playback.push_frame(Vector2(v, v))
