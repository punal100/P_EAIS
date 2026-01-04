# EAIS Visual Editor Documentation

## Overview

The EAIS Visual Editor provides a node-based interface for creating and editing AI behavior graphs.

## Technology Stack

The visual editor is built using:
- **UEdGraph** - Unreal's graph framework
- **UEdGraphNode** - Node representation
- **UEdGraphSchema** - Graph rules and connections
- **SGraphEditor** - Slate widget for rendering

**Important**: The editor uses Unreal's native graph system, not canvas-based alternatives.

## Architecture

```
┌─────────────────────────────────┐
│     EAIS Graph Editor Tab       │
│  (Dockable Editor Tab)          │
└──────────────┬──────────────────┘
               │ Contains
               v
┌─────────────────────────────────┐
│     SGraphEditor Widget         │
│  (SEAIS_GraphEditor)            │
└──────────────┬──────────────────┘
               │ Uses
               v
┌─────────────────────────────────┐
│     UEdGraph / UEdGraphNode     │
│  (UEAIS_GraphNode)              │
└─────────────────────────────────┘
```

## Key Classes

### UEAIS_GraphNode
Represents an AI state in the graph.

**Properties:**
- StateId - Unique identifier
- bIsTerminal - Whether state is terminal
- OnEnter/OnTick/OnExit - Action arrays
- Transitions - Outgoing transitions

### SEAIS_GraphEditor
Custom SGraphEditor widget.

**Features:**
- Drag state nodes
- Connect transitions
- Inline editing
- Validation display

### UEAIS_GraphSchema
Defines graph rules.

**Rules:**
- What connections are allowed
- How to create new nodes
- Context menu actions

## Opening the Editor

```
Tools → EAIS → EAIS AI Editor
```

## Workflow

### Creating a New Profile
1. Open EAIS AI Editor
2. Click "New Profile"
3. Enter profile name
4. Add states (right-click → Add State)
5. Connect transitions
6. Edit state properties
7. Save profile

### Editing Existing Profile
1. Select profile from dropdown
2. Click "Load"
3. Make changes
4. Click "Save"

### Exporting Runtime JSON
1. Open profile in editor
2. Click "Export Runtime JSON"
3. Validates and strips editor metadata
4. Saves to Content/AIProfiles/

## Editor JSON Format

Editor JSON includes layout metadata:

```json
{
  "schemaVersion": 1,
  "name": "ProfileName",
  "initialState": "Idle",
  "states": [ /* runtime data */ ],
  "editor": {
    "nodes": {
      "Idle": {
        "pos": { "x": 100.0, "y": 100.0 },
        "color": "#3A3A3A",
        "comment": "Entry state"
      }
    },
    "edges": [
      { "from": "Idle", "to": "Active" }
    ],
    "viewport": {
      "zoom": 1.0,
      "pan": { "x": 0.0, "y": 0.0 }
    }
  }
}
```

## Validation

The editor continuously validates:
- All transitions point to existing states
- Initial state exists
- Non-terminal states have transitions or actions
- No circular dependencies without delay

### Visual Feedback
- **Green badge**: Valid state
- **Red badge**: Invalid state
- **Yellow edges**: Warning
- **Red edges**: Error

## Hotkeys

| Key | Action |
|-----|--------|
| Ctrl+S | Save profile |
| Ctrl+N | New state |
| Delete | Delete selected |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| F | Focus on selection |
| Home | Reset viewport |

---

*P_EAIS - Modular AI System*
