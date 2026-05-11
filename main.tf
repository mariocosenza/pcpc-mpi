provider "google" {
  project = "pcpc-lab"
  region  = "us-west2"
}

resource "google_compute_resource_policy" "mpi_collocation_policy" {
  name   = "mpi-compact-policy"
  region = "us-west2"

  group_placement_policy {
    availability_domain_count = 1
    collocation               = "COLLOCATED"
  }
}

resource "google_compute_instance" "vm_instances" {
  count        = 8
  name         = "instance-pcpc${count.index + 1}"
  machine_type = "c3-standard-8" 
  zone         = "us-west2-a"


  advanced_machine_features {
    threads_per_core = 1
    enable_uefi_networking = true
  }

  scheduling {
    preemptible                 = true
    automatic_restart           = false
    provisioning_model          = "SPOT"
    instance_termination_action = "STOP"
  }

  resource_policies = [google_compute_resource_policy.mpi_collocation_policy.id]

  boot_disk {
    auto_delete = true 
    
    initialize_params {
      image = "rocky-linux-cloud/rocky-linux-10-optimized-gcp-v20260427"
      size  = 40
      type  = "hyperdisk-balanced"

      provisioned_iops       = 3000
      provisioned_throughput = 360
    }
  }

  network_interface {
    network = "default"
    access_config {
      network_tier = "PREMIUM"
    }
  }

  tags = ["mpi-node"]

  metadata = {
    google-logging-enabled    = "false"
    google-monitoring-enabled = "false"
    ssh-keys = "pcpc:${tls_private_key.ssh_key.public_key_openssh}"
  }
  
  metadata_startup_script = templatefile("${path.module}/install.sh", {
    id_rsa     = tls_private_key.ssh_key.private_key_pem
    id_rsa_pub = tls_private_key.ssh_key.public_key_openssh
  })
}

resource "google_compute_firewall" "mpi_allow_internal" {
  name    = "mpi-allow-internal"
  network = "default"

  allow {
    protocol = "tcp"
  }
  allow {
    protocol = "udp"
  }
  allow {
    protocol = "icmp"
  }
  
  source_tags = ["mpi-node"]
  target_tags = ["mpi-node"]
}

resource "google_compute_firewall" "allow_ssh" {
  name    = "allow-ssh-external"
  network = "default"

  allow {
    protocol = "tcp"
    ports    = ["22"]
  }

  source_ranges = ["0.0.0.0/0"]
  target_tags   = ["mpi-node"]
}

resource "tls_private_key" "ssh_key" {
  algorithm = "RSA"
  rsa_bits  = 4096
}