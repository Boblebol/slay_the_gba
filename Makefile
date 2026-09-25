#---------------------------------------------------------------------------------------------------------------------
# Slay The GBA Makefile
# Supports both native devkitARM and automatic Docker fallback!
#---------------------------------------------------------------------------------------------------------------------
TARGET          := slay_the_gba
BUILD           := build
LIBBUTANO       := butano/butano
PYTHON          := python3
SOURCES         := src src/engine butano/common/src
INCLUDES        := include butano/common/include
DATA            := 
GRAPHICS        := graphics butano/common/graphics
AUDIO           := audio butano/common/audio
AUDIOBACKEND    := maxmod
AUDIOTOOL       := 
DMGAUDIO        := dmg_audio butano/common/dmg_audio
DMGAUDIOBACKEND := default
ROMTITLE        := SLAYTHEGBA
ROMCODE         := SGBA
USERFLAGS       := 
USERCXXFLAGS    := 
USERASFLAGS     := 
USERLDFLAGS     := 
USERLIBDIRS     := 
USERLIBS        := 
DEFAULTLIBS     := 
STACKTRACE      := 
USERBUILD       := 
EXTTOOL         := 

# Check if we are inside devkitpro environment
ifeq ($(strip $(DEVKITARM)),)
    ARM_GCC := $(shell which arm-none-eabi-gcc 2>/dev/null)
    ifeq ($(ARM_GCC),)
        USE_DOCKER := 1
    endif
endif

ifeq ($(USE_DOCKER),1)

all:
	@echo "==> devkitARM not found in host PATH. Building ROM via Docker (devkitpro/devkitarm)..."
	docker run --rm -v "$$(pwd):/work" -w /work devkitpro/devkitarm bash -c 'source /opt/devkitpro/devkitarm.sh && make -j$$(nproc)'

clean:
	@echo "==> Cleaning via Docker..."
	docker run --rm -v "$$(pwd):/work" -w /work devkitpro/devkitarm bash -c 'source /opt/devkitpro/devkitarm.sh && make clean'

test:
	@mkdir -p bin
	clang++ -std=c++20 -Iinclude src/engine/card.cpp src/engine/entity.cpp src/engine/combat.cpp tests/cli_test.cpp -o bin/cli_test
	./bin/cli_test

play:
	@mkdir -p bin
	clang++ -std=c++20 -Iinclude src/engine/card.cpp src/engine/entity.cpp src/engine/combat.cpp tests/interactive_cli.cpp -o bin/slay_cli
	./bin/slay_cli

else

ifndef LIBBUTANOABS
	export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak

test:
	@mkdir -p bin
	clang++ -std=c++20 -Iinclude src/engine/card.cpp src/engine/entity.cpp src/engine/combat.cpp tests/cli_test.cpp -o bin/cli_test
	./bin/cli_test

play:
	@mkdir -p bin
	clang++ -std=c++20 -Iinclude src/engine/card.cpp src/engine/entity.cpp src/engine/combat.cpp tests/interactive_cli.cpp -o bin/slay_cli
	./bin/slay_cli

endif
