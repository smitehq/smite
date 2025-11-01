# smite.sh

![Smite Logo](https://avatars.githubusercontent.com/u/240213050?s=88&v=4)

**smite.sh** is a CLI-based troubleshooting simulator for DevOps apprentices. Dive into interactive scenarios mimicking real-world Kubernetes (K8s), AWS EKS, and Linux environments. Learn by breaking and fixing—quests guide you through outages, scaling issues, and config mishaps, all in a safe, emulated REPL.

Built with C++17, yaml-cpp for state, fmt for styled output, and a modular router for plug-and-play extensions. No GC, but RAII keeps it tight. Since October 2025, it's your journey to mastery.

## Features
- **Interactive REPL**: Type shell commands, K8s `kubectl`, or AWS CLI—output feels real (tables, JSON, errors).
- **Modular Design**: Load/unload environments (K8s, AWS, Linux shell)—commands route by prefix.
- **Quests & Progress**: Opt-in challenges with conditions (e.g., "fix CrashLoopBackOff"). Eval during load or post-command.
- **Chaining Support**: `cd /etc && chmod rw-r--r-- config` works—stops on errors.
- **Static Linking**: Single `smite.exe`—portable, no DLL hell (MinGW/Clang).
- **Emulated State**: YAML-driven FS/pods/nodes—edit for new scenarios.

## Installation
1. **Prerequisites**:
   - MSYS2 MinGW-w64 (for Windows; or native on Linux/macOS).
   - Clang++ (15+ recommended; `pacman -S mingw-w64-x86_64-clang` in MSYS2).

2. **Clone & Build**:
    ```bash
    git clone git@github.com:smitehq/smite.git
    cd smite
    make  # Or make clean && make
    ./smite.exe  # Or ./smite on Linux/macOS
    ```

## Usage
Run `./smite.exe`—welcome banner loads modules, then REPL (`$ ` prompt).

### REPL Commands
- **Engine Commands**:
- `help` → All prefixes + engine cmds.
- `modules` → Loaded modules (e.g., linux (6 cmds), kubernetes (5)).
- `quests` → List all; `quests kubernetes` → module-specific; `quests kubernetes 0` → Activate quest 0.
- **Module Commands**:
- Linux: `ls`, `cd /etc`, `cat notes`, `chmod rw-r--r-- config`, `touch newfile`.
- K8s: `kubectl get pods`, `kubectl logs backend`, `kubectl describe pod backend`, `kubectl edit deployment api-service`.
- AWS: `aws eks list-clusters`, `aws eks describe-nodegroup smite-cluster workers`, `aws eks update-nodegroup-config smite-cluster workers 3`.
- **Chaining**: `cd /etc && ls` → Silent cd, then list.

Example Session:
```bash
$ ls
notes (rw-r--r--)

$ cat notes
Hint: cd /etc && chmod rw-r--r-- config to fix

$ cd /etc && ls
config (r--r--r--)

$ chmod rw-r--r-- config && kubectl get pods
Permissions updated for config
NAME		READY	    STATUS		RESTARTS
backend	    0/1	        Running	    0

$ quests kubernetes 0
Activated quest 0 for kubernetes. Re-run app to check progress.

$ quit
Journey ends. Farewell, Apprentice.
```

Re-run to check quest progress (e.g., "The Crashing Pod: Complete" after edit).

## Modules
- **Linux** (Always Loaded): Basic shell (cd, ls, cat, chmod). Emulates FS with perms—quests for perms fixes.
- **Kubernetes**: `kubectl` cmds for pods/logs/describe/edit. Emulates cluster with crashing backend pod.
- **AWS**: AWS CLI for EKS (`aws eks list-clusters`, `describe-nodegroup`, `update-nodegroup-config`). Emulates under-scaled nodegroup.

Add new: Drop `./modules/newmodule/module.cpp` + YAMLs, add factory to main.cpp—plug-and-play.

## Quests
Opt-in challenges (via `quests <module> <id>`):
- **Kubernetes**: "The Crashing Pod" — Fix backend outage (edit deployment, eval status).
- **AWS**: "Scale the Node Group" — Increase workers (update-nodegroup, eval size).
- **Linux**: "Fix Config Permissions" — Chmod /etc/config, eval perms.

Progress evals on re-run (or post-command hook if extended). Add to quests.yaml, activate via REPL.

## Building
- **Makefile**: Clang++ with static linking (`-static`), yaml-cpp, fmt, stdc++fs.
- **Dependencies** (MSYS2): `pacman -S mingw-w64-x86_64-clang mingw-w64-x86_64-yaml-cpp mingw-w64-x86_64-fmt mingw-w64-x86_64-toolchain`.
- **Custom**: Add `-DDEBUG` to CXXFLAGS for checkpoints.

Cross-platform: Makefile works on Linux/macOS (adjust paths).

## Contributing
- Fork, branch, PR.
- Code: C++17, RAII, smart pointers—no leaks.
- Tests: Add to `test/` (none yet; focus REPL).
- Issues: YAML formats, new modules, full bash (`;`, `|`).

## License
MIT—use, modify, smite responsibly.

---

*Built with ❤️*