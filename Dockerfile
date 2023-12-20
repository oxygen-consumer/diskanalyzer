# Use an official Fedora runtime as a base image
FROM fedora:latest

# Install necessary build tools and dependencies
RUN dnf install -y \
    make \
    clang

# Set the working directory in the container
WORKDIR /app

# Copy the source code to the container
COPY . /app

# Build the daemon and cli
RUN make all && make install

# Enter the container environment
CMD ["/bin/bash"]