Import("env")

# Get the board configuration
board_config = env.BoardConfig()

# Get flash size in bytes. Default to 0 if not specified.
flash_size = board_config.get("upload.maximum_size", 0)

# Define megabytes for easier comparison
MB = 1024 * 1024

# Get the current build flags
build_flags = env.get("BUILD_FLAGS", [])

# Add HAS_GAMES for boards with more than 4MB flash
if flash_size > 4 * MB:
    build_flags.append("-DHAS_GAMES")
    print(f"-> Board has > 4MB flash. Enabling games (HAS_GAMES).")

# Add THENG_GAME for boards with 8MB or more flash
if flash_size >= 8 * MB:
    build_flags.append("-DTHENG_GAME")
    print(f"-> Board has >= 8MB flash. Enabling extended games (THENG_GAME).")

# Update the environment's build flags
env.Replace(BUILD_FLAGS=build_flags)
