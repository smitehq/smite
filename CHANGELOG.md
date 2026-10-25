# Changelog

## 0.2.0

First-run experience for a new player, through the crashloop quest.

- Module data (quests and cluster state) is resolved from the binary's directory. Set `SMITE_DATA` to override it.
- Settings, shell history, and quest progress all live in `~/.smite`.
- Telemetry is off until you opt in. Smite asks after the first quest completion, not at startup.
- A new profile starts in `k8s_first_steps`. Finishing a quest prints `Next: …`.
- Quest lists are ordered by difficulty, then name.
- An unimplemented `kubectl` command reports that smite has not implemented it.
- Aliases: `po`/`pod`, `deploy`/`deployment`, `no`, and `describe deploy`.
- Nine quests whose hints require unimplemented commands stay hidden until those commands exist.
- Guided path: First Steps → Deciphering the Logs → The Crashing Pod.
- The image-pull hint names `kubectl create secret generic`, the command that quest accepts.

## 0.1.0

- Initial public simulator: shell, Kubernetes quests, local telemetry, and quest progress.
