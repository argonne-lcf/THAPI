#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent
PLUGIN = ROOT / "build/libthapi-fi.so"
TEST_DIR = ROOT / "test"

TIMEOUT = 30
PREFIX = "[THAPI libfabric] fi_send"


def main():
    if not PLUGIN.is_file():
        raise RuntimeError(f"Plugin not found: {PLUGIN}")

    TEST_DIR.mkdir(exist_ok=True)

    env = os.environ.copy()
    env["FI_PROVIDER_PATH"] = f"+{PLUGIN}"
    env["FI_HOOK"] = "thapi"

    command = [
        "fi_pingpong",
        "-p", "cxi",
        "-d", "cxi0",
        "-e", "rdm",
        "-m", "msg",
        "-I", "3",
        "-S", "256",
        "-c",
    ]

    server_log = TEST_DIR / "server-cxi.trace"
    client_log = TEST_DIR / "client-cxi.trace"

    with server_log.open("w") as server_err, \
         client_log.open("w") as client_err:

        server = subprocess.Popen(
            command, cwd=ROOT, env=env, stderr=server_err
        )

        try:
            # Give the server time to start its control listener.
            time.sleep(1)

            if server.poll() is not None:
                raise RuntimeError(
                    "Server exited before client startup; "
                    "see test/server-cxi.trace"
                )

            client = subprocess.run(
                command + ["127.0.0.1"],
                cwd=ROOT,
                env=env,
                stderr=client_err,
                timeout=TIMEOUT,
            )

            if client.returncode != 0:
                raise RuntimeError(
                    f"Client failed ({client.returncode}); "
                    "see test/client-cxi.trace"
                )

            if server.wait(timeout=TIMEOUT) != 0:
                raise RuntimeError(
                    "Server failed; see test/server-cxi.trace"
                )

        finally:
            # Do not leave a waiting server behind after failure.
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()

    # Communication success alone does not prove interception.
    for log in (server_log, client_log):
        text = log.read_text()
        entries = text.count(f"{PREFIX} entry ")
        exits = text.count(f"{PREFIX} exit ")

        if entries == 0 or entries != exits:
            raise RuntimeError(
                f"{log.name}: expected paired trace messages, "
                f"found {entries} entries and {exits} exits"
            )

        print(f"PASS: {log.name}: {entries} send entry/exit pairs")

    print("PASS: CXI communication completed and the hook intercepted fi_send.")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        print(f"FAIL: {error}")
        raise SystemExit(1)