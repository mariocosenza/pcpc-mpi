#!/bin/bash

echo "Starting rapid server configuration (OpenMPI)..."

# Check if user 'pcpc' exists
if id "pcpc" &>/dev/null; then
    echo "User pcpc exists. Changing password to 'root'..."
    echo "pcpc:root" | chpasswd
else
    echo "Creating user pcpc..."
    useradd -s /bin/bash -d /home/pcpc/ -m -G sudo pcpc
    # Set the password to "root" for user pcpc
    echo "pcpc:root" | chpasswd
    echo "User pcpc created successfully."
fi

# Update and install packages quickly without interactive prompts
echo "Updating APT and installing packages..."
export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -yq vim htop build-essential openssh-client openssh-server openmpi-bin libopenmpi-dev

# SSH Configuration for the cluster
echo "Configuring SSH keys..."
SSH_DIR="/home/pcpc/.ssh"
mkdir -p "$SSH_DIR"

# Generate keys ONLY if they don't already exist (prevents overwriting on workers)
if [ ! -f "$SSH_DIR/id_rsa" ]; then
    ssh-keygen -t rsa -b 4096 -f "$SSH_DIR/id_rsa" -q -N ""
fi

# Add the public key to authorized_keys
cat "$SSH_DIR/id_rsa.pub" >> "$SSH_DIR/authorized_keys"

# Create the SSH config file to disable strict host checking within the cluster
cat <<EOF > "$SSH_DIR/config"
Host *
   StrictHostKeyChecking no
   UserKnownHostsFile=/dev/null
EOF

# Set the strict permissions required by SSH
chmod 700 "$SSH_DIR"
chmod 600 "$SSH_DIR/id_rsa" "$SSH_DIR/authorized_keys" "$SSH_DIR/config"
chmod 644 "$SSH_DIR/id_rsa.pub"
chown -R pcpc:pcpc "$SSH_DIR"

# Update shared library cache
echo "Running ldconfig..."
ldconfig

echo "Installation completed successfully!"