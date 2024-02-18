import os
from pathlib import Path
from shutil import rmtree

from invoke import task

KEY_DIR = Path.cwd() / "keys"
PRIV_KEY = KEY_DIR / "fw_private_key.pem"
PUB_KEY = KEY_DIR / "fw_public_key.pem"
SIG_FW_DIR = Path.cwd() / "signed"

MCU_BOOT_DEPS = (
    Path.cwd() / "csrc" / "mcuboot" / "mcuboot" / "scripts" / "requirements.txt"
)


@task
def get_mcuboot_deps(c):
    """Get mcuboot dependencies."""
    c.run(f"pip install --upgrade -r {MCU_BOOT_DEPS}")


@task
def clean(c):
    """Clean up"""
    if KEY_DIR.exists():
        rmtree(KEY_DIR)
    if SIG_FW_DIR.exists():
        rmtree(SIG_FW_DIR)
    if Path.cwd().joinpath("zig-out").exists():
        rmtree(Path.cwd().joinpath("zig-out"))
    if Path.cwd().joinpath("zig-cache").exists():
        rmtree(Path.cwd().joinpath("zig-cache"))


@task
def key_dir(c):
    """Create key directory."""
    if not KEY_DIR.exists():
        KEY_DIR.mkdir()


@task(pre=[key_dir])
def private_key(c):
    """Create private key."""
    c.run(f"openssl ecparam -genkey -name prime256v1 -noout -out {PRIV_KEY}")


@task
def sig_fw_dir(c):
    """Create signed firmware directory."""
    if not SIG_FW_DIR.exists():
        SIG_FW_DIR.mkdir()


@task
def sign_fw_images(c):
    """Create signed firmware images."""
    c.run("doit")


@task
def build_picolibc(c):
    """Build picolibc. Run from the root of the project in Ubuntu"""
    curr_dir = Path.cwd()
    os.chdir(Path.cwd() / "toolchain" / "picolibc")

    c.run(
        "meson setup --cross-file scripts/cross-clang-thumbv7m-none-eabi-miso.txt --optimization 2 ./build --wipe"
    )
    os.chdir("./build")
    c.run("meson compile")
    os.chdir(curr_dir)
