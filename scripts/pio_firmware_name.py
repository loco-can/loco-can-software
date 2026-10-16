"""Name the PlatformIO firmware image after the Loco-CAN module.

build_flags select the module, for example::

    -D MODULE=PROGRAMMER_MODULE
    -D PROGRAMMER_MODULE_VERSION=V_3_0_0

This pre-script turns that pair into PROGNAME ``programmer_v3_0_0``.
``V_2_1_12`` becomes ``controller_v2_1_12``. ESP32 links a ``.bin`` and AVR
links a ``.hex``.

Dots are avoided on purpose: SCons treats the last ``.`` in PROGNAME as a
suffix and would otherwise emit ``programmer_v3.hex``.

Load this file with ``extra_scripts = pre:scripts/pio_firmware_name.py``.
The ESP32 and AVR builders keep a custom PROGNAME only when it is set
before their own script runs.
"""

import re

try:
    Import("env")
except NameError:
    env = None

_DEFINE = re.compile(r"(?:^|\s)-D\s*([A-Za-z_][A-Za-z0-9_]*)(?:=(\S+))?")
_VERSION = re.compile(r"V_(\d+)_(\d+)_(\d+)")


def defines_from_flags(flags):
    if isinstance(flags, (list, tuple)):
        text = "\n".join(str(item) for item in flags)
    else:
        text = str(flags or "")
    found = {}
    for name, value in _DEFINE.findall(text):
        found[name] = value.strip("\"'") if value else None
    return found


def firmware_progname(flags):
    """Return ``<module>_v<major>_<minor>_<patch>`` or None when flags are incomplete."""
    defines = defines_from_flags(flags)
    module = defines.get("MODULE")
    if not module:
        return None
    version = defines.get(module + "_VERSION")
    if not version:
        return None

    short = module[: -len("_MODULE")] if module.endswith("_MODULE") else module
    match = _VERSION.fullmatch(version)
    if match is None:
        return None
    return "{0}_v{1}_{2}_{3}".format(
        short.lower(), match.group(1), match.group(2), match.group(3)
    )


def image_extension(platform):
    platform = str(platform or "")
    if platform.startswith("atmelavr"):
        return ".hex"
    if platform.startswith("espressif32"):
        return ".bin"
    return None


if env is not None:
    progname = firmware_progname(env.GetProjectOption("build_flags", ""))
    if progname:
        env.Replace(PROGNAME=progname)
        extension = image_extension(env.GetProjectOption("platform", ""))
        suffix = extension or ""
        print("Firmware image: {0}{1}".format(progname, suffix))
    else:
        print(
            "pio_firmware_name: MODULE and <MODULE>_VERSION are not both set; "
            "keeping the default firmware name"
        )
