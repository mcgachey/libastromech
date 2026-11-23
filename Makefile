# libastromech monorepo
#
# Each implementation (python/, java/, c/) is built and tested on its own and
# never needs files outside its directory. The top-level config/ directory is the
# single source of truth for shared data; `make sync-config` copies it into each
# implementation so those directories stay self-contained.
#
#   make                 build every implementation that exists
#   make test            check config sync, then test every implementation
#   make <lang>-build    build one implementation   (python, java, c, esp32)
#   make <lang>-test     test one implementation
#   make python-deploy   test, build and upload to PyPI (REPO=testpypi for TestPyPI)
#   make sync-config     copy config/ into each implementation
#   make check-config    fail if any implementation's config copy has drifted

PYTHON ?= python3
VENV   ?= python/.venv

# Where each implementation keeps its copy of config/.
CONFIG_DEST_python := python/src/config
CONFIG_DEST_java   := java/src/main/resources/config
CONFIG_DEST_c      := c/config
CONFIG_DEST_esp32  := esp32/config

IMPLS := python java c esp32
# Only implementations whose directory exists are touched.
PRESENT := $(foreach i,$(IMPLS),$(if $(wildcard $(i)),$(i)))

.PHONY: all build test clean sync-config check-config \
        $(foreach i,$(IMPLS),$(i)-build $(i)-test $(i)-clean) python-deploy python-dist-check

all: build

build: $(addsuffix -build,$(PRESENT))
test: check-config $(addsuffix -test,$(PRESENT))
clean: $(addsuffix -clean,$(PRESENT))

# ---- shared config ---------------------------------------------------------

sync-config:
	@set -e; for i in $(PRESENT); do \
	  case $$i in \
	    python) d=$(CONFIG_DEST_python);; \
	    java)   d=$(CONFIG_DEST_java);; \
	    c)      d=$(CONFIG_DEST_c);; \
    esp32)  d=$(CONFIG_DEST_esp32);; \
	  esac; \
	  echo "sync config -> $$d"; \
	  rm -rf "$$d"; mkdir -p "$$d"; cp -R config/. "$$d"/; \
	  if [ $$i = esp32 ]; then $(PYTHON) esp32/tools/gen_config.py; fi; \
	done

check-config:
	@set -e; for i in $(PRESENT); do \
	  case $$i in \
	    python) d=$(CONFIG_DEST_python);; \
	    java)   d=$(CONFIG_DEST_java);; \
	    c)      d=$(CONFIG_DEST_c);; \
    esp32)  d=$(CONFIG_DEST_esp32);; \
	  esac; \
	  if ! diff -r config "$$d" >/dev/null 2>&1; then \
	    echo "ERROR: $$d is out of sync with config/. Run 'make sync-config'."; exit 1; \
	  fi; \
	  if [ $$i = esp32 ]; then $(PYTHON) esp32/tools/gen_config.py --check || exit 1; fi; \
	done; echo "config in sync"

# ---- python ----------------------------------------------------------------

$(VENV)/.stamp: python/pyproject.toml
	$(PYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install -q --upgrade pip build
	$(VENV)/bin/pip install -q -e python
	@touch $@

python-build: $(VENV)/.stamp
	$(VENV)/bin/python -m build python

# No unit tests exist yet: this compiles all sources and smoke-imports the
# package, including loading every shipped config file. If python/tests exists,
# pytest is run as well.
python-test: $(VENV)/.stamp
	$(VENV)/bin/python -m compileall -q python/src
	$(VENV)/bin/python -c "import libastromech, pathlib, yaml; \
	[yaml.safe_load(p.read_text()) for p in pathlib.Path('$(CONFIG_DEST_python)').rglob('*.yml')]; \
	print('python smoke test ok')"
	@if [ -d python/tests ]; then \
	  $(VENV)/bin/pip install -q pytest && $(VENV)/bin/pytest python/tests; \
	fi

# Upload to PyPI. Publishing is irreversible (a version can never be re-uploaded),
# so this runs the tests, rebuilds from a clean dist/, validates the metadata and
# asks for confirmation. Credentials come from ~/.pypirc or TWINE_USERNAME /
# TWINE_PASSWORD (use __token__ and an API token). Set CONFIRM=yes to skip the
# prompt in CI. Dry run: `make python-dist-check`.
REPO ?= pypi

python-dist-check: python-test
	rm -rf python/dist
	$(VENV)/bin/python -m build python
	$(VENV)/bin/pip install -q twine
	$(VENV)/bin/twine check python/dist/*

python-deploy: python-dist-check
	@v=$$($(VENV)/bin/python -c "import importlib.metadata as m; print(m.version('libastromech'))"); \
	if [ "$(CONFIRM)" != "yes" ]; then \
	  printf "Upload libastromech $$v to $(REPO)? [y/N] "; read a; \
	  case $$a in y|Y) ;; *) echo aborted; exit 1;; esac; \
	fi
	$(VENV)/bin/twine upload --non-interactive --repository $(REPO) python/dist/*

python-clean:
	rm -rf python/build python/dist python/src/*.egg-info $(VENV)

# ---- java ------------------------------------------------------------------
# Uses Maven or Gradle, whichever the implementation provides.

java-build:
	@if [ -f java/pom.xml ]; then cd java && mvn -B -DskipTests package; \
	elif [ -f java/build.gradle ] || [ -f java/build.gradle.kts ]; then cd java && ./gradlew build -x test; \
	else echo "java: no build file yet, skipping"; fi

java-test:
	@if [ -f java/pom.xml ]; then cd java && mvn -B test; \
	elif [ -f java/build.gradle ] || [ -f java/build.gradle.kts ]; then cd java && ./gradlew test; \
	else echo "java: no build file yet, skipping"; fi

java-clean:
	@if [ -f java/pom.xml ]; then cd java && mvn -B clean; \
	elif [ -f java/build.gradle ] || [ -f java/build.gradle.kts ]; then cd java && ./gradlew clean; fi

# ---- c ---------------------------------------------------------------------
# Delegates to c/Makefile, which must provide all, test and clean targets.

c-build:
	@if [ -f c/Makefile ]; then $(MAKE) -C c; else echo "c: no Makefile yet, skipping"; fi

c-test:
	@if [ -f c/Makefile ]; then $(MAKE) -C c test; else echo "c: no Makefile yet, skipping"; fi

c-clean:
	@if [ -f c/Makefile ]; then $(MAKE) -C c clean; fi

# ---- esp32 -----------------------------------------------------------------
# PlatformIO (from PATH, e.g. /opt/esp-venv/bin). Builds every example for the
# board and runs the host-side protocol unit tests (no hardware needed).

PIO ?= pio

esp32-build:
	cd esp32 && $(PIO) run

esp32-test:
	cd esp32 && $(PIO) test -e native

esp32-clean:
	cd esp32 && $(PIO) run -t clean; rm -rf .pio
