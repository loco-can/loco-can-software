Import("env")

# Arduino-ESP32 LittleFS is compiled with CONFIG_LITTLEFS_OBJ_NAME_LEN=64.
# PlatformIO's mklittlefs defaults to 32, which drops or truncates Next.js chunk names.
if "DataToBin" in env["BUILDERS"]:
    env["BUILDERS"]["DataToBin"].action = env.VerboseAction(
        " ".join(
            [
                '"$MKFSTOOL"',
                "-c",
                "$SOURCES",
                "-s",
                "$FS_SIZE",
                "-p",
                "$FS_PAGE",
                "-b",
                "$FS_BLOCK",
                "-n",
                "64",
                "$TARGET",
            ]
        ),
        "Building FS image from '$SOURCES' directory to $TARGET",
    )
