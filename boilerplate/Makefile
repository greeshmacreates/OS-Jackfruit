obj-m += monitor.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Static linking for workloads (optional)
WORKLOAD_LDFLAGS ?= -static

USER_TARGETS := engine memory_hog cpu_hog io_pulse

# ---------------- BUILD ----------------

all: $(USER_TARGETS) module

ci: WORKLOAD_LDFLAGS =
ci: $(USER_TARGETS)

# ---------------- USER SPACE ----------------

engine: engine.c monitor_ioctl.h
	gcc -O2 -Wall -Wextra -o engine engine.c -lpthread

memory_hog: memory_hog.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o memory_hog memory_hog.c

cpu_hog: cpu_hog.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o cpu_hog cpu_hog.c

io_pulse: io_pulse.c
	gcc -O2 -Wall $(WORKLOAD_LDFLAGS) -o io_pulse io_pulse.c

# ---------------- KERNEL MODULE ----------------

module: monitor.ko

monitor.ko: monitor.c monitor_ioctl.h
	@if [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) modules; \
	else \
		echo "⚠️ Kernel headers not found. Skipping module build."; \
	fi

# ---------------- CLEAN ----------------

clean:
	@if [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) clean; \
	fi
	rm -f $(USER_TARGETS)
	rm -f *.o *.mod *.mod.c *.symvers *.order
	rm -rf logs
	rm -f *.log
	rm -f /tmp/mini_runtime.sock

.PHONY: all ci module clean
