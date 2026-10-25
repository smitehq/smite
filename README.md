# smite.sh

![Smite Logo](https://avatars.githubusercontent.com/u/240213050?s=88&v=4)

*smite.sh* is a CLI troubleshooting simulator for DevOps apprentices. You get a shell and a small Kubernetes cluster, then quests that ask you to notice what is broken and fix it. Nothing here talks to a real cluster.

Built with C++20, yaml-cpp, fmt, and readline.

---

## 60-second quickstart

```bash
git clone https://github.com/smitehq/smite.git
cd smite
make
./smite --version    # smite.sh 0.2.0
./smite
```

A new profile starts in **First Steps in Kubernetes**. Run:

```text
kubectl version
```

Smite completes the quest and prints the next one:

```text
Next: Deciphering the Logs (quests kubernetes k8s_reading_logs)
```

Continue the guided path:

```text
quests kubernetes k8s_reading_logs
kubectl logs backend

quests kubernetes k8s_crashloop
kubectl get pods
kubectl describe pod backend
kubectl logs backend
kubectl get secrets
kubectl create secret generic db-secret --from-literal username=admin --from-literal password=supersecret
```

The backend pod leaves CrashLoopBackOff. Type `quests` to browse further scenarios, `hint` if you are stuck, and `quit` to leave.

After that first completion, smite asks whether to keep a local post-mortem (commands, time, hints). It stays off unless you say yes. Change it later with `settings telemetry enable` or `settings telemetry disable`. Nothing is sent off the machine.

---

## Install

### Linux amd64 release

A version tag (`0.2.0` or `v0.2.0`) publishes `smite-<version>-linux-amd64.tar.gz` on [GitHub Releases](https://github.com/smitehq/smite/releases). The archive contains the `smite` binary, `LICENSE`, `README.md`, and the Kubernetes quest and state YAML under `src/modules/`. Unpack it and run `./smite`. The binary loads `src/modules` from beside itself. Set `SMITE_DATA` to that `src/modules` directory if you relocate the binary.

The binary links yaml-cpp, fmt, readline, and ncurses dynamically. Install those shared libraries on the machine that runs it.

### Build from source

Clang++ with C++20, plus yaml-cpp, fmt, readline, and ncurses.

```bash
# Debian/Ubuntu
sudo apt-get install clang libyaml-cpp-dev libfmt-dev libreadline-dev libncurses-dev

# macOS
brew install yaml-cpp fmt readline ncurses

# MSYS2
pacman -S mingw-w64-x86_64-clang mingw-w64-x86_64-yaml-cpp mingw-w64-x86_64-fmt mingw-w64-x86_64-ncurses mingw-w64-x86_64-readline
```

```bash
make
./smite          # Linux/macOS
./smite.exe      # Windows
```

Quest and cluster YAML are loaded from `src/modules` next to the binary (the `make dist` layout includes that tree). If you move the binary, set `SMITE_DATA` to the `modules` directory.

Player state is not stored beside the binary. Settings, readline history, and quest progress go in `~/.smite`:

```text
~/.smite/settings.yaml
~/.smite/quest_state.yaml
~/.smite/.history
```

---

## REPL

Prompt: `zaphod@lappy486:~$`

| Command | What it does |
| --- | --- |
| `help` | List command prefixes |
| `modules` | List loaded modules |
| `quests` | Open the quest browser |
| `quests kubernetes` | List unlocked Kubernetes quests |
| `quests kubernetes <id>` | Start that quest |
| `hint` | Next hint for the active quest |
| `settings` | Show telemetry |
| `settings telemetry enable` / `disable` | Turn local post-mortems on or off |
| `quit` | Exit |

Shell commands: `ls`, `cd`, `pwd`, `cat`, `echo`, `touch`, `chmod`, `nano`.

`&&` runs a second command when the first one did not fail.

### kubectl

These commands are implemented. Short names work the way kubectl players type them.

```text
kubectl version
kubectl get pods | po | pod
kubectl get nodes | no
kubectl get deployments | deployment | deploy
kubectl get services | svc
kubectl get configmaps | cm
kubectl get secrets
kubectl get events
kubectl describe pod | po <name>
kubectl describe node | no <name>
kubectl describe deployment | deploy <name>
kubectl logs <pod>
kubectl delete pod | po <name>
kubectl top pods
kubectl top nodes
kubectl create secret generic <name> --from-literal <key>=<value>
kubectl create configmap <name> --from-literal=<key>=<value>
kubectl edit deployment | deploy <name>
kubectl edit service <name>
kubectl scale deployment | deploy <name> --replicas=<n>
kubectl rollout <status|history|undo> deployment <name>
kubectl cordon <node>
kubectl uncordon <node>
```

Anything else that starts with `kubectl` is reported as not implemented. It is not a shell "command not found" error: smite knows you meant kubectl and does not emulate that subcommand yet.

---

## Quests

Guided path for a new player:

1. `k8s_first_steps` — run `kubectl version`
2. `k8s_reading_logs` — run `kubectl logs backend`
3. `k8s_crashloop` — create the missing `db-secret` (see the quickstart)

Other playable scenarios cover image pull failures, probes, scaling, rollouts, services, configmaps, and node pressure. A quest file whose hints depend on commands smite does not implement yet (ingress, network policy, drain, quotas, statefulsets, and similar) is hidden from `quests` until that command exists.

Add a scenario as `src/modules/kubernetes/quests/<id>.yaml` plus a matching file under `state/`. Set `available: false` when a hint tells the player to run a command the simulator cannot perform.

---

## Modules

| Module | Description |
| --- | --- |
| Shell | Everyday commands (`ls`, `cd`, `cat`, `nano`, …) against an in-memory filesystem. |
| Kubernetes | Nodes, pods, deployments, services, and the quest cluster state. |

---

## Build notes

```bash
make            # debug binary
make tests      # unit tests
make clean && make
```

`make release` and `make dist` produce a stripped binary and a tarball (`dist/smite-<version>-linux-amd64.tar.gz` on Linux). Pushing a version tag runs that `make dist` in GitHub Actions and uploads the archive to the release. Windows builds are statically linked; Linux and macOS link the libraries listed above.

---

## Contributing

PRs are welcome, especially ones that make a quest completable with the commands the hints name. C++20, RAII, and the existing module interface.

---

## License

MIT. Use, modify, and smite responsibly.
