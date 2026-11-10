# EFAIDriver — input provider for EFVehicle (master prompt §17.3 / supplement §12: AI uses the same movement
# model as the player, with its own decision logic). Follows a looping route of waypoints with simple steering.
class_name EFAIDriver
extends Node

var route: Array[Vector3] = []
var index := 0
var arrive_radius := 12.0
var cruise := 1.0            # throttle 0..1
var _axes := {}

func setup(points: Array[Vector3], throttle := 1.0) -> void:
	route = points
	cruise = throttle

func axis(pos: String, neg: String) -> float:
	return float(_axes.get(pos, 0.0)) - float(_axes.get(neg, 0.0))

# Called by the vehicle each physics frame with its own transform/speed.
func think(vehicle: Node3D, speed: float) -> void:
	_axes.clear()
	if route.is_empty():
		return
	var target: Vector3 = route[index]
	var pos := vehicle.global_position
	var flat := Vector2(target.x - pos.x, target.z - pos.z)
	if flat.length() < arrive_radius:
		index = (index + 1) % route.size()
		return
	var forward := -vehicle.global_transform.basis.z
	var desired := Vector3(flat.x, 0, flat.y).normalized()
	var cross := forward.cross(desired).y
	var dot := forward.dot(desired)
	var steer := clampf(-cross * 2.0, -1.0, 1.0)     # positive = steer right (vehicle yaw decreases with steer)
	if steer > 0.05:
		_axes["steer_right"] = abs(steer)
	elif steer < -0.05:
		_axes["steer_left"] = abs(steer)
	var want := cruise * (0.35 if dot < 0.5 else 1.0)
	if speed > 0.1 and dot < 0.0:
		want = 0.2
	_axes["accelerate"] = want
