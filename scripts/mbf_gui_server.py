#!/usr/bin/env python3
"""Local-only HTTP bridge between the Parametric Lab and mbf_build_house."""
from __future__ import annotations
import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
GUI_DIR = ROOT / "gui"
NUMERIC_RANGES = {
    "width": (8.0, 100.0), "depth": (7.0, 100.0),
    "south": (0.0, 100.0), "north": (0.0, 100.0),
    "east": (0.0, 100.0), "west": (0.0, 100.0),
    "floors": (1, 200), "bedrooms": (1, 4), "bathrooms": (1, 4), "seed": (0, 2**63 - 1),
}
PROFILES = ("balanced", "open", "privacy")


def parse_brief(payload):
    if not isinstance(payload, dict):
        raise ValueError("Request body must be a JSON object.")
    values = {}
    for key, (low, high) in NUMERIC_RANGES.items():
        value = payload.get(key)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError(f"{key} must be a number.")
        if not math.isfinite(value) or value < low or value > high:
            raise ValueError(f"{key} must be between {low} and {high}.")
        if key in ("floors", "bedrooms", "seed") and int(value) != value:
            raise ValueError(f"{key} must be an integer.")
        values[key] = int(value) if key in ("floors", "bedrooms", "seed") else float(value)
    profile = payload.get("profile", "all")
    if profile not in ("all", "best", *PROFILES):
        raise ValueError("profile must be all, best, balanced, open, or privacy.")
    if values["west"] + values["east"] >= values["width"]:
        raise ValueError("East and west setbacks leave no buildable width.")
    if values["south"] + values["north"] >= values["depth"]:
        raise ValueError("North and south setbacks leave no buildable depth.")
    return values, profile


def generate(executable: Path, values, profile: str):
    with tempfile.TemporaryDirectory(prefix="mbf-lab-") as temp:
        output = Path(temp) / "candidate.svg"
        args = [
            str(executable),
            "--width", str(values["width"]), "--depth", str(values["depth"]),
            "--setback-south", str(values["south"]), "--setback-north", str(values["north"]),
            "--setback-east", str(values["east"]), "--setback-west", str(values["west"]),
            "--floors", str(values["floors"]), "--bedrooms", str(values["bedrooms"]),
            "--seed", str(values["seed"]), "--profile", profile, "--output", str(output),
        ]
        completed = subprocess.run(args, cwd=ROOT, capture_output=True, text=True,
                                   timeout=90, check=False)
        result = {
            "profile": profile,
            "ok": completed.returncode == 0 and output.is_file(),
            "stdout": completed.stdout[-12000:],
            "error": completed.stderr[-12000:] if completed.returncode else "",
            "svg": output.read_text(encoding="utf-8") if completed.returncode == 0 and output.is_file() else "",
        }
        return result


def generate_topology_preview(executable: Path, bedrooms: int, bathrooms: int):
    with tempfile.TemporaryDirectory(prefix="mbf-topology-") as temp:
        output = Path(temp) / "topologies.svg"
        completed = subprocess.run(
            [str(executable), "--bedrooms", str(bedrooms), "--bathrooms", str(bathrooms), "--output", str(output)],
            cwd=ROOT, capture_output=True, text=True, timeout=45, check=False,
        )
        return {
            "svg": output.read_text(encoding="utf-8") if completed.returncode == 0 and output.is_file() else "",
            "report": completed.stdout[-6000:],
            "error": completed.stderr[-6000:] if completed.returncode else "",
        }


def generate_topology_plans(executable: Path, values):
    with tempfile.TemporaryDirectory(prefix="mbf-topology-plans-") as temp:
        prefix = Path(temp) / "topology_candidate"
        args = [
            str(executable), "--width", str(values["width"]), "--depth", str(values["depth"]),
            "--setback-south", str(values["south"]), "--setback-north", str(values["north"]),
            "--setback-east", str(values["east"]), "--setback-west", str(values["west"]),
            "--bedrooms", str(values["bedrooms"]), "--bathrooms", str(values["bathrooms"]), "--floors", str(values["floors"]), "--seed", str(values["seed"]),
            "--output-prefix", str(prefix),
        ]
        completed = subprocess.run(args, cwd=ROOT, capture_output=True, text=True,
                                   timeout=90, check=False)
        candidates = []
        for index, path in enumerate(sorted(Path(temp).glob("topology_candidate_*.svg")), start=1):
            strategy = f"Topology candidate {index}"
            metrics = {}
            for line in completed.stdout.splitlines():
                if line.startswith(f"Candidate {index}:"):
                    fields = [part.strip() for part in line.split("|")]
                    strategy = fields[0].split(":", 1)[1].strip()
                    for key in ("daylight", "privacy", "circulation", "overall", "doors", "windows", "openings"):
                        for field in fields[1:]:
                            if field.startswith(key + " "):
                                try:
                                    metrics[key] = float(field[len(key) + 1:])
                                except ValueError:
                                    pass
                        for field in fields[1:]:
                            if field.startswith("accessible "):
                                metrics["accessible"] = field[len("accessible "):]
                    break
            candidates.append({
                "profile": strategy,
                "quality": metrics,
                "svg": path.read_text(encoding="utf-8"),
            })
        return {
            "candidates": candidates,
            "report": completed.stdout[-10000:],
            "error": completed.stderr[-10000:] if completed.returncode else "",
        }


class Handler(BaseHTTPRequestHandler):
    executable: Path

    def send_bytes(self, status, data, content_type):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = urlparse(self.path).path
        if path in ("/", "/index.html"):
            target = GUI_DIR / "index.html"
            if target.is_file():
                self.send_bytes(200, target.read_bytes(), "text/html; charset=utf-8")
            else:
                self.send_bytes(500, b"gui/index.html is missing", "text/plain; charset=utf-8")
        elif path == "/health":
            self.send_bytes(200, b'{"status":"ok","engine":"mbf_build_house"}', "application/json")
        else:
            self.send_bytes(404, b"Not found", "text/plain; charset=utf-8")

    def do_POST(self):
        if urlparse(self.path).path != "/api/generate":
            self.send_bytes(404, b'{"error":"Not found"}', "application/json")
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 32_768:
                raise ValueError("Request body must be between 1 and 32768 bytes.")
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
            values, profile = parse_brief(payload)
            requested = PROFILES if profile == "all" else (profile,)
            candidates = [generate(self.executable, values, item) for item in requested]
            good = [candidate for candidate in candidates if candidate["ok"]]
            topology = generate_topology_preview(self.topology_executable, values["bedrooms"], values["bathrooms"])
            topology_plans = generate_topology_plans(self.topology_house_executable, values)
            message = "" if good else "The legacy profile generator did not produce a valid floor plan. See diagnostics."
            body = json.dumps({
                "candidates": candidates, "error": message,
                "topology_svg": topology["svg"],
                "topology_report": topology["report"],
                "topology_error": topology["error"],
                "topology_plans": topology_plans["candidates"],
                "topology_plan_report": topology_plans["report"],
                "topology_plan_error": topology_plans["error"],
            }, ensure_ascii=False).encode("utf-8")
            self.send_bytes(200, body, "application/json; charset=utf-8")
        except subprocess.TimeoutExpired:
            self.send_bytes(504, b'{"error":"Generation exceeded the 90-second time limit."}', "application/json")
        except (ValueError, json.JSONDecodeError, UnicodeDecodeError) as exc:
            body = json.dumps({"error": str(exc)}, ensure_ascii=False).encode("utf-8")
            self.send_bytes(400, body, "application/json; charset=utf-8")
        except Exception as exc:
            body = json.dumps({"error": f"Local generation service error: {exc}"}, ensure_ascii=False).encode("utf-8")
            self.send_bytes(500, body, "application/json; charset=utf-8")

    def log_message(self, fmt, *args):
        print("[Mayhem Lab] " + fmt % args)


def main():
    parser = argparse.ArgumentParser(description="Run the local Mayhem Building Forge Parametric Lab.")
    parser.add_argument("--exe", default="build/Release/mbf_build_house.exe",
                        help="Path to the compiled mbf_build_house executable.")
    parser.add_argument("--topology-exe", default="build/Release/mbf_topology_preview.exe",
                        help="Path to the compiled topology preview executable.")
    parser.add_argument("--topology-house-exe", default="build/Release/mbf_generate_topology_house.exe",
                        help="Path to the topology-driven floor-plan generator executable.")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    executable = (ROOT / args.exe).resolve() if not Path(args.exe).is_absolute() else Path(args.exe)
    topology_executable = (ROOT / args.topology_exe).resolve() if not Path(args.topology_exe).is_absolute() else Path(args.topology_exe)
    if not executable.is_file():
        raise SystemExit(f"Generator executable not found: {executable}\nBuild it first with: cmake --build build --config Release")
    topology_house_executable = (ROOT / args.topology_house_exe).resolve() if not Path(args.topology_house_exe).is_absolute() else Path(args.topology_house_exe)
    if not topology_executable.is_file():
        raise SystemExit(f"Topology preview executable not found: {topology_executable}\nBuild it first with: cmake --build build --config Release")
    if not topology_house_executable.is_file():
        raise SystemExit(f"Topology house generator not found: {topology_house_executable}\nBuild it first with: cmake --build build --config Release")
    if not (1 <= args.port <= 65535):
        raise SystemExit("Port must be between 1 and 65535.")
    Handler.executable = executable
    Handler.topology_executable = topology_executable
    Handler.topology_house_executable = topology_house_executable
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"Mayhem Parametric Lab is running at http://127.0.0.1:{args.port}")
    print(f"Native engine: {executable}")
    print("Local-only server. Press Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Mayhem Parametric Lab.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
