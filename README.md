# OS-Jackfruit — Multi-Container Runtime

## 1. Team Information

| Name        | SRN           |
| ----------- | ------------- |
| Greeshma N  | PES1UG24CS175 |
| Harshitha P | PES1UG24CS186 |

---

## 2. Build, Load, and Run Instructions

### Prerequisites

* Ubuntu 22.04 or 24.04 VM (recommended)
* WSL works partially (kernel module not supported)

---

### Install dependencies

```bash
sudo apt update
sudo apt install -y build-essential
```

---

### Build

```bash
gcc engine.c -o engine -lpthread
```

---

### Start supervisor (Terminal 1)

```bash
./engine supervisor
```

---

### Run containers (Terminal 2)

```bash
sudo ./engine run alpha / "echo hello"
sudo ./engine run beta / "ls"
```

---

## 3. Demo Screenshots

### Screenshot 1 — Container execution

![multi-container](screenshots/01-multi-container.png)

---

### Screenshot 2 — Metadata tracking

![ps-metadata](screenshots/02-ps-metadata.png)

---

### Screenshot 3 — Logging system

![logging](screenshots/03-logging.png)

---

### Screenshot 4 — Multiple containers

![cli-ipc](screenshots/04-cli-ipc.png)

---

### Screenshot 5 — Soft limit logs

![soft-limit](screenshots/05-soft-limit.png)

---

### Screenshot 6 — Hard limit enforcement

![hard-limit](screenshots/06-hard-limit.png)

---

### Screenshot 7 — Scheduling experiment

![scheduling](screenshots/07-scheduling.png)

---

### Screenshot 8 — Clean teardown

![teardown](screenshots/08-teardown.png)

---

## 4. Engineering Analysis

### 4.1 Isolation Mechanisms

The runtime uses process-level isolation techniques. Containers are created using:

* `fork()` to create a child process
* `chroot()` to isolate filesystem
* `exec()` to run commands inside the container

Unlike full container runtimes, namespace isolation using `clone()` is not implemented.

---

### 4.2 Supervisor and Process Lifecycle

A supervisor process runs continuously and manages containers.

* Prevents zombie processes using `wait()`
* Handles child process lifecycle
* Ensures proper cleanup after execution

---

### 4.3 IPC, Threads, and Synchronization

The project uses:

**Pipes (IPC):**

* Used to capture container output
* `dup2()` redirects stdout/stderr

**Logging system:**

* Implemented using bounded buffer
* Producer → container output
* Consumer → logging thread

**Synchronization:**

* `pthread_mutex_t` for mutual exclusion
* Condition variables to avoid busy waiting

---

### 4.4 Memory Management

The system demonstrates memory usage behavior:

* Soft and hard limits observed via logs
* RSS (Resident Set Size) used as metric
* Demonstrates how processes consume memory

---

### 4.5 Scheduling Behavior

Linux uses Completely Fair Scheduler (CFS):

* CPU-bound processes → high CPU usage
* I/O-bound processes → scheduled quickly
* Nice values affect CPU allocation

---

## 5. Design Decisions and Tradeoffs

* Used `chroot()` instead of full namespaces → simpler
* Used pipes instead of sockets → easier IPC
* Fixed-size buffer → prevents overflow
* Single supervisor loop → simpler design

---

## 6. Scheduler Experiment Results

### CPU priority experiment

| Container | Nice value | CPU%  |
| --------- | ---------- | ----- |
| cpu_hi    | -5         | High  |
| cpu_lo    | +15        | Lower |

### CPU vs I/O

| Type      | Behavior                       |
| --------- | ------------------------------ |
| CPU-bound | Continuous CPU usage           |
| I/O-bound | Sleeps and resumes efficiently |

---

## 7. Limitations

* Kernel module not supported in WSL
* CLI commands (`start`, `ps`, `logs`, `stop`) not implemented
* Namespace isolation not fully implemented
* Simplified container runtime

---

## 8. Conclusion

This project demonstrates:

* Process management
* Synchronization
* IPC
* Basic containerization

---

## 👩‍💻 Author

Greeshma N and Harshitha P

