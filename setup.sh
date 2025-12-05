#!/bin/bash

# This script interactively configures the config.ini file for the NTRIP client.

echo "Welcome to the NTRIP client configurator."
echo "Please provide the following values. Press Enter to accept the default value shown in brackets."
echo

# A simple function to prompt the user for a value
ask_value() {
    local prompt_message=$1
    local default_value=$2
    local user_input
    
    # Prompt the user
    read -p "$prompt_message [$default_value]: " user_input
    
    # If the user just pressed Enter, use the default. Otherwise, use their input.
    if [ -z "$user_input" ]; then
        echo "$default_value"
    else
        echo "$user_input"
    fi
}

# --- NTRIP Section ---
echo "--- NTRIP Caster Settings ---"
NTRIP_HOST=$(ask_value "Enter NTRIP caster host" "crtk.net")
NTRIP_PORT=$(ask_value "Enter NTRIP caster port" "2101")
MOUNTPOINT=$(ask_value "Enter NTRIP mountpoint ('auto' for closest)" "auto")
NTRIP_USERNAME=$(ask_value "Enter NTRIP username" "centipede")
NTRIP_PASSWORD=$(ask_value "Enter NTRIP password" "centipede")
echo

# --- Serial Port Section ---
echo "--- Serial Port Settings ---"
SERIAL_PORT=$(ask_value "Enter serial port device" "/dev/ttyACM0")
SERIAL_BAUD=$(ask_value "Enter serial port baud rate" "115200")
echo

# --- Output Section ---
echo "--- Output Settings ---"
READ_FROM_SERIAL=$(ask_value "Display GPS data from serial port? (true/false)" "true")
echo

# --- File Generation ---
CONFIG_FILE="config.ini"
echo "Writing configuration to $CONFIG_FILE..."

# Use a "here document" to write the configuration file
cat > "$CONFIG_FILE" << EOL
[ntrip]
host = ${NTRIP_HOST}
port = ${NTRIP_PORT}
mountpoint = ${MOUNTPOINT}
username = ${NTRIP_USERNAME}
password = ${NTRIP_PASSWORD}

[serial]
port = ${SERIAL_PORT}
baud = ${SERIAL_BAUD}

[output]
read_from_serial = ${READ_FROM_SERIAL}
EOL

echo "Configuration file '$CONFIG_FILE' created successfully."
echo "You can now run the main application."
