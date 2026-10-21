# Scene main scripts

Scene construction and gameplay are separate phases. Creating a Scene loads its
same-named Lua module and calls optional `on_build` once on a candidate edit
Registry. Successful construction publishes editable initial objects. An empty
template produces an empty Scene; the bundled `default.lua` builds the 24-object
GameDemo before Play, so you can edit its transforms, materials and physics.

New Scene scripts return this table:

```lua
local elapsed = 0

return {
    on_build = function()
        -- Create initial editable objects here, once per new Scene instance.
    end,

    on_start = function()
        elapsed = 0
        -- Bind existing runtime objects here; do not recreate authored objects.
    end,

    on_update = function(dt)
        elapsed = elapsed + dt
    end,

    on_stop = function()
    end,
}
```

`on_start`, `on_update` and `on_stop` remain required. Older three-callback files with
declaration-only module bodies are compatible and create an empty Scene. Module top-level code should only
define functions and data: the module is evaluated once for construction and
again in a separate Lua state on each new Play. Lua locals and entity handles
from construction are not carried into runtime.

`on_build` can create/edit entities, components, hierarchy and the initial Game
camera. Its clock stays at zero and input is empty. It has no sound or contact
listener API and cannot register shared assets; physics does not step. Any
syntax, contract or construction failure discards the candidate, preserves the
current Scene and script bytes, and reports a diagnostic. Construction Lua and
its temporary context are released before the Scene is published.

Play clones the edited data and creates fresh physics, Lua and game camera
state. `Entity.find(name)` finds an entity in the current Lua Registry, returns
nil if absent, and reports ambiguous duplicate names. Use it in `on_start` to
bind existing objects instead of creating them from original definitions.
The default demo uses stable names `body` and `platform_2`; update its binding
code if you rename those objects.

Pause freezes updates, game time, physics and gameplay audio. Resume uses the
same environment without calling `on_build` or `on_start` again. Stop calls
`on_stop`, discards runtime data, and returns to your edited objects with their
identities and properties preserved. Switching or renaming a Scene also never
runs `on_build` again or resets authored edits.

Every Scene binds to `assets/scripts/<scene-name>.lua` under the editor asset
root. Missing files receive a four-callback template; existing files are reused
without overwriting. Rename moves the Lua file transactionally; an occupied
destination fails. Delete and creation undo retain the script. Redo constructs
a new Scene from the current file and runs its `on_build` once. Changing the
file while a Scene exists does not rebuild that Scene automatically.

Names follow Windows filename rules and are unique ignoring case. Paths,
device names such as CON/COM1, trailing spaces/dots and illegal filename
characters are rejected. Unicode scene names and paths are supported.

Models, textures, shaders and sounds must be loaded before construction/Play.
At startup the optional `assets/scripts/preload.lua` returns a declarative list
of shared assets, with paths relative to the asset root. The editor uses its
normal import pipeline before constructing default. A missing source or import
failure reports its key and aborts startup. Other resources can be imported in
Editing. Runtime and construction `AssetManager.add_*` calls remain forbidden.

The bundled preload list includes the old `LoadAssets()` assets plus cyborg,
nanosuit and PBR textures. `default.lua` loads GameDemo helper definitions in
both independent phases, supplements missing data from
`GameDemo/default_scene_defs.lua`, builds objects in `on_build`, then binds
controllers and starts audio in `on_start`. Existing GameDemo definitions take
precedence. `main.lua` is a retained reference and is not executed automatically.

Edits survive Stop and Scene switches in the current editor session. They are
not written back into Lua or autosaved: restarting/recreating constructs from
the script again. Use the existing explicit save workflow where applicable.
Scene JSON loading/format is unchanged. External Lua I/O is not part of Stop
restoration, and the construction Lua omits io/os/package libraries.
