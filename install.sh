#!/bin/bash

if id "pcpc" &>/dev/null; then
    echo "pcpc:root" | chpasswd
else
    useradd -s /bin/bash -d /home/pcpc/ -m -G wheel pcpc
    echo "pcpc:root" | chpasswd
fi

dnf install -y vim git gcc gcc-c++ make openssh-clients openssh-server

cat <<EOF > /etc/yum.repos.d/oneAPI.repo
[oneAPI]
name=Intel oneAPI repository
baseurl=https://yum.repos.intel.com/oneapi
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
EOF

if [ -f "/usr/bin/google_install_intelmpi" ]; then
    /usr/bin/google_install_intelmpi
fi

dnf install -y intel-oneapi-compiler-dpcpp-cpp intel-oneapi-mpi-devel

SSH_DIR="/home/pcpc/.ssh"
mkdir -p "$SSH_DIR"

cat << 'EOF_RSA' > "$SSH_DIR/id_rsa"
${id_rsa}
EOF_RSA

cat << 'EOF_PUB' > "$SSH_DIR/id_rsa.pub"
${id_rsa_pub}
EOF_PUB

cat "$SSH_DIR/id_rsa.pub" >> "$SSH_DIR/authorized_keys"

cat <<EOF > "$SSH_DIR/config"
Host *
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
    LogLevel ERROR
EOF

sed -i 's/#PubkeyAuthentication yes/PubkeyAuthentication yes/' /etc/ssh/sshd_config
echo "UseDNS no" >> /etc/ssh/sshd_config
systemctl enable --now sshd

chown -R pcpc:pcpc "$SSH_DIR"
chmod 700 "$SSH_DIR"
chmod 600 "$SSH_DIR/id_rsa" "$SSH_DIR/authorized_keys"

REPO_DIR="/home/pcpc/pcpc-mpi"
if [ ! -d "$REPO_DIR" ]; then
    sudo -u pcpc git clone https://github.com/mariocosenza/pcpc-mpi.git "$REPO_DIR"
fi

cat <<EOF >> /home/pcpc/.bashrc
if [ -f "/opt/intel/oneapi/setvars.sh" ]; then
    source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
fi
export I_MPI_PIN_DOMAIN=core
export I_MPI_PIN_ORDER=compact
EOF

if [ -f "/opt/intel/oneapi/setvars.sh" ]; then
    source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
fi

cd "$REPO_DIR/mpi/lab8" || exit 1

FLAGS="-O3 -xSAPPHIRERAPIDS -ipo -ffast-math -fiopenmp-simd -qopt-mem-layout-trans=3"

mpiicx $FLAGS generate_seed.c -o generate_seed
mpiicx $FLAGS lab8vm-file.c -o game_of_life

chown -R pcpc:pcpc "$REPO_DIR"
ldconfig