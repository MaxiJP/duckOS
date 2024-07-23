#!/bin/sh

set -e

# Check for KVM Support
if [ -z "$USE_KVM" ]; then
	USE_KVM="0"
	if [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
	  if ! command -v arch &> /dev/null; then
        USE_KVM="1" # No arch command
    else
      ARCH="$(arch)"
      if [[ "$ARCH" == "x86_64" ]] || [[ "$ARCH" =~ i[3-6]86 ]]; then
        USE_KVM="1"
      fi
    fi
	fi
fi

if [ -z "$DUCKOS_IMAGE" ]; then
	DUCKOS_IMAGE="duckOS.img"
fi

# Determine what acceleration we should use
if [ -z "$DUCKOS_QEMU_ACCEL" ]; then
	if command -v wslpath >/dev/null; then
		DUCKOS_QEMU_ACCEL="-accel whpx,kernel-irqchip=off -accel tcg"
	else
		if [ "$USE_KVM" -ne "0" ]; then
			DUCKOS_QEMU_ACCEL="-enable-kvm"
		else
			DUCKOS_QEMU_ACCEL=""
		fi
	fi
fi

DUCKOS_QEMU_MACHINE=""
case $ARCH in
  i686)
    QEMU_SYSTEM="i386"
    DUCKOS_QEMU_MEM="512M"
    DUCKOS_QEMU_DRIVE="-drive file=$DUCKOS_IMAGE,cache=directsync,format=raw,id=disk,if=ide"
    DUCKOS_QEMU_DEVICES="-device ac97"
    DUCKOS_QEMU_SERIAL="-serial stdio"
    ;;
  aarch64)
    QEMU_SYSTEM="aarch64"
    DUCKOS_QEMU_MACHINE="-machine raspi3b"
    DUCKOS_QEMU_MEM="1G"
    DUCKOS_QEMU_DRIVE=""
    DUCKOS_QEMU_DEVICES=""
    DUCKOS_QEMU_SERIAL="-serial null -serial stdio" # UART1 is mini UART
    ;;
  *)
    fail "Unsupported architecture $ARCH."
    ;;
esac

# Find which qemu binary we should use
if command -v wslpath >/dev/null; then
	# We're on windows, use windows QEMU
	if [ -z "$DUCKOS_WIN_QEMU_INSTALL_DIR" ]; then
		DUCKOS_WIN_QEMU_INSTALL_DIR="C:\\Program Files\\qemu"
	fi
	USE_KVM="0"
	DUCKOS_QEMU="$(wslpath "${DUCKOS_WIN_QEMU_INSTALL_DIR}")/qemu-system-$QEMU_SYSTEM.exe"
	DUCKOS_IMAGE="$(wslpath -w "$DUCKOS_IMAGE")"
else
	DUCKOS_QEMU="qemu-system-$QEMU_SYSTEM"
fi

DUCKOS_QEMU_DISPLAY=""

if "$DUCKOS_QEMU" --display help | grep -iq sdl; then
	DUCKOS_QEMU_DISPLAY="--display sdl"
elif "$DUCKOS_QEMU" --display help | grep -iq cocoa; then
	DUCKOS_QEMU_DISPLAY="--display cocoa"
fi

if [ -z "$DUCKOS_KERNEL_ARGS" ]; then
  DUCKOS_KERNEL_ARGS="$@"
fi

# Run!
DUCKOS_QEMU_ARGS="
	-s
	-kernel kernel/${KERNEL_NAME}
	-m $DUCKOS_QEMU_MEM
	$DUCKOS_QEMU_SERIAL
	$DUCKOS_QEMU_DEVICES
	$DUCKOS_QEMU_DRIVE
	$DUCKOS_QEMU_MACHINE
	$DUCKOS_QEMU_DISPLAY
	$DUCKOS_QEMU_ACCEL"

"$DUCKOS_QEMU" $DUCKOS_QEMU_ARGS -append "$DUCKOS_KERNEL_ARGS"
