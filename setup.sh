#/bin/bash

# Clone cereal
echo "Downloading cereal ..."
CEREAL_REPO="https://github.com/USCiLab/cereal.git"
git clone "$CEREAL_REPO"

# Clone fastflow repository
echo "Cloning fastflow repository ..."
FASTFLOW_REPO="https://github.com/fastflow/fastflow.git"
git clone "$FASTFLOW_REPO"

# Download openmpi-5.0.3
echo "Downloading OpenMPI 5.0.3 ..."
OPENMPI_URL="https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.3.tar.gz"
wget -O "openmpi-5.0.3.tar.gz" "https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.3.tar.gz"

# Extract openmpi-5.0.3
echo "Extracting OpenMPI 5.0.3 ..."
OPENMPI_TAR="openmpi-5.0.3.tar.gz"
OPENMPI_DIR="openmpi-5.0.3"
mkdir -p "$OPENMPI_DIR"
tar -xzf "$OPENMPI_TAR" -C "$OPENMPI_DIR"

echo "Download, clone and extraction operations completed."
