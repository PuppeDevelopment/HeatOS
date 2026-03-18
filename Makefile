ARCH ?= x86
POWERSHELL ?= powershell

.PHONY: help build build-x64 run run-x64 run-skip clean

help:
	@echo "HeatOS build wrapper"
	@echo "  make build        (ARCH=x86 by default)"
	@echo "  make build ARCH=x64"
	@echo "  make run          (build + run)"
	@echo "  make run ARCH=x64"
	@echo "  make run-skip     (run without rebuilding)"
	@echo "  make clean"

build:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Arch $(ARCH)

build-x64:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Arch x64

run:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File scripts/run.ps1 -Arch $(ARCH)

run-x64:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File scripts/run.ps1 -Arch x64

run-skip:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File scripts/run.ps1 -Arch $(ARCH) -SkipBuild

clean:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path build) { Remove-Item -Recurse -Force build }"
