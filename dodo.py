"""Doit file for signing firmware images using the mcuboot image tool.

Generates a public key, signs the firmware images, and verifies the signed images.
"""
from pathlib import Path
import subprocess
import sys

# Define paths
KEY_DIR = Path.cwd() / "keys"
PRIV_KEY = KEY_DIR / "fw_private_key.pem"
PUB_KEY = KEY_DIR / "fw_public_key.pem"

FW_DIR = Path.cwd() / "zig-out" / "firmware"
SIG_FW_DIR = Path.cwd() / "signed"

LWM2M_BIN = "lwm2m.bin"
MQTT_BIN = "mqtt.bin"

LWM2M_SIG_BIN = SIG_FW_DIR / "lwm2m_sig.bin"
MQTT_SIG_BIN = SIG_FW_DIR / "mqtt_sig.bin"

IMG_TOOL = Path.cwd() / "csrc" / "mcuboot" / "mcuboot" / "scripts" / "imgtool.py"

MISO_FW_VERSION = "0.1.2"

PYTHON_EXE = sys.executable


def zig_exe():
    """Choose Zig executable. If Zig is not installed, use the Python ziglang package."""
    try:
        subprocess.run(["zig", "version"], check=True)
    except subprocess.CalledProcessError:
        return f"{PYTHON_EXE} -m ziglang"
    return "zig"


ZIG_CMD = zig_exe()


def task_public_key():
    """Generate public key."""
    return {
        "actions": [f"openssl ec -in {PRIV_KEY} -pubout -out {PUB_KEY}"],
        "file_dep": [PRIV_KEY],
        "targets": [PUB_KEY],
        "clean": True,
        "verbosity": 2,
    }


def task_fw_binaries():
    """Build firmware binaries."""
    return {
        "actions": [f"{ZIG_CMD} build"],
        "file_dep": [Path.cwd() / "build.zig"],
        "targets": [FW_DIR.joinpath(LWM2M_BIN), FW_DIR.joinpath(MQTT_BIN)],
        "clean": True,
        "verbosity": 2,
    }


def task_sig_fw_images():
    """Create signed firmware images"""
    list = [FW_DIR.joinpath(LWM2M_BIN), FW_DIR.joinpath(MQTT_BIN)]

    if not SIG_FW_DIR.exists():
        SIG_FW_DIR.mkdir()

    for bin in list:
        target_file_name = SIG_FW_DIR.joinpath(bin.stem + "_sig.bin")

        cmd1 = f"{PYTHON_EXE} {IMG_TOOL} sign -v {MISO_FW_VERSION} -F 0x40000 -R 0xff --header-size 0x80 --pad-header -k {PRIV_KEY} --overwrite-only --public-key-format full -S 0xB0000 --align 4 {bin} {target_file_name}"
        cmd2 = f"{PYTHON_EXE} {IMG_TOOL} dumpinfo {target_file_name}"
        yield {
            "name": f"Sign {bin} firmware image.",
            "actions": [cmd1, cmd2],
            "file_dep": [PUB_KEY, bin],
            "targets": [target_file_name],
            "clean": True,
            "verbosity": 2,
        }


def task_verify_fw_images():
    """Verify signed firmware images"""
    list = [FW_DIR.joinpath(LWM2M_BIN), FW_DIR.joinpath(MQTT_BIN)]
    for bin in list:
        target_file_name = SIG_FW_DIR.joinpath(bin.stem + "_sig.bin")

        yield {
            "name": f"Verify {bin} firmware image.",
            "actions": [f"{PYTHON_EXE} {IMG_TOOL} verify {LWM2M_SIG_BIN}"],
            "file_dep": [PUB_KEY, target_file_name],
            "verbosity": 2,
        }
