# pcpc-mpi

Fast, repeatable **OpenMPI cluster setup on Ubuntu** (tested with Ubuntu 24.04 LTS images) with helper scripts for **parallel file distribution** and an example **Terraform** config to spin up a small GCE cluster.

This repo is intentionally lightweight and script-first:
- `install.sh` bootstraps a node for MPI work (packages + SSH setup).
- `pscp.sh` provides a simple “parallel scp” helper to copy files to many nodes at once.
- `main.tf` is an example Terraform configuration for provisioning multiple Google Compute Engine VMs.

## What it does

### `install.sh` (node bootstrap)
On a fresh Ubuntu VM, the script:
- Creates a `pcpc` user (if missing) and adds it to `sudo`
- Sets the `pcpc` password to `root` (change this for real usage)
- Installs common tools + OpenMPI:
  - `openmpi-bin`, `libopenmpi-dev`, `openssh-server/client`, `build-essential`, etc.
- Configures passwordless-ish SSH convenience for cluster use by:
  - generating an SSH keypair (only if not present)
  - appending the public key to `authorized_keys`
  - writing an SSH config that disables strict host checking within the cluster

> Security note: Disabling strict host key checking and setting a simple password is convenient for labs, but not recommended for production or exposed hosts.

### `pscp.sh` (parallel copy helper)
Copies a local file to `/home/pcpc/` on multiple nodes concurrently using `scp` in the background.

Also supports `--install` to add the script directory to your `PATH` via `~/.bashrc`.

### `main.tf` (Terraform example)
Example Terraform configuration to create **4 preemptible/spot** `e2-micro` instances in GCE (region `europe-north2`, zone `europe-north2-b`) using an Ubuntu 24.04 LTS image.

> Note: the startup-script line is currently commented out. See below for enabling automated bootstrapping.

---

## Quick start

### 1) Bootstrap a node (manually)
On each VM (or at least your first node while testing):

```bash
chmod +x install.sh
sudo ./install.sh
```

### 2) Copy files to all nodes with `pscp`
Install to PATH (optional):

```bash
chmod +x pscp.sh
./pscp.sh --install
source ~/.bashrc
```

Copy a file (example copies `hello` to two nodes):

```bash
pscp ./hello 34.2.52.194 34.2.52.195
```

The file will be uploaded to:

```text
pcpc@<ip>:/home/pcpc/
```

---

## Terraform (GCE) usage

### Prerequisites
- Terraform installed
- Google Cloud project + credentials configured (e.g., `gcloud auth application-default login`)
- Compute Engine API enabled
- Permissions to create instances

### Apply
From the repo root:

```bash
terraform init
terraform apply
```

### (Optional) enable automatic OpenMPI install via startup script
In `main.tf`, uncomment the line:

```hcl
#metadata_startup_script = file("${path.module}/install.sh")
```

so that it becomes:

```hcl
metadata_startup_script = file("${path.module}/install.sh")
```

Then `terraform apply` again.

---

## Typical MPI workflow (sketch)

1. Provision nodes (Terraform or your preferred method)
2. Ensure all nodes have OpenMPI + SSH configured (via `install.sh`)
3. Build your MPI program on one node (or locally)
4. Distribute the binary to all nodes with `pscp`
5. Run `mpirun` from the head node, providing a hostfile (you supply)

(Hostfile creation and `mpirun` commands are intentionally not hardcoded here since they vary by environment/networking.)

---

## Repository layout

- `install.sh` — OpenMPI + SSH bootstrap for Ubuntu nodes
- `pscp.sh` — parallel `scp` helper for copying files to many nodes
- `main.tf` — example Terraform config for a small GCE cluster
- `.vscode/` — editor settings (optional)
- `mpi/` — (reserved) MPI-related code/examples (currently empty or not populated)

---

## Notes / caveats

- The default `pcpc:root` password is for convenience; change/remove it if needed.
- SSH host checking is disabled for convenience inside the cluster.
- `main.tf` currently hardcodes:
  - `project = "pcpc-lab"`
  - `region = "europe-north2"`
  Adjust these to match your GCP setup.

