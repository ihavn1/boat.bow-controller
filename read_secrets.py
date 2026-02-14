"""
PlatformIO script to read OTA password from secrets.h and inject into espota uploads.
This keeps passwords out of platformio.ini (which may be committed to git).
"""
Import("env")
import re


def extract_ota_password():
    """Read OTA_PASSWORD from src/secrets.h."""
    try:
        with open("src/secrets.h", "r") as handle:
            content = handle.read()
            match = re.search(r'#define\s+OTA_PASSWORD\s+"([^"]+)"', content)
            if match:
                password = match.group(1)
                print("OTA password loaded from src/secrets.h")
                return password
    except Exception as exc:
        print(f"Warning: Could not read OTA_PASSWORD from src/secrets.h: {exc}")
    return None


def on_upload(source, target, env):
    """Called before upload to inject OTA password on network uploads."""
    upload_port = env.get("UPLOAD_PORT", "")

    # Check if it's a network upload (not a COM port or /dev/* path).
    if upload_port and not upload_port.startswith("COM") and not upload_port.startswith("/dev/"):
        password = extract_ota_password()
        if password:
            flags = env.get("UPLOADERFLAGS", [])
            flags = [flag for flag in flags if not flag.startswith("--auth=")]
            flags.append(f"--auth={password}")
            env.Replace(UPLOADERFLAGS=flags)
            print(f"OTA authentication configured for {upload_port}")
        else:
            print("Warning: Network upload without OTA_PASSWORD; authentication will fail")


# Register the callback for both firmware and filesystem uploads.
env.AddPreAction("upload", on_upload)
env.AddPreAction("uploadfs", on_upload)
