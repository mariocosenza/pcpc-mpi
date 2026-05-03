#!/bin/bash

echo "Starting rapid server configuration (Intel MPI)..."

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
apt-get install -yq vim htop build-essential openssh-client openssh-server

# Install Intel MPI via google_install_intelmpi
echo "Installing Intel MPI..."
if [ -f "/usr/bin/google_install_intelmpi" ]; then
    /usr/bin/google_install_intelmpi
else
    # Fallback/Alternative: install from official Intel repo
    echo "google_install_intelmpi not found, attempting alternative installation..."
    apt-get install -yq intel-mpi-2021.10.0 || echo "Failed to install intel-mpi package via apt."
fi

# SSH Configuration for the cluster
echo "Configuring SSH keys..."
SSH_DIR="/home/pcpc/.ssh"
mkdir -p "$SSH_DIR"

# Write the common SSH keys provided by Terraform
echo "${id_rsa}" > "$SSH_DIR/id_rsa"
echo "${id_rsa_pub}" > "$SSH_DIR/id_rsa.pub"

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

echo "Verifying installations..."
ALL_OK=true

# Check standard packages
for cmd in vim htop gcc ssh sshd; do
    if command -v $cmd &> /dev/null; then
        echo " - [OK] $cmd is installed."
    else
        echo " - [ERROR] $cmd is NOT installed."
        ALL_OK=false
    fi
done

# Check Intel MPI
if dpkg -l | grep -q intel-mpi || [ -d "/opt/intel/mpi" ] || command -v mpiicc &> /dev/null; then
    echo " - [OK] Intel MPI components found."
else
    echo " - [WARNING] Intel MPI does not seem to be correctly installed or is not in standard paths."
    ALL_OK=false
fi

if [ "$ALL_OK" = true ]; then
    echo "Installation and verification completed successfully!"
else
    echo "Installation completed, but some packages are missing. Please check the logs."
    exit 1
fi