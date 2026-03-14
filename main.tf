provider "google" {
  project = "pcpc-lab" 
  region  = "europe-north2"
}

resource "google_compute_instance" "vm_instances" {
  count        = 4
  name         = "instance-pcpc${count.index + 1}"
  machine_type = "e2-micro"
  zone         = "europe-north2-b"

  scheduling {
    preemptible                 = true
    automatic_restart           = false
    provisioning_model          = "SPOT"
    instance_termination_action = "STOP"
  }

  boot_disk {
    initialize_params {
      image = "ubuntu-os-cloud/ubuntu-2404-lts-amd64"
      size  = 10
      type  = "pd-standard"
    }
  }
  network_interface {
    network = "default"
    access_config {
      network_tier = "STANDARD"
    }
  }

  labels = {
    env = "pcpc-lab-mpi"
  }

  #Installing OpenMPI and other necessary packages using a startup script
  #metadata_startup_script = file("${path.module}/install.sh")
}