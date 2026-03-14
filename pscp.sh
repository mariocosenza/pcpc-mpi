#!/bin/bash

# ==========================================
# SELF-INSTALLATION BLOCK
# ==========================================
if [ "$1" == "--install" ]; then
    # Get the absolute path of the directory where this script is located
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
    
    # Check if the directory is already in .bashrc
    if grep -q "$SCRIPT_DIR" ~/.bashrc; then
        echo "The path is already in your ~/.bashrc!"
    else
        # Append the path to .bashrc
        echo -e "\n# pscp script path\nexport PATH=\"\$PATH:$SCRIPT_DIR\"" >> ~/.bashrc
        echo "✅ Added $SCRIPT_DIR to PATH in ~/.bashrc."
        echo "🔄 Please run 'source ~/.bashrc' or restart your terminal to apply changes."
    fi
    exit 0
fi

# ==========================================
# MAIN SCP LOGIC
# ==========================================

# 1. Check arguments (must provide a file and at least one IP address)
if [ "$#" -lt 2 ]; then
    echo "Usage: pscp <file_to_copy> <ip1> [ip2] [ip3] ..."
    echo "Install: ./pscp.sh --install (to add to PATH)"
    echo "Example: pscp ./hello 34.2.52.194 34.2.52.195"
    exit 1
fi

FILE=$1
# The 'shift' command shifts the positional parameters: it removes the first one (the file) 
# so the array "$@" will contain ONLY the remaining IPs.
shift 

# 2. Check if the file actually exists
if [ ! -f "$FILE" ]; then
    echo "Error: File '$FILE' does not exist."
    exit 1
fi

echo "Starting parallel copy of '$FILE'..."

# 3. Loop through all provided IPs
for IP in "$@"; do
    echo "-> Sending to $IP..."
    
    # Copy the file. 
    # -o StrictHostKeyChecking=no prevents prompt blocks if the IP is new.
    # The '&' operator at the end runs the command in the background (parallel execution).
    scp -q -o StrictHostKeyChecking=no "$FILE" "pcpc@$IP:/home/pcpc/" &
done

# 4. Wait for all background processes to finish before exiting the script
wait

echo "Copy completed on all nodes!"