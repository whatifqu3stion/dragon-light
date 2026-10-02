#!/usr/bin/env python3
"""Preview the exact embedded control page with example API responses."""

import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs


HEADER = Path(__file__).resolve().parents[1] / "include" / "web_ui.h"
HTML = HEADER.read_text(encoding="utf-8").split('R"HTML(', 1)[1].split(')HTML";', 1)[0]
STATUS = {
    "brightness": 64,
    "mode": "auto",
    "rotation": "R",
    "source": "calendar",
    "time": "2026-10-02 08:42:18",
    "ip": "192.0.2.1",
}
MODES = {"auto": "auto", "off": "off", "spin": "spin preview", "celebrate": "celebration preview"}


class PreviewHandler(BaseHTTPRequestHandler):
    def respond(self, code, content_type, body):
        data = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def send_status(self):
        self.respond(200, "application/json", json.dumps(STATUS))

    def do_GET(self):
        if self.path == "/":
            self.respond(200, "text/html; charset=utf-8", HTML)
        elif self.path == "/api/status":
            self.send_status()
        elif self.path == "/favicon.ico":
            self.respond(204, "text/plain", "")
        else:
            self.respond(404, "text/plain", "Not found")

    def do_POST(self):
        size = int(self.headers.get("Content-Length", "0"))
        if size > 1024:
            self.respond(413, "text/plain", "Request too large")
            return
        settings = parse_qs(self.rfile.read(size).decode("utf-8"))
        value = settings.get("value", [""])[0]
        if self.path == "/api/brightness":
            if not value.isascii() or not value.isdecimal() or not 5 <= int(value) <= 128:
                self.respond(400, "text/plain", "Brightness must be 5 through 128.")
                return
            STATUS["brightness"] = int(value)
        elif self.path == "/api/mode":
            if value not in MODES:
                self.respond(400, "text/plain", "Unknown display mode.")
                return
            STATUS["mode"] = MODES[value]
        else:
            self.respond(404, "text/plain", "Not found")
            return
        self.send_status()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8040)
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), PreviewHandler)
    print(f"Example UI preview: http://127.0.0.1:{server.server_port}", flush=True)
    print("Simulated status and controls only. No device connection. Press Ctrl+C to stop.", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
