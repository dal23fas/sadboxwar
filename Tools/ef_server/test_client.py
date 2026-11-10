#!/usr/bin/env python3
"""Smoke test for ef_server: speaks the same JSON-lines protocol as godot/scripts/world.gd.
Usage: test_client.py <content.json> [port]   (server must already be running)
Exit 0 on success."""
import json, socket, sys

def main():
    content = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 7777
    s = socket.create_connection(("127.0.0.1", port), timeout=5)
    f = s.makefile("rwb")
    seq = 0
    def call(op, **args):
        nonlocal seq
        seq += 1
        f.write((json.dumps({"id": seq, "op": op, "args": args}) + "\n").encode()); f.flush()
        resp = json.loads(f.readline())
        assert resp["id"] == seq, resp
        return resp
    r = call("ping"); assert r["ok"] and r["result"]["core"] == "EFWorldCore", r
    r = call("start_new_game", content_path=content); assert r["ok"], r
    r = call("apply", command={"type": "AcceptMission", "mission_id": "mission_first_contact"}); assert r["ok"], r
    r = call("apply", command={"type": "InteractNpc", "npc_id": "npc_harbor_master"}); assert not r["ok"] and r["code"] == "too_far", r
    r = call("apply", command={"type": "ReportPose", "entity_id": "player_main", "pose": {"position": [-600, 150, 0], "rotation": [0, 0, 0]}}); assert r["ok"], r
    r = call("apply", command={"type": "InteractNpc", "npc_id": "npc_harbor_master"}); assert r["ok"], r
    r = call("advance_ticks", ticks=30); assert r["ok"] and r["result"]["tick"] == 30, r
    r = call("state", news_count=5); assert r["ok"], r
    st = r["result"]
    assert st["time"]["clock_text"] == "Day 1 08:30", st["time"]
    assert any(m["mission_id"] == "mission_first_contact" and m["current_objective_index"] == 2 for m in st["missions"])
    r = call("save", slot="tcp_smoke"); assert r["ok"], r
    r = call("load", slot="tcp_smoke"); assert r["ok"] and r["result"]["world_tick"] == 30, r
    # a second connection sees the same world (server keeps state across reconnects)
    f.close(); s.close()
    s = socket.create_connection(("127.0.0.1", port), timeout=5); f = s.makefile("rwb")
    r = call("state"); assert r["ok"] and r["result"]["time"]["tick"] == 30, r
    f.write(b"quit\n"); f.flush()
    print("ef_server smoke test: PASS (tick=30, mission objective index=2, save/load ok, reconnect ok)")

if __name__ == "__main__":
    main()
