# EFSky — sun, sky, fog, water and weather effects driven by the authoritative world clock and weather.
class_name EFSky
extends Node3D

var sun: DirectionalLight3D
var env: WorldEnvironment
var water: MeshInstance3D
var rain: GPUParticles3D
var _hour := 8.0
var _weather := "clear"

const WATER_SHADER := """
shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;
uniform vec4 shallow : source_color = vec4(0.16, 0.5, 0.58, 0.82);
uniform vec4 deep : source_color = vec4(0.03, 0.17, 0.36, 0.96);
uniform float wave_height = 0.5;
uniform float storm = 0.0;
void vertex() {
	float a = wave_height * (1.0 + storm * 2.5);
	VERTEX.y += sin(VERTEX.x * 0.045 + TIME * 1.1) * a + cos(VERTEX.z * 0.06 + TIME * 0.9) * a * 0.6 + sin((VERTEX.x + VERTEX.z) * 0.02 + TIME * 0.4) * a * 0.8;
}
void fragment() {
	float fres = pow(1.0 - clamp(dot(NORMAL, VIEW), 0.0, 1.0), 2.5);
	ALBEDO = mix(shallow.rgb, deep.rgb, fres) * (1.0 - storm * 0.25);
	ALPHA = mix(shallow.a, deep.a, fres);
	ROUGHNESS = 0.08 + storm * 0.3;
	SPECULAR = 0.6;
}
"""

func _ready() -> void:
	sun = DirectionalLight3D.new()
	sun.shadow_enabled = true
	sun.light_energy = 1.2
	add_child(sun)
	env = WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var mat := ProceduralSkyMaterial.new()
	mat.sky_top_color = Color(0.25, 0.45, 0.8)
	mat.sky_horizon_color = Color(0.7, 0.8, 0.9)
	mat.ground_bottom_color = Color(0.2, 0.25, 0.3)
	mat.ground_horizon_color = Color(0.6, 0.7, 0.8)
	sky.sky_material = mat
	e.sky = sky
	e.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	e.fog_enabled = true
	e.fog_density = 0.0004
	e.fog_light_color = Color(0.7, 0.78, 0.85)
	env.environment = e
	add_child(env)
	# water
	water = MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(6400, 5200)
	pm.subdivide_width = 96
	pm.subdivide_depth = 80
	water.mesh = pm
	water.position = Vector3(600, 0.0, -400)
	var sh := Shader.new()
	sh.code = WATER_SHADER
	var sm := ShaderMaterial.new()
	sm.shader = sh
	water.material_override = sm
	add_child(water)
	# rain (particles are cheap to keep around; emitting toggled by weather)
	rain = GPUParticles3D.new()
	var pmat := ParticleProcessMaterial.new()
	pmat.direction = Vector3(0.1, -1, 0)
	pmat.initial_velocity_min = 18.0
	pmat.initial_velocity_max = 24.0
	pmat.gravity = Vector3(0, -9.8, 0)
	pmat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	pmat.emission_box_extents = Vector3(60, 2, 60)
	rain.process_material = pmat
	var drop := BoxMesh.new()
	drop.size = Vector3(0.03, 0.5, 0.03)
	var dm := StandardMaterial3D.new()
	dm.albedo_color = Color(0.75, 0.82, 0.95, 0.6)
	dm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	drop.material = dm
	rain.draw_pass_1 = drop
	rain.amount = 1500
	rain.lifetime = 2.5
	rain.emitting = false
	add_child(rain)
	World.state_updated.connect(_on_state)

func _on_state(st: Dictionary) -> void:
	var t: Dictionary = st.get("time", {})
	_hour = float(t.get("hour", 8)) + float(t.get("minute", 0)) / 60.0
	var regions: Array = st.get("regions", [])
	var region_id: String = st.get("player", {}).get("region_id", "")
	for r in regions:
		if r.get("id", "") == region_id:
			_weather = str(r.get("weather", "clear"))
	_apply()

func _apply() -> void:
	# sun: rises 06:00, sets 20:00 (game clock); night keeps a dim moon-like light
	var day_t: float = clamp((_hour - 6.0) / 14.0, 0.0, 1.0)
	var elevation := sin(day_t * PI) * 75.0
	var is_day := _hour >= 6.0 and _hour <= 20.0
	sun.rotation_degrees = Vector3(-(elevation if is_day else 25.0), 35.0 + day_t * 110.0, 0)
	var storm := 1.0 if _weather == "storm" else (0.5 if _weather == "rain" else 0.0)
	sun.light_energy = (0.15 + 1.1 * sin(day_t * PI) if is_day else 0.08) * (1.0 - storm * 0.55)
	sun.light_color = Color(1.0, 0.95, 0.85).lerp(Color(0.9, 0.6, 0.4), 1.0 - sin(day_t * PI)) if is_day else Color(0.5, 0.6, 0.8)
	var e := env.environment
	e.fog_density = 0.0004 + storm * 0.0025
	e.ambient_light_energy = (1.0 if is_day else 0.25) * (1.0 - storm * 0.3)
	var sm: ShaderMaterial = water.material_override
	sm.set_shader_parameter("storm", storm)
	rain.emitting = storm > 0.0

func follow(pos: Vector3) -> void:
	rain.position = pos + Vector3(0, 25, 0)
	# underwater look: dense blue fog when the camera is below the surface
	var cam := get_viewport().get_camera_3d()
	if cam != null:
		var under := cam.global_position.y < -0.3
		env.environment.fog_density = 0.03 if under else env.environment.fog_density
		env.environment.fog_light_color = Color(0.1, 0.3, 0.45) if under else Color(0.7, 0.78, 0.85)
