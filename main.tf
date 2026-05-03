provider "google" {
  project = "pcpc-lab" 
  region  = "europe-north2"
}

resource "google_compute_instance" "vm_instances" {
  count        = 4
  name         = "instance-pcpc${count.index + 1}"
  machine_type = "e2-micro" # Nota: molto limitata per compilare Intel MPI
  zone         = "europe-north2-b"

  scheduling {
    preemptible                 = true
    automatic_restart           = false
    provisioning_model          = "SPOT"
    instance_termination_action = "STOP"
  }

  boot_disk {
    initialize_params {
      image = "rocky-linux-cloud/rocky-linux-9" 
      size  = 20 
      type  = "pd-balanced" 
    }
  }

  network_interface {
    network = "default"
    access_config {
      network_tier = "PREMIUM" 
    }
  }

  labels = {
    env = "pcpc-lab-mpi"
  }

  metadata = {
    ssh-keys = "pcpc:${tls_private_key.ssh_key.public_key_openssh}"
  }

  metadata_startup_script = templatefile("${path.module}/install.sh", {
    id_rsa     = tls_private_key.ssh_key.private_key_pem
    id_rsa_pub = tls_private_key.ssh_key.public_key_openssh
  })
}

resource "tls_private_key" "ssh_key" {
  algorithm = "RSA"
  rsa_bits  = 4096
}
