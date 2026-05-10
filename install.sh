#!/bin/bash

if id "pcpc" &>/dev/null; then
    echo "pcpc:root" | chpasswd
else
    useradd -s /bin/bash -d /home/pcpc/ -m -G wheel pcpc
    echo "pcpc:root" | chpasswd
fi

export DEBIAN_FRONTEND=noninteractive
dnf install -y vim htop git gcc gcc-c++ make openssh-clients openssh-server

if [ -f "/usr/bin/google_install_intelmpi" ]; then
    /usr/bin/google_install_intelmpi
fi
dnf install -y intel-oneapi-compiler-dpcpp-cpp-and-cpp-classic intel-oneapi-mpi-devel

SSH_DIR="/home/pcpc/.ssh"
mkdir -p "$SSH_DIR"
echo "${id_rsa}" > "$SSH_DIR/id_rsa"
echo "${id_rsa_pub}" > "$SSH_DIR/id_rsa.pub"
cat "$SSH_DIR/id_rsa.pub" >> "$SSH_DIR/authorized_keys"

cat <<EOF > "$SSH_DIR/config"
Host *
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
    LogLevel ERROR
EOF

sed -i 's/#PubkeyAuthentication yes/PubkeyAuthentication yes/' /etc/ssh/sshd_config
echo "UseDNS no" >> /etc/ssh/sshd_config
systemctl restart sshd

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

cd "$REPO_DIR/lab8"

FLAGS="-O3 -xSAPPHIRERAPIDS -ipo -ffast-math -fiopenmp-simd -qopt-mem-layout-trans=3 -D_CRT_SECURE_NO_WARNINGS"

sudo -u pcpc mpiicx $FLAGS generate_seed.c -o generate_seed
sudo -u pcpc mpiicx $FLAGS lab8vm-file.c -o game_of_life

chown -R pcpc:pcpc "$REPO_DIR"
ldconfig