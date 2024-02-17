from pathlib import Path
from shutil import rmtree

from invoke import task

KEY_DIR = Path.cwd().joinpath("keys")
PRIV_KEY = KEY_DIR.joinpath("fw_private_key.pem")
PUB_KEY = KEY_DIR.joinpath("fw_public_key.pem")

FW_DIR = Path.cwd().joinpath("zig-out").joinpath("firmware")
SIG_FW_DIR = Path.cwd().joinpath("signed")

LWM2M_BIN = FW_DIR.joinpath("lwm2m.bin")
MQTT_BIN = FW_DIR.joinpath("mqtt.bin")

LWM2M_SIG_BIN = SIG_FW_DIR.joinpath("lwm2m_sig.bin")
MQTT_SIG_BIN = SIG_FW_DIR.joinpath("mqtt_sig.bin")

IMG_TOOL = (
    Path.cwd()
    .joinpath("csrc")
    .joinpath("mcuboot")
    .joinpath("mcuboot")
    .joinpath("scripts")
    .joinpath("imgtool.py")
)


@task
def clean(c):
    if KEY_DIR.exists():
        rmtree(KEY_DIR)
    if SIG_FW_DIR.exists():
        rmtree(SIG_FW_DIR)


@task
def key_dir(c):
    if not KEY_DIR.exists():
        KEY_DIR.mkdir()


@task(pre=[key_dir])
def private_key(c):
    c.run(f"openssl ecparam -genkey -name prime256v1 -noout -out {PRIV_KEY}")


@task(pre=[private_key])
def public_key(c):
    c.run(f"openssl ec -in {PRIV_KEY} -pubout -out {PUB_KEY}")


@task
def sig_fw_dir(c):
    if not SIG_FW_DIR.exists():
        SIG_FW_DIR.mkdir()


@task(pre=[public_key, sig_fw_dir])
def sig_fw_images(c):
    c.run(
        f'python {IMG_TOOL} sign -v "0.1.2" -F 0x40000 -R 0xff --header-size 0x80 --pad-header -k {PRIV_KEY} --overwrite-only --public-key-format full -S 0xB0000 --align 4 {LWM2M_BIN} {LWM2M_SIG_BIN}'
    )
    c.run(f"python {IMG_TOOL} dumpinfo {LWM2M_SIG_BIN}")
