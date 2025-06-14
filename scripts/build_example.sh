#!/bin/bash

# Ensure NASM is installed
if ! command -v nasm &> /dev/null; then
    echo "ERROR: NASM assembler is required but not installed"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p ../build

# Assemble the program
nasm -f bin ../rootfs/example.asm -o ../build/example.bin

# Check if assembly was successful
if [ $? -eq 0 ]; then
    echo "Successfully assembled example.bin"
    
    # Copy to rootfs
    cp ../build/example.bin ../rootfs/
    echo "Copied example.bin to rootfs/"
else
    echo "Failed to assemble example.asm"
    exit 1
fi
