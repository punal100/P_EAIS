# PLAN.md — Full Complete Plan for P_MEIS → P_MWCS → P_EAIS → P_MiniFootball → A_WCG Integration

**Status:** Actionable, non-backwards-compatible, production-ready plan
**Engine target:** Unreal Engine 5.2 (primary)
**Primary platforms:** Windows (CI runner), macOS (editor/iOS tests), Linux (headless builds optional)
**Date:** 2025-12-29

---

# SUMMARY (one-line)

Deliver a JSON-programmable AI system (P_EAIS) integrated with P_MEIS (input), P_MWCS/A_WCG (visual widgets & editor), and P_MiniFootball (gameplay), with full automated headless build/test/validation scripts, Editor visual AI authoring, in-level testing instructions, and updated README/GUIDE files. Modify any repository files as needed.

---

# 1. Objectives & Constraints

* Build a **JSON-driven AI** (P_EAIS) that designers can author and extend via JSON or a visual Editor Utility (block/graph).
* Integrate tightly with **P_MEIS** (to use or emulate inputs), **P_MWCS/A_WCG** (to produce Editor UI & visual nodes), and **P_MiniFootball** (to provide AI players).
* Provide **automated headless** build, test, and validation scripts for Windows/macOS/Linux and GitHub Actions CI workflows.
* No backwards compatibility: change APIs, rename files, and restructure as needed to make it work.
* Update or create full `README.md` and `GUIDE.md` in all affected repos.
* All edits and new files may be committed directly to the repositories.

---

# 2. Deliverables

* `P_EAIS` plugin with:

  * Runtime module (JSON parser, interpreter, scheduler, blackboard, actions/conditions registry).
  * Editor module (UEdGraph-backed visual AI editor + Editor Utility Widget fallback).
  * Example `UAIBehaviour` assets (JSON) and domain-specific action mappings for P_MiniFootball.
* Integration code and lightweight adapters in `P_MiniFootball`, `P_MEIS`, `P_MWCS`, `A_WCG`.
* Headless build & test scripts (`scripts/`): Windows PowerShell, macOS/Linux Bash.
* GitHub Actions CI workflow(s) for build/test/generate.
* Automated tests: unit, integration, runtime functional tests (Automation Test framework).
* Full `README.md` and `GUIDE.md` for each repo: `P_EAIS`, `P_MiniFootball`, `P_MWCS`, `P_MEIS`, `A_WCG`.
* `PLAN.md` (this file) committed to a repo root (suggest: `P_EAIS/PLAN.md` and project root).

---

# 3. High-level Architecture & Data Flow

1. **P_MEIS** — Input abstraction & emulation: provides named actions/events and an API to synthesize input events (used by AI for test/replay).
2. **P_MWCS / A_WCG** — Editor-side widget generation: converts HTML/CSS/JS or C++ spec providers to UMG/Widget Blueprints and provides a rendering surface for Editor UI nodes. Used by the visual AI editor to render custom node UIs or palettes.
3. **P_EAIS** — JSON-driven AI:

   * **UAIBehaviour assets** hold DSL JSON for behaviors.
   * **FAIInterpreter** executes behavior nodes using a scheduler and maintains a per-AI blackboard.
   * **AEAICustomController / UAIComponent** attach to pawns and invoke actions/conditions.
   * **ActionsRegistry & ConditionsRegistry** map string keys to C++ or Blueprint callbacks (gameplay primitives).
   * Optionally, `EmulateInput` action routes through P_MEIS to simulate player inputs.
4. **P_MiniFootball** — Game-specific actions (kick, pass, tackle, get-ball, team logic); registers action handlers with P_EAIS.

Data flow:

* Designer/author creates JSON or uses Visual Editor → saved to `UAIBehaviour` asset.
* At runtime, `AEAICustomController` loads assigned `UAIBehaviour` and interprets it; actions call into `P_MiniFootball` game API or P_MEIS input emulator.

---

# 4. P_EAIS: JSON DSL & Runtime Design (detailed)

## 4.1 JSON DSL — `eaibehavior.schema.json` (place under `P_EAIS/Content/EAISSchemas/`)

Provide a formal JSON Schema to validate assets at save/load. Minimal, extendable schema:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "EAIS Behavior",
  "type": "object",
  "required": ["id","meta","tick_rate","root"],
  "properties": {
    "id": { "type": "string" },
    "meta": {
      "type":"object",
      "properties":{
        "name":{"type":"string"},
        "author":{"type":"string"},
        "version":{"type":"string"},
        "description":{"type":"string"}
      },
      "required":["name","version"]
    },
    "blackboard_defaults": {
      "type":"object",
      "additionalProperties": true
    },
    "tick_rate": { "type":"number", "minimum": 0.01 },
    "root": { "$ref": "#/definitions/node" }
  },
  "definitions": {
    "node": {
      "type":"object",
      "properties":{
        "type":{"type":"string"},
        "name":{"type":"string"},
        "params":{"type":"object"},
        "children":{"type":"array", "items": {"$ref":"#/definitions/node"}}
      },
      "required":["type"]
    }
  }
}
```

## 4.2 Node types

* `selector`, `sequence`, `parallel`: control flow.
* `condition`: references a condition name registered in ConditionsRegistry; returns boolean.
* `action`: references action name registered in ActionsRegistry; returns Success/Failure/Running.
* `decorator`: e.g., `repeat`, `invert`, `until`, `cooldown`.
* `utility` (optional) for utility-based decision nodes.

## 4.3 Runtime components (C++ recommended)

* `UAIBehaviour : UPrimaryDataAsset` — stores JSON and parsed tree pointer, validation metadata.
* `FEAISNode` / `UEAISGraph` structures — runtime node types.
* `FEAIInterpreter` — tick-based interpreter implementing control-flow semantics.
* `UAIComponent : UActorComponent` — owner pawn’s AI component, exposes Start/Stop/Debug functions.
* `AEAICustomController : AAIController` — default controller to hook into movement/possession; integrates with UAIComponent.
* `FEAIActionsRegistry` — `RegisterAction(name, delegate)` and `InvokeAction(name, context, params)`.
* `FEAIConditionsRegistry` — similar registry for conditions.
* `UEAISBlackboard` — wrapper around a map<string, variant> and optional UE BlackBoard integration.

## 4.4 Extensibility

* Actions and conditions may be registered in plugin startup (`StartupModule`) by C++ modules, or dynamically by Blueprints via exposed registration nodes. This allows P_MiniFootball to register domain-specific actions: `Kick`, `PassTo`, `FindOpenPlayer`, etc.

## 4.5 Debugging & validation

* Validate JSON against schema at save; record validation errors on asset and fail editor save when fatal.
* Provide runtime debug draws and logs (`LogP_EAIS`) and a console command `EAIS.Debug <on|off>` and `EAIS.DumpBlackboard <ActorName>`.

---

# 5. Editor: Visual AI Authoring (detailed)

## 5.1 Approach: UEdGraph-based editor (recommended)

* Implement a custom asset editor using `UEdGraph` / `UEdGraphSchema` / `UEdGraphNode` types.
* Map each graph node to JSON node on save (round-trip).
* Editor modules:

  * `P_EAIS.Editor` — registers asset type `UEAISBehaviourAsset`, custom details panel, and graph editor.
  * `P_EAIS.GraphNodes` — action/condition node types with property panels bound to schema parameters.

## 5.2 Fallback: Editor Utility Widget (rapid prototype)

* Build an Editor Utility Widget (UMG) that uses P_MWCS/A_WCG generated HTML UI for the block palette and node canvas.
* Implement import/export: Editor UI serializes node definitions into the `UAIBehaviour` JSON and saves it as an asset.

## 5.3 Features required

* Node palette: action nodes, condition nodes, composites (sequence/selector).
* Drag-and-drop node creation, connect/disconnect pins with validation.
* Node property inspector: edit params typed by schema (numbers, enums, actor refs).
* Live validation; show errors in the asset.
* Simulation/Preview pane: run the behavior with a simulated blackboard and timeline trace.
* One-click ‘Attach to Selected Pawn’ to apply behavior in the open level.

## 5.4 Implementation notes

* Use P_MWCS to render node visuals if you want HTML-based styling; otherwise use Slate nodes.
* Store asset JSON in `UAIBehaviour` as text field; on load parse JSON into runtime graph.

---

# 6. Integration details & code-level TODOs

## 6.1 P_MEIS integration

* Add an **Input Emulation API** in P_MEIS:

  * `UPMEISBridge::EmulateAction(FName ActionName, bool bPressed)`.
  * `UPMEISBridge::EmulateAxis(FName AxisName, float Value)`.
* P_EAIS provides an `EmulateInput` action that calls P_MEIS functions.
* Add P_MEIS to `PublicDependencyModuleNames` of `P_EAIS` where needed.

## 6.2 P_MWCS / A_WCG integration

* Add a `EAISNodeSpecProvider` to P_MWCS to produce HTML/CSS descriptions for node visuals (optional).
* Add a commandlet or Editor utility to regenerate node visuals: `-run=MWCS_GenerateNodeUI -target=P_EAIS`.

## 6.3 P_MiniFootball integration

* Add `IGameplayAIInterface` in P_MiniFootball:

  * `FVector GetBallLocation();`
  * `AActor* GetClosestTeammate(AActor* Self);`
  * `bool AttemptPass(AActor* From, AActor* To, float Power);`
  * `bool AttemptShot(AActor* Shooter, FVector Target, float Power);`
  * `bool IsInPossession(AActor* Player);`
* In `P_MiniFootball` module `StartupModule()`, register action handlers with `FEAIActionsRegistry`:

  * `Kick`, `PassTo`, `MoveToBall`, `HoldPosition`, `Tackle`.
* Ensure `P_MiniFootball` exposes these functions as Blueprint-callable wrappers.

## 6.4 Example runtime flow for a `PassTo` action

1. Action `PassTo` invoked by interpreter; params: `{ "target": "actor_id", "power": 0.6 }`.
2. Action handler calls `IGameplayAIInterface::AttemptPass(OwnerPawn, targetActor, power)`.
3. On success returns `Success`; on failure `Failure`; if in progress return `Running`.

---

# 7. File / Repo Change Checklist (actionable patch-like list)

> Modify files freely; suggested exact new files and edits.

## P_EAIS (new plugin)

* `Plugins/P_EAIS/P_EAIS.uplugin` — plugin descriptor (modules: Runtime + Editor).
* `Plugins/P_EAIS/Source/P_EAISRuntime/` — add runtime sources:

  * `AIBehaviourAsset.h/.cpp`
  * `EAISParser.h/.cpp`
  * `EAISInterpreter.h/.cpp`
  * `EAIActionsRegistry.h/.cpp`
  * `EAIBlackboard.h/.cpp`
  * `AIComponent.h/.cpp`
  * `EAILog.h`
* `Plugins/P_EAIS/Source/P_EAISTools/` — editor sources:

  * `EAISAssetEditor.h/.cpp`
  * `EAISGraphSchema.h/.cpp`
  * `EAISGraphNode_Condition.h/.cpp`
  * `EAISGraphNode_Action.h/.cpp`
* `Plugins/P_EAIS/Content/EAISSchemas/eaibehavior.schema.json`
* `Plugins/P_EAIS/Content/EAISScripts/` — example JSON behaviours (`attacker_basic.json`, `defender_basic.json`, `goalkeeper.json`)
* `Plugins/P_EAIS/Scripts/` — automation scripts: `build_headless_windows.ps1`, `build_headless_unix.sh`, `run_tests.sh`, `validate_json.py`
* `Plugins/P_EAIS/README.md`, `Plugins/P_EAIS/GUIDE.md`, `Plugins/P_EAIS/PLAN.md` (this file).

## P_MiniFootball (edits)

* `Plugins/P_MiniFootball/Source/P_MiniFootball/Public/AI/IGameplayAIInterface.h` (new)
* `Plugins/P_MiniFootball/Source/P_MiniFootball/Private/AI/AIIntegration.cpp` — register actions with P_EAIS registry
* `Plugins/P_MiniFootball/Content/AIProfiles/` — add example profiles referencing `UAIBehaviour` assets
* Update `P_MiniFootball.uplugin` to require `P_EAIS`, `P_MEIS`, `P_MWCS`.

## P_MEIS (edits)

* `Plugins/P_MEIS/Source/P_MEIS/Public/PMEISBridge.h/.cpp` — add `EmulateAction`, `EmulateAxis` API (exposed to Blueprint & C++).
* Export header in module build file.

## P_MWCS / A_WCG (edits)

* `Plugins/P_MWCS/Editor/SpecProviders/EAISNodeSpecProvider.cpp` — optional: add node UI spec provider for P_EAIS.
* `Plugins/A_WCG/Tools/` — sample HTML node templates used by visual editor.

## Project

* `MyProject.uproject` — enable plugins: `"P_EAIS"`, `"P_MEIS"`, `"P_MWCS"`, `"P_MiniFootball"`, `"A_WCG"`.

---

# 8. Automated Headless Build & Test Scripts (concrete)

> Put these scripts in each repo under `scripts/`. Ensure they are executable.

## 8.1 Windows PowerShell: `scripts/build_headless_windows.ps1`

```powershell
param(
  [string]$UEPath = "C:\Program Files\Epic Games\UE_5.2",
  [string]$Project = "$PWD\MyProject.uproject",
  [string]$Platform = "Win64",
  [string]$Configuration = "Development",
  [string]$ArchiveDir = "$PWD\artifacts\win"
)

# Build editor (Editor target)
& "$UEPath\Engine\Build\BatchFiles\Build.bat" MyProjectEditor $Platform $Configuration "$Project" -waitmutex

# Run UAT BuildCookRun for packaging (headless)
& "$UEPath\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$Project" -noP4 -platform=$Platform -clientconfig=$Configuration -serverconfig=$Configuration -cook -allmaps -build -stage -archive -archivedirectory="$ArchiveDir" -unattended -CrashForUAT -utf8output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Generate widgets via MWCS commandlet
& "$UEPath\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$Project" -run=MWCS_CreateWidgets -unattended -NullRHI
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Run automation tests
& "$UEPath\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$Project" -run=Automation RunTests "EAIS.*" -unattended -NullRHI -log="Logs/Automation.log"
exit $LASTEXITCODE
```

## 8.2 macOS/Linux Bash: `scripts/build_headless_unix.sh`

```bash
#!/usr/bin/env bash
UE_DIR=${UE_DIR:-"/Users/Shared/Epic/UE_5.2"}
PROJECT=${PROJECT:-"$PWD/MyProject.uproject"}
PLATFORM=${PLATFORM:-"Mac"}
CONFIG=${CONFIG:-"Development"}
ARCHIVE_DIR=${ARCHIVE_DIR:-"$PWD/artifacts/mac"}

"$UE_DIR/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$PROJECT" -noP4 -platform=$PLATFORM -clientconfig=$CONFIG -cook -build -stage -archive -archivedirectory="$ARCHIVE_DIR" -unattended -utf8output
if [ $? -ne 0 ]; then exit 1; fi

# Generate widgets
"$UE_DIR/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PROJECT" -run=MWCS_CreateWidgets -unattended -nullrhi
if [ $? -ne 0 ]; then exit 1; fi

# Run automation tests
"$UE_DIR/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PROJECT" -run=Automation RunTests "EAIS.*" -unattended -nullrhi -log="Logs/Automation.log"
exit $?
```

## 8.3 JSON Schema validation helper: `scripts/validate_json.py`

```python
#!/usr/bin/env python3
import json, sys, glob, jsonschema
schema = json.load(open("Content/EAISSchemas/eaibehavior.schema.json"))
failed = 0
for f in glob.glob("Content/EAISScripts/*.json"):
    try:
        data = json.load(open(f))
        jsonschema.validate(data, schema)
        print(f"[OK] {f}")
    except Exception as e:
        print(f"[ERR] {f} -> {e}")
        failed += 1
if failed:
    sys.exit(2)
```

## 8.4 Test runner wrapper: `scripts/run_tests.sh`

* Runs `validate_json.py`, then `build_headless` script, then `UnrealEditor-Cmd` automation tests. Fail fast on errors.

---

# 9. Continuous Integration: GitHub Actions (example)

Create `.github/workflows/ci-unreal.yml` (adjust self-hosted runners when needed):

```yaml
name: CI - Build & Test
on: [push, pull_request]
jobs:
  build-and-test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Cache UE artifacts
        uses: actions/cache@v4
        with:
          path: |
            ~/.cache/ue
          key: ${{ runner.os }}-ue5-5.2
      - name: Setup UE environment
        run: echo "Assume Unreal Engine 5.2 installed on runner"
      - name: Validate JSONs
        run: python scripts/validate_json.py
      - name: Build & Test
        run: powershell -File scripts/build_headless_windows.ps1 -UEPath "C:\Program Files\Epic Games\UE_5.2" -Project "$PWD/MyProject.uproject"
      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: build-artifacts
          path: artifacts/
```

**Notes**

* For macOS editor tests or iOS packaging you must use a self-hosted mac runner with Xcode and Unreal installed or run on your own CI infrastructure.
* Store secrets (keystore, provisioning profiles, passwords) in GitHub Secrets and pass to scripts via environment vars. Never commit secrets.

---

# 10. Automated Tests (detailed list)

## 10.1 Unit tests (C++ Automation)

* `EAIS.Parser.Parse_ValidJSON_ReturnsGraph`
* `EAIS.Parser.Parse_InvalidJSON_ReturnsError`
* `EAIS.Scheduler.BasicSequenceExecution`
* `EAIS.ActionsRegistry.RegisterAndInvoke`

## 10.2 Integration (Editor)

* `EAIS.Editor.SaveLoad_RoundTrip` — create asset programmatically, save, re-open and validate.
* `EAIS.Editor.GenerateWidgets_MWCS` — run MWCS_CreateWidgets and validate generated assets exist.

## 10.3 Runtime functional tests (Automation)

* `EAIS.Functional.Attacker_Behavior_Smoke`:

  * Spawn attacker pawn (AI), assign `attacker_basic.json`, start match, assert within X seconds that attacker moved towards ball and called `AttemptShot`.
* `EAIS.Functional.PassChain`:

  * Two AI players execute a pass sequence; validate ball changes possession and passes succeeded.

## 10.4 Test reporting

* Use Unreal Automation system; results will export to `Saved/Logs/Automation.xml` or similar JUnit-style logs. CI fails if any test fails. Keep `-unattended -nullrhi` flags for headless runs.

---

# 11. In-Level Testing Instructions (how to test AI in a level)

## 11.1 Prepare the test level

1. Open `EAIS_TestMap` (create under `Content/Maps/EAIS_TestMap.umap`).
2. Add `NavMeshBoundsVolume` covering field; build navigation.
3. Place ball actor (P_MiniFootball Ball) and spawn points for players.
4. Add debug UI widget (optional) created by MWCS to show AI debug info.

## 11.2 Spawn & run AI

1. Start PIE as `Listen Server` or run standalone server.
2. Open console (tilde `~`) and run:

   * `EAIS.SpawnBot AttackerProfile` — spawns an AI player using assigned `UAIBehaviour`.
   * `EAIS.Debug on` — enable AI debug logs & on-screen blackboard.
3. Observe AI behavior: movement toward ball, passing, shooting. Use `EAIS.DumpBlackboard <ActorName>` to print blackboard.
4. To simulate input events (test P_MEIS emulation):

   * `EAIS.EmulateInput <ActorName> <ActionName> <value>` — routes input to P_MEIS bridge to exercise emulated inputs.

## 11.3 Manual validation checklist

* AI responds to ball/spawn correctly.
* Calls to game APIs (`AttemptPass`, `AttemptShot`) are logged and succeed/fail deterministically.
* No crashes, infinite loops, or stack overflows.
* Debug visualization matches blackboard values.

---

# 12. README.md & GUIDE.md: templates and required content

Provide full `README.md` and `GUIDE.md` for each repo. Below are condensed ready-to-paste skeletons; include them as files under each plugin root.

## 12.1 P_EAIS/README.md (full)

```
# P_EAIS — JSON-driven AI for Unreal Engine 5.2

## Summary
P_EAIS is a modular plugin enabling data-driven AI via a JSON Behavior DSL and an Editor visual authoring tool. Integrates with P_MEIS for input emulation and P_MiniFootball for gameplay actions.

## Quickstart
1. Enable plugin `P_EAIS` in your `.uproject`.
2. Build the project.
3. Open `EAIS_TestMap`.
4. Create/assign `UAIBehaviour` asset to an AI pawn or use `EAIS.SpawnBot <Profile>` to spawn.

## Files of interest
- `Content/EAISSchemas/eaibehavior.schema.json` — behavior schema.
- `Content/EAISScripts/*.json` — example behaviors.
- `Source/P_EAISRuntime/*` — runtime implementation.
- `Source/P_EAISTools/*` — editor implementation.
- `Scripts/` — automation scripts (build & tests).

## CI / Automation
See `scripts/` and `.github/workflows/ci-unreal.yml` for build and test automation.

## Extending
- Register actions via `FEAIActionsRegistry::Register("ActionName", Delegate)`.
- Register conditions the same way.

## Support
- Use `EAIS.Debug on` to enable debugging.
```

## 12.2 P_EAIS/GUIDE.md (full)

Include:

* JSON schema reference (paste schema).
* Step-by-step Editor authoring flow.
* How to register actions/conditions (C++ examples).
* How to add new nodes to the editor.
* CI integration and examples of running headless tests.

(Place full detailed content; ensure code examples match your codebase.)

## 12.3 P_MiniFootball README/GUIDE additions

* Add “AI Integration” section: show how to enable `P_EAIS`, spawn bots, register AI profiles, and run automated gameplay tests.

---

# 13. Deployment, Rollout & Migration

## 13.1 Branching & rollout

* Use a feature branch: `feature/eaidriver`.
* Create PR with tests attached. CI must succeed (build + tests) before merge to main/staging.

## 13.2 Migration

* Existing AI assets (if any) should be migrated manually or via a converter tool added to `Scripts/convert_old_ai_to_eais.py`. Because backwards compatibility is not required, you may archive old assets into `~Archive/old_ai/`.

## 13.3 Rollback

* Keep `main` stable. If post-merge issues encountered, revert the PR and open a hotfix branch.

---

# 14. Security, Secrets & Signing

* Android keystore passwords, iOS provisioning profiles, and other secrets must be stored in CI secret store (GitHub Secrets) and **not committed**.
* Scripts must read these from env vars and fail if missing with a clear error.

---

# 15. Debugging & Common Issues

* **JSON validation failures:** run `python scripts/validate_json.py` and fix schema errors.
* **Editor graph not round-tripping:** ensure `UEAISBehaviour` asset stores JSON and graph serialization uses consistent node IDs.
* **AI not moving:** check `NavMeshBoundsVolume` and movement component, and confirm `AEAICustomController` rights and server authority.
* **CI failures:** inspect Automation logs under `Saved/Logs/Automation.log` and `Artifacts/`.

---

# 16. Known Risks & Mitigations

* **Performance:** many complex behaviors can impact tick budget — mitigate with coarser `tick_rate`, utility pruning, and pooled interpreters.
* **Editor complexity:** building a custom graph editor is non-trivial — build a UMG prototype first, then migrate to UEdGraph.
* **CI time/cost:** full UE builds are heavy; use cache layers and selective job triggers (e.g., on PR only).

---

# 17. Acceptance Criteria (CI must enforce)

1. `scripts/validate_json.py` returns zero errors for provided example JSON behaviors.
2. Headless build script completes for Windows (`build_headless_windows.ps1`) and produces packaged artifacts.
3. MWCS/CreateWidgets commandlet runs without fatal warnings and produces expected widget assets.
4. Automation tests pass: unit + integration + listed functional tests.
5. `EAIS.SpawnBot AttackerProfile` spawns AI in `EAIS_TestMap` and the bot executes `AttemptShot` or `PassTo` at least once during the test run.

---

# 18. Step-by-step Implementation Plan (actionable tasks)

## Phase 0 — Prep (1 day)

* Create plugin skeleton for P_EAIS. Add empty editor and runtime modules.
* Add JSON schema file and one example behavior JSON.

## Phase 1 — Runtime (3–5 days)

* Implement `UAIBehaviour`, `EAISParser`, `EAISInterpreter`, simple blackboard, `ActionsRegistry`.
* Implement `AEAICustomController` and `UAIComponent`.
* Implement primitives: `MoveToLocation`, `MoveToBall` (query P_MiniFootball), `AttemptPass`, `AttemptShot`—initially stub to log.

## Phase 2 — Game integration (2–3 days)

* Implement `IGameplayAIInterface` in P_MiniFootball and map to real gameplay functions.
* Register P_MiniFootball action handlers with P_EAIS.
* Create `attacker_basic.json` and `defender_basic.json` behaviors.

## Phase 3 — Editor (4–7 days)

* Implement UEdGraph-backed editor or UMG prototype.
* Node palette, property inspector, save/export to JSON.

## Phase 4 — Automation & CI (2–4 days)

* Add `scripts/` (build, test, validate).
* Add GitHub Actions template; wire secrets for signing.
* Add unit & functional tests.

## Phase 5 — Polish & Docs (2–3 days)

* Complete README and GUIDE for all repos.
* Add debug console commands, blackboard UI, and sample maps.

## Phase 6 — Review & Merge

* Run full CI, fix issues, merge to main.

> Phases estimated for planning only; actual commit sizes and times depend on team.

---

# 19. Sample JSON Behavior (place under `P_EAIS/Content/EAISScripts/attacker_basic.json`)

```json
{
  "id": "attacker_basic_v1",
  "meta": { "name": "Attacker Basic", "version": "1.0", "author": "designer" },
  "blackboard_defaults": { "target_actor": null, "last_kick_time": 0.0 },
  "tick_rate": 0.1,
  "root": {
    "type": "selector",
    "children": [
      {
        "type": "sequence",
        "name": "ShootWhenPossible",
        "children": [
          { "type": "condition", "name": "HasBall" },
          { "type": "action", "name": "MoveToGoal", "params": { "tolerance": 200 } },
          { "type": "action", "name": "AttemptShot", "params": { "power": 0.85 } }
        ]
      },
      { "type": "action", "name": "FindBall" }
    ]
  }
}
```

---

# 20. Example C++ Registration Snippet (P_MiniFootball Startup)

```cpp
void FP_MiniFootballModule::StartupModule()
{
    // Register gameplay actions
    FEAIActionsRegistry::Get().Register("AttemptPass", &FMiniFootballAIActions::AttemptPass);
    FEAIActionsRegistry::Get().Register("AttemptShot", &FMiniFootballAIActions::AttemptShot);
    FEAIActionsRegistry::Get().Register("MoveToBall", &FMiniFootballAIActions::MoveToBall);
}
```

---

# 21. Final checklist to commit changes (apply in order)

1. Add `P_EAIS` plugin and example assets.
2. Modify `P_MiniFootball` to expose `IGameplayAIInterface` and register actions.
3. Add P_MEIS `PMEISBridge` emulation functions.
4. Add P_MWCS node spec provider for editor visuals (optional).
5. Add scripts `scripts/*` and CI `.github/workflows/ci-unreal.yml`.
6. Add test files and ensure headless runs work locally.
7. Update README/GUIDE files in each repo.
8. Push feature branch and run CI.

---

# 22. What I could not verify automatically

* Exact file paths and class names in your current repositories (I designed names to fit standard Unreal conventions). When implementing, adjust `#include` and module names to match your repository.
* Exact MWCS/A_WCG commandlet names and options may differ; use `-run=list` on `UnrealEditor-Cmd` to discover registered commandlets if needed.

---

# 23. Next immediate actions (developer tasks you or CI should run now)

1. Create `Plugins/P_EAIS` plugin skeleton and commit.
2. Add `eaibehavior.schema.json` and the `attacker_basic.json` example.
3. Run `python scripts/validate_json.py` to confirm JSONs validate.
4. Run `scripts/build_headless_unix.sh` / `build_headless_windows.ps1` locally to detect environment issues.
5. Iterate on action/condition registration until integration tests pass.

---

# Appendix A — Useful Console Commands (implement)

* `EAIS.SpawnBot <ProfileName>`
* `EAIS.Debug <on|off>`
* `EAIS.DumpBlackboard <ActorName>`
* `EAIS.EmulateInput <ActorName> <ActionName> <value>`

---

# Appendix B — Contact points in code (where to add hooks)

* `GameMode::PostLogin` — spawn bots or assign controllers for bots in play testing.
* `PlayerController::BeginPlay` — initialize P_MEIS input mapping if needed.
* `UAIComponent::BeginPlay` — automatically start assigned behavior if `AutoStart=true`.

---

This PLAN.md is comprehensive and prescriptive. Implement the listed files and scripts; run the included headless scripts locally; push to a feature branch and use the provided CI workflow to validate.
