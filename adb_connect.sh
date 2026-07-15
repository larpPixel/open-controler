#!/bin/bash

# --- Color Definitions ---
GREEN='\033[0;32m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${PURPLE}=== QR Controller ADB Tunnel Setup ===${NC}"

# Check for adb
if ! command -v adb &> /dev/null; then
    echo -e "${RED}Error: adb command not found. Please install Android SDK platform tools.${NC}"
    exit 1
fi

# Check for connected devices
DEVICE_COUNT=$(adb devices | grep -v "List" | grep -v "^$" | wc -l)
if [ "$DEVICE_COUNT" -eq 0 ]; then
    echo -e "${RED}Error: No Android device connected via USB. Run 'adb devices' to check.${NC}"
    exit 1
fi

echo -e "${CYAN}Setting up ADB reverse port forwarding (TCP 7654)...${NC}"
adb reverse tcp:7654 tcp:7654

# Kill any existing nc listeners on PC port 7654
echo -e "${CYAN}Cleaning up any old tunnel processes on PC...${NC}"
fuser -k -n tcp 7654 &>/dev/null || true
rm -f /tmp/pc_forward_fifo

# Start PC-side TCP-to-UDP forwarder (runs in background)
echo -e "${CYAN}Starting PC-side TCP-to-UDP forwarder...${NC}"
mkfifo /tmp/pc_forward_fifo
nc -l -p 7654 < /tmp/pc_forward_fifo | nc -u -w 1 127.0.0.1 7654 > /tmp/pc_forward_fifo &
PC_NC_PID=$!

# Start phone-side UDP-to-TCP forwarder (runs in background)
echo -e "${CYAN}Starting phone-side UDP-to-TCP forwarder...${NC}"
adb shell "mkdir -p /data/local/tmp/forward && rm -f /data/local/tmp/forward/fifo && mkfifo /data/local/tmp/forward/fifo"
adb shell "nc -u -l -p 7654 < /data/local/tmp/forward/fifo | nc 127.0.0.1 7654 > /data/local/tmp/forward/fifo" &
PHONE_NC_PID=$!

echo -e "${GREEN}ADB Tunnel is now active!${NC}"
echo -e "1. Open the app on your phone."
echo -e "2. Tap the ${PURPLE}Connect (ADB / USB)${NC} button."
echo -e "3. Play!"
echo -e ""
echo -e "Press [ENTER] or Ctrl+C to stop the tunnel and clean up."
read -r

# Clean up
echo -e "${CYAN}Stopping tunnel processes...${NC}"
kill $PC_NC_PID &>/dev/null || true
kill $PHONE_NC_PID &>/dev/null || true
adb shell "killall nc" &>/dev/null || true
rm -f /tmp/pc_forward_fifo
adb reverse --remove tcp:7654 &>/dev/null || true
echo -e "${GREEN}Cleaned up successfully.${NC}"
