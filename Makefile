#---------------------------------------------------------------------------------------------------------------------
# SLAY THE GBA — MAKEFILE UNIFIÉ & HAUTE FIDÉLITÉ
# Support natif devkitARM & conteneur Docker devkitpro/devkitarm
#---------------------------------------------------------------------------------------------------------------------

HERO ?= IRONCLAD

# Couleurs ANSI pour affichage terminal
CYAN    := \033[1;36m
GREEN   := \033[1;32m
YELLOW  := \033[1;33m
RED     := \033[1;31m
MAGENTA := \033[1;35m
BLUE    := \033[1;34m
BOLD    := \033[1m
RESET   := \033[0m

# Configuration par Cartouche / Héros (1 ROM = 1 Héros)
ifeq ($(HERO),SILENT)
    TARGET := slay_silent
    ROMTITLE := SLAYSILENT
    USERCXXFLAGS += -DHERO_SILENT
else ifeq ($(HERO),DEFECT)
    TARGET := slay_defect
    ROMTITLE := SLAYDEFECT
    USERCXXFLAGS += -DHERO_DEFECT
else ifeq ($(HERO),REGENT)
    TARGET := slay_regent
    ROMTITLE := SLAYREGENT
    USERCXXFLAGS += -DHERO_REGENT
else ifeq ($(HERO),NECROBINDER)
    TARGET := slay_necrobinder
    ROMTITLE := SLAYNECRO
    USERCXXFLAGS += -DHERO_NECROBINDER
else
    TARGET := slay_ironclad
    ROMTITLE := SLAYIRON
    USERCXXFLAGS += -DHERO_IRONCLAD
endif

BUILD           := build_$(TARGET)
LIBBUTANO       := butano/butano
PYTHON          := python3
SOURCES         := src src/engine src/gba butano/common/src
INCLUDES        := include butano/common/include
DATA            := 
GRAPHICS        := graphics butano/common/graphics
AUDIO           := audio butano/common/audio
AUDIOBACKEND    := maxmod
AUDIOTOOL       := 
DMGAUDIO        := dmg_audio butano/common/dmg_audio
DMGAUDIOBACKEND := default
ROMCODE         := SGBA
USERFLAGS       := 
USERASFLAGS     := 
USERLDFLAGS     := 
USERLIBDIRS     := 
USERLIBS        := 
DEFAULTLIBS     := 
STACKTRACE      := 
USERBUILD       := 
EXTTOOL         := 

ENGINE_SOURCES  := src/engine/card.cpp \
                   src/engine/entity.cpp \
                   src/engine/combat.cpp \
                   src/engine/ancient.cpp \
                   src/engine/relic.cpp \
                   src/engine/monster.cpp \
                   src/engine/potion.cpp \
                   src/engine/event.cpp \
                   src/engine/card_db.cpp \
                   src/engine/character.cpp \
                   src/engine/map.cpp \
                   src/engine/run.cpp \
                   src/engine/shop.cpp \
                   src/engine/save_system.cpp \
                   src/engine/title_menu.cpp \
                   src/engine/navigation.cpp \
                   src/engine/event_manager.cpp \
                   src/engine/pokemon_battle_ui.cpp

# Détection de l'environnement devkitARM / Docker
ifeq ($(strip $(DEVKITARM)),)
    ARM_GCC := $(shell which arm-none-eabi-gcc 2>/dev/null)
    ifeq ($(ARM_GCC),)
        USE_DOCKER := 1
    endif
endif

.PHONY: help default all all-roms ironclad silent defect regent necrobinder clean clean-all clean-obsolete test test-e2e test-all play size info run run-ironclad run-silent run-defect run-regent run-necrobinder

ifeq ($(USE_DOCKER),1)

# Cible par défaut : Menu d'aide interactif
default: help

all:
	@printf "$(BLUE)==> $(BOLD)Compilation de $(TARGET).gba via Docker (devkitpro/devkitarm)...$(RESET)\n"
	@docker run --rm -v "$$(pwd):/work" -w /work devkitpro/devkitarm bash -c 'source /opt/devkitpro/devkitarm.sh && make HERO=$(HERO) -j$$(nproc)'
	@printf "$(GREEN)✓ Cartouche $(TARGET).gba prête !$(RESET)\n"

# -------------------------------------------------------------------------------------------------
# 🎮 COMPILATION DES 5 CARTOUCHES INDIVIDUELLES
# -------------------------------------------------------------------------------------------------
ironclad:
	@$(MAKE) HERO=IRONCLAD all

silent:
	@$(MAKE) HERO=SILENT all

defect:
	@$(MAKE) HERO=DEFECT all

regent:
	@$(MAKE) HERO=REGENT all

necrobinder:
	@$(MAKE) HERO=NECROBINDER all

all-roms:
	@printf "\n$(MAGENTA)========================================================================$(RESET)\n"
	@printf "$(MAGENTA)  🗡️  SLAY THE GBA — COMPILATION COMPLÈTE DES 5 CARTOUCHES HÉROS       $(RESET)\n"
	@printf "$(MAGENTA)========================================================================$(RESET)\n\n"
	@$(MAKE) HERO=IRONCLAD all
	@$(MAKE) HERO=SILENT all
	@$(MAKE) HERO=DEFECT all
	@$(MAKE) HERO=REGENT all
	@$(MAKE) HERO=NECROBINDER all
	@printf "\n$(GREEN)$(BOLD)✓ Les 5 Cartouches GBA officielles sont prêtes à jouer :$(RESET)\n"
	@ls -lh slay_*.gba

roms: all-roms

# -------------------------------------------------------------------------------------------------
# 🧹 CIBLES DE NETTOYAGE
# -------------------------------------------------------------------------------------------------
clean-obsolete:
	@printf "$(YELLOW)==> Suppression des anciennes versions obsolètes du jeu...$(RESET)\n"
	@rm -f slay_the_gba.gba slay_the_gba.elf slay_the_gba.sav slay_the_gba.map
	@rm -f *.elf *.map *.sav
	@printf "$(GREEN)✓ Fichiers et builds obsolètes supprimés.$(RESET)\n"

clean: clean-obsolete
	@printf "$(YELLOW)==> Nettoyage des répertoires intermédiaires de compilation...$(RESET)\n"
	@rm -rf bin
	@rm -rf build_* build
	@docker run --rm -v "$$(pwd):/work" -w /work devkitpro/devkitarm bash -c 'source /opt/devkitpro/devkitarm.sh && make clean' 2>/dev/null || true
	@printf "$(GREEN)✓ Nettoyage terminé. Les 5 cartouches officielles restent intactes.$(RESET)\n"

clean-all: clean
	@printf "$(RED)==> Suppression de TOUTES les ROMs GBA (slay_*.gba)...$(RESET)\n"
	@rm -f slay_*.gba
	@printf "$(GREEN)✓ Répertoire totalement réinitialisé à zéro (propre).$(RESET)\n"

# -------------------------------------------------------------------------------------------------
# 🧪 TESTS & AUTOMATISATION DU MOTEUR C++20
# -------------------------------------------------------------------------------------------------
test:
	@mkdir -p bin
	@printf "$(CYAN)==> Compilation et exécution des tests unitaires déterministes...$(RESET)\n"
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/cli_test.cpp -o bin/cli_test
	@./bin/cli_test
	@printf "$(GREEN)✓ Tous les tests unitaires ont réussi !$(RESET)\n"

test-e2e:
	@mkdir -p bin
	@printf "$(CYAN)==> Compilation et exécution du test de simulation bout-en-bout (E2E)...$(RESET)\n"
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/e2e_test.cpp -o bin/e2e_test
	@./bin/e2e_test
	@printf "$(GREEN)✓ Test E2E validé à 100%% !$(RESET)\n"

test-all: test test-e2e
	@printf "\n$(GREEN)$(BOLD)========================================================================$(RESET)\n"
	@printf "$(GREEN)$(BOLD)  🎉 VALIDATION COMPLÈTE DU MOTEUR : 100%% DE SUCCÈS                    $(RESET)\n"
	@printf "$(GREEN)$(BOLD)========================================================================$(RESET)\n"

# -------------------------------------------------------------------------------------------------
# 🕹️ SIMULATEUR TERMINAL INTERACTIF
# -------------------------------------------------------------------------------------------------
play:
	@mkdir -p bin
	@printf "$(MAGENTA)==> Compilation du Simulateur Terminal Haute Fidélité...$(RESET)\n"
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/interactive_cli.cpp -o bin/slay_cli
	@./bin/slay_cli

cli: play

# -------------------------------------------------------------------------------------------------
# 📊 INFORMATIONS & TAILLE DES CARTOUCHES
# -------------------------------------------------------------------------------------------------
size: info

info:
	@printf "\n$(BOLD)📊 ÉTAT DES CARTOUCHES SLAY THE GBA (Limite Cartouche GBA : 32 Mo) :$(RESET)\n"
	@printf "%s\n" "------------------------------------------------------------------------"
	@if ls slay_*.gba 1> /dev/null 2>&1; then \
		ls -lh slay_*.gba | awk '{printf "  📦 %-24s  Taille: %6s  (Date: %s %s)\n", $$9, $$5, $$7, $$8}'; \
	else \
		printf "  $(YELLOW)Aucune ROM compilée pour l'instant. Tapez 'make all-roms'.$(RESET)\n"; \
	fi
	@printf "%s\n" "------------------------------------------------------------------------"

# -------------------------------------------------------------------------------------------------
# 🚀 LANCEMENT DANS UN ÉMULATEUR
# -------------------------------------------------------------------------------------------------
run-ironclad: ironclad
	@open slay_ironclad.gba 2>/dev/null || xdg-open slay_ironclad.gba 2>/dev/null || mgba-qt slay_ironclad.gba 2>/dev/null || true

run-silent: silent
	@open slay_silent.gba 2>/dev/null || xdg-open slay_silent.gba 2>/dev/null || mgba-qt slay_silent.gba 2>/dev/null || true

run-defect: defect
	@open slay_defect.gba 2>/dev/null || xdg-open slay_defect.gba 2>/dev/null || mgba-qt slay_defect.gba 2>/dev/null || true

run-regent: regent
	@open slay_regent.gba 2>/dev/null || xdg-open slay_regent.gba 2>/dev/null || mgba-qt slay_regent.gba 2>/dev/null || true

run-necrobinder: necrobinder
	@open slay_necrobinder.gba 2>/dev/null || xdg-open slay_necrobinder.gba 2>/dev/null || mgba-qt slay_necrobinder.gba 2>/dev/null || true

run: run-ironclad

else

# Configuration Butano natif (lorsque devkitARM est installé localement)
ifeq ($(strip $(LIBBUTANOABS)),)
    export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif
ifeq ($(strip $(LIBBUTANOABS)),)
    export LIBBUTANOABS := $(CURDIR)/$(LIBBUTANO)
endif

include $(LIBBUTANOABS)/butano.mak

clean-obsolete:
	@rm -f slay_the_gba.gba slay_the_gba.elf slay_the_gba.sav slay_the_gba.map
	@rm -f *.elf *.map *.sav

clean-all: clean clean-obsolete
	@rm -f slay_*.gba

test:
	@mkdir -p bin
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/cli_test.cpp -o bin/cli_test
	@./bin/cli_test

test-e2e:
	@mkdir -p bin
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/e2e_test.cpp -o bin/e2e_test
	@./bin/e2e_test

test-all: test test-e2e

play:
	@mkdir -p bin
	@clang++ -std=c++20 -Iinclude $(ENGINE_SOURCES) tests/interactive_cli.cpp -o bin/slay_cli
	@./bin/slay_cli

endif

# -------------------------------------------------------------------------------------------------
# 📖 MENU D'AIDE PRINCIPAL
# -------------------------------------------------------------------------------------------------
help:
	@printf "\n"
	@printf "$(CYAN)$(BOLD)  ███████╗██╗      █████╗ ██╗   ██╗    ████████╗██╗  ██╗███████╗    ██████╗ ██████╗  █████╗ $(RESET)\n"
	@printf "$(CYAN)$(BOLD)  ██╔════╝██║     ██╔══██╗╚██╗ ██╔╝    ╚══██╔══╝██║  ██║██╔════╝   ██╔════╝ ██╔══██╗██╔══██╗$(RESET)\n"
	@printf "$(CYAN)$(BOLD)  ███████╗██║     ███████║ ╚████╔╝        ██║   ███████║█████╗     ██║  ███╗██████╔╝███████║$(RESET)\n"
	@printf "$(CYAN)$(BOLD)  ╚════██║██║     ██╔══██║  ╚██╔╝         ██║   ██╔══██║██╔══╝     ██║   ██║██╔══██╗██╔══██║$(RESET)\n"
	@printf "$(CYAN)$(BOLD)  ███████║███████╗██║  ██║   ██║          ██║   ██║  ██║███████╗   ╚██████╔╝██████╔╝██║  ██║$(RESET)\n"
	@printf "$(CYAN)$(BOLD)  ╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝          ╚═╝   ╚═╝  ╚═╝╚══════╝    ╚═════╝ ╚═════╝ ╚═╝  ╚═╝$(RESET)\n"
	@printf "\n"
	@printf "  $(BOLD)Moteur Roguelike Slay the Spire 2 sur Game Boy Advance (1 Héros = 1 ROM)$(RESET)\n"
	@printf "  $(MAGENTA)=================================================================================$(RESET)\n"
	@printf "  $(BOLD)COMMANDE$(RESET)            $(BOLD)DESCRIPTION$(RESET)\n"
	@printf "  $(MAGENTA)---------------------------------------------------------------------------------$(RESET)\n"
	@printf "  $(GREEN)make all-roms$(RESET)       Compile les 5 cartouches GBA simultanément (Ironclad, Silent, ...)\n"
	@printf "  $(GREEN)make ironclad$(RESET)       Compile uniquement l'Édition Soldat de Fer (slay_ironclad.gba)\n"
	@printf "  $(GREEN)make silent$(RESET)         Compile uniquement l'Édition La Silencieuse (slay_silent.gba)\n"
	@printf "  $(GREEN)make defect$(RESET)         Compile uniquement l'Édition Le Défectueux (slay_defect.gba)\n"
	@printf "  $(GREEN)make regent$(RESET)         Compile uniquement l'Édition Le Régent (slay_regent.gba)\n"
	@printf "  $(GREEN)make necrobinder$(RESET)    Compile uniquement l'Édition La Nécrolieuse (slay_necrobinder.gba)\n"
	@printf "\n"
	@printf "  $(CYAN)make test$(RESET)           Lance la suite de tests unitaires C++20 déterministes\n"
	@printf "  $(CYAN)make test-e2e$(RESET)       Lance le test d'intégration bout-en-bout (Run complète)\n"
	@printf "  $(CYAN)make test-all$(RESET)       Exécute l'intégralité des suites de tests\n"
	@printf "  $(CYAN)make play$(RESET)           Lance le simulateur interactif complet dans le terminal\n"
	@printf "\n"
	@printf "  $(YELLOW)make clean$(RESET)          Nettoie les builds et fichiers obsolètes (conserve les 5 ROMs)\n"
	@printf "  $(YELLOW)make clean-all$(RESET)      Supprime TOUT y compris les ROMs GBA compilées\n"
	@printf "  $(YELLOW)make clean-obsolete$(RESET) Supprime les anciennes versions obsolètes (slay_the_gba.gba)\n"
	@printf "  $(YELLOW)make info$(RESET)           Affiche la taille et le statut des 5 cartouches GBA\n"
	@printf "\n"
	@printf "  $(BLUE)make run$(RESET)            Lance la ROM par défaut dans votre émulateur GBA\n"
	@printf "  $(BLUE)make run-silent$(RESET)     Lance l'édition La Silencieuse dans l'émulateur GBA\n"
	@printf "  $(MAGENTA)=================================================================================$(RESET)\n\n"
