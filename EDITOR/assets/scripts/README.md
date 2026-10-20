# Scene main scripts

The editor starts with an empty, active `default` Scene. It creates
`default.lua` only when the file is absent. Every Scene binds to
`assets/scripts/<scene-name>.lua`, resolved from the editor's asset root.

New Scene scripts return this table:

```lua
local elapsed = 0

return {
    on_start = function()
        elapsed = 0
    end,

    on_update = function(dt)
        elapsed = elapsed + dt
    end,

    on_stop = function()
    end,
}
```

All three callbacks are required. New Play reads the file again and creates a
fresh Lua environment. Pause freezes updates, game time, physics and gameplay
audio. Play while paused resumes that environment without calling `on_start`
again. Stop calls `on_stop`, then discards the runtime scene even if that callback
fails. Runtime changes are never written back to the edit scene.

Creating a Scene reuses an existing same-named Lua file without overwriting it.
Renaming a Scene also renames its Lua file; an occupied destination fails the
whole rename. Deleting a Scene or undoing its creation preserves the Lua file.
Redo binds the current file contents, or generates a template if it is missing.
Names must be valid Windows filenames; names are unique ignoring case. Paths,
device names such as CON/COM1, trailing spaces/dots and illegal filename
characters are rejected. Unicode names are supported.

Switching Scenes only binds and compiles their scripts. Top-level Lua code runs
on Play, not while editing or dragging a Scene. A syntax diagnostic does not
overwrite your file; fix it externally and press Play to retry.

Models, textures, shaders and sounds must be loaded through editor import before
Play. Runtime `AssetManager.add_*` calls fail instead of changing the shared
asset registry. The old `main.lua` and `GameDemo` examples are retained but are
not loaded automatically; migrate their gameplay code into these callbacks and
preload their assets through the editor. External Lua I/O is ordinary I/O and is
not part of Stop restoration.

Scenes remain in memory until explicitly saved with the existing workflow.
This change does not modify the JSON format/loading or automatically restore
Scenes by scanning the script directory.
