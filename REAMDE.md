Final Project for the SPM Course @ Unipi, a.y. 2023-2024.

# Setup
You need to have `cereal`, `fastflow` and `openmpi-5.0.3` to correctly run the tests.
After download, run `setup.sh` to download them.

# Usage
For Makefile:
- `make all` to compile all sources
- `make all_ff` to compile all Fastflow sources
- `make all_mpi` to compile all MPI sources
- `make <filename>` to compile a given file with g++
- `make mpi_<filename>` or `make mpi file=<filename>` to compile a given file with mpicxx
- `make clean file=<filename>` to remove given file
- `make cleanall` to clean up all files

For tests:
- `ffruns_spmcluster.sh` for running Fastflow on spmcluster
- `ffruns_spmnuma.sh` for running Fastflow on spmnuma
- `mpiruns.sh` for running MPI on spmcluster
