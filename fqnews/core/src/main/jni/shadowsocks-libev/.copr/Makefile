.PHONY: deps srpm
.DEFAULT_GOAL := srpm

DOT_COPR := $(dir $(firstword $(MAKEFILE_LIST)))
TOP_DIR := $(realpath $(DOT_COPR)/../)

RPM_DIR := $(TOP_DIR)/rpm
outdir ?= $(RPM_DIR)/SRPMS

HAS_GIT := $(shell command -v git 2> /dev/null)
ifndef HAS_GIT
deps:
	dnf -y install git
else
deps:
endif

srpm: deps
	$(TOP_DIR)/rpm/genrpm.sh -o $(outdir)
