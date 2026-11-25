# Use the latest LTS version of Ubuntu
FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install C/C++ compilers (gcc/g++), make, cmake, gdb (debugger), and git
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    git \
    && rm -rf /var/lib/apt/lists/*


# Set the working directory inside the container
WORKDIR /workspace
