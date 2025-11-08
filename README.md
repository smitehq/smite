# smite.sh

![Smite Logo](https://avatars.githubusercontent.com/u/240213050?s=88&v=4)

*smite.sh* is a CLI-based troubleshooting simulator for DevOps apprentices. Dive into interactive scenarios mimicking real-world Kubernetes (K8s), AWS EKS, and Linux environments. Learn by breaking and fixing-quests guide you through outages, scaling issues, and config mishaps, all in a safe, emulated REPL.

Built with C++17, *yaml-cpp* for state, *fmt* for styled output, and a modular router for plug-and-play extensions.

---

## Features

-   **Interactive REPL**: Type shell commands, K8s `kubectl`, or AWS CLI-output feels authentic.
-   **Cluster-Aware K8s Simulator**: Emulates nodes, pods, and events; supports `kubectl get pods -o wide`, `describe`, and `logs`.
-   **Quests & Progression**: Scenarios like _CrashLoopBackOff_ or misconfigurations with progression tracking.
-   **Chaining Support**: `cd /etc && chmod rw-r--r-- config`-executes sequentially.
-   **Static Linking**: Single binary (`smite.exe`), no DLL dependencies.
-   **YAML-Driven State**: Edit `state.yaml` to define clusters, nodes, pods, and events.
    
---

## Installation

1.  **Prerequisites**
    
    -   MSYS2 MinGW-w64 (Windows) or Clang toolchain (Linux/macOS)
    -   Clang++ 17+
        ```bash
        pacman -S mingw-w64-x86_64-{clang,yaml-cpp,fmt,ncurses,readline}
        ```
        
2.  **Clone & Build**

    ```bash
    git clone git@github.com:smitehq/smite.git
    cd smite
    make # or make clean && make
    ./smite.exe # Windows
    ./smite # Linux/macOS
    ```
    
---

## Usage

Run `./smite.exe` - the banner welcomes you, loads modules, and drops you into the `$` REPL.

### REPL Commands

#### Engine

```
help → show available modules and commands
modules → list loaded modules
quests → list available quests
quests k8s 0 → activate quest 0 for Kubernetes
quit → exit
```

#### Linux Module

```
ls, cd, cat, chmod, touch, nano
```

#### Kubernetes Module

```
kubectl get pods
kubectl get pods -o wide
kubectl describe pod <name>
kubectl get events
kubectl get nodes
kubectl describe node <name>
kubectl logs <pod>
```

#### AWS Module

```
aws eks list-clusters
aws eks describe-nodegroup <cluster> <nodegroup>
aws eks update-nodegroup-config <cluster> <nodegroup> <count>
```

---

### Example Session

```console
⚡  Welcome, Apprentice. Your journey begins.

zaphod@lappy486:~$ kubectl get pods -o wide
NAME       READY  STATUS            RESTARTS  NODE     IP          IMAGE
backend    0/1    CrashLoopBackOff  5         node-1   10.244.0.5  my-backend-image:v1
frontend   1/1    Running           0         node-2   10.244.0.6  nginx:latest

zaphod@lappy486:~$ kubectl get events wide
LAST SEEN            TYPE     REASON    OBJECT    MESSAGE
2025-10-24T14:00:01  Warning  Failed    backend   Failed to pull secret 'db-secret'
2025-10-24T14:01:01  Normal   Pulling   backend   Pulling image "my-backend-image:v1"
2025-10-24T14:01:30  Warning  BackOff   backend   Back-off restarting failed container

zaphod@lappy486:~$ kubectl describe pod backend
Name:         backend
Node:         node-1
Status:       CrashLoopBackOff
Restarts:     5
Image:        my-backend-image:v1
Last State:
  Reason:     Error
  Exit Code:  1
  Started:    2025-10-24T14:00:00
  Finished:   2025-10-24T14:00:01

Events:
  Warning  Failed  Failed to pull secret 'db-secret'
  Normal   Pulling Pulling image "my-backend-image:v1"
  Warning  BackOff Back-off restarting failed container
```

---

## Modules

| Module | Description |
| --- | --- |
| Linux | File and permission management simulator. |
| Kubernetes | Cluster-aware emulation of nodes, pods, and events. |
| AWS | Simulates EKS clusters and nodegroup scaling. |

Each module registers its own command namespace (`kubectl`, `aws`, etc.) and state file. Add new ones via `src/modules/<name>/`.

---

## Quests

Activate via `quests <module> <id>`:

-   **Kubernetes** – “The Crashing Pod”: Fix a backend in CrashLoopBackOff.
-   **AWS** – “Scale the Node Group”: Increase node count to resolve load issues.
-   **Linux** – “Fix Config Permissions”: Correct `/etc/config` file permissions.
    
---

## Building

-   Uses static linking.
-   Supports MSYS2, Linux, and macOS (adjust paths).
-   Debugging:
    ```
    make clean && make DEBUG=1
    ```

---

## Contributing

-   PRs welcome - focus on gameplay feel and extensibility.
-   Code style: modern C++17, RAII, smart pointers.
-   Scenarios: add YAMLs under `/modules/<name>/`
    - Module state in `state.yaml` and quests in `quests.yaml`
    
---

## License

MIT - use, modify, and smite responsibly.

---

*Built with ❤️*