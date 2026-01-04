# Related Projects

This repo is part of a small ecosystem of VM/emulation + tooling projects that share ideas, conventions, and (eventually) reusable components.

## Core emulation

- **x64-vm** (this repo)  
  x86 real-mode emulator aimed at PC-style boot bring-up (floppy/BIOS-ish path), with a test-first instruction runway.

- **libvm / arm-vm**  
  ARM-focused emulator and test harness.  
  Relationship: x64-vm borrows the test layout philosophy and incremental bring-up style from this project.

## VM tooling + orchestration

- **guppy**  
  CLI/tooling ecosystem for working with virtual disks, images, and VM-style workflows.  
  Relationship: likely home for higher-level workflows that feed images/config into x64-vm/libvm.

- **vim-cmd**  
  A CLI inspired by VMware’s `vim-cmd` for interacting with a management daemon.  
  Relationship: orchestration/management UX patterns that may be reused across projects.

- **hostd**  
  A management daemon inspired by VMware’s `hostd`.  
  Relationship: backend service layer that `vim-cmd` talks to; potential long-term control plane for VM instances.

## Display + remote UI

- **libspice**  
  A minimal SPICE-like library/endpoint work used to explore remote framebuffer and input plumbing.  
  Relationship: possible future display backend for VM video output (framebuffer → remote viewer).

## Test harnesses / external validation

- **qemu-smoketests**  
  A lightweight QEMU-based smoke test suite used to validate binaries and “known-good” behavior.  
  Relationship: a reference oracle to compare emulator behavior, boot sequences, and device assumptions.

---

## Notes / Conventions

- Many repos share “test-first” bring-up and numbered test directories (e.g., `00-smoke`, `01-core-iset`).
- The long-term direction is reusable building blocks: device interfaces, image tooling, CLI patter