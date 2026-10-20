# Scene runtime

`PlaybackState` is Editing, Playing or Paused. Only Editing permits content
changes. Paused still owns a runtime session. Toolbar availability is derived
from the state rather than separate button flags.

`GameClock` advances only when the runtime explicitly supplies a Playing frame.
Its fixed physics step is 1/60 s and accepted frame delta is capped at 0.25 s.
Pause preserves the fractional physics step but adds no elapsed wall time.
Editor UI and Scene camera navigation retain real time. Lua gameplay timers use
the session clock; native general-purpose timers retain their normal clock.

Component metadata is registered independently before the first Play. Editing
can add components without constructing or executing a Lua environment.

## Runtime data ownership

`PlaybackController` owns the one runtime session. `SceneObject::GetRegistry()`
always means edit data. `VisibleRegistry()` selects edit data in Editing and
the same runtime data for both viewports in Playing/Paused. Game uses a copied
game camera; Scene keeps its independent editor camera.

The clone supports Identification, Transform, MeshFilter, MeshRender, Light,
CubeCollider, SphereCollider and Physics attributes. It first maps entities,
then rewrites identities, parents and physics user data. Materials are value
copies; loaded GPU assets are shared. PhysicsCommon, PhysicsWorld, rigid bodies,
colliders and Lua environments are rebuilt. Unknown components, legacy script
closures and invalid hierarchies reject Play instead of silently dropping data.

On Stop, runtime selection is cleared, `on_stop` is protected, session sounds
are halted, Lua callback references are released, Lua is closed while its
Registry still exists, and runtime entities/context are reset. Failed start
uses the same cleanup without publishing a Playing state. Update errors pause
the session before another physics step.

## Inputs and sound

The Game image publishes viewport bounds/window identity and focus. Gameplay
input is isolated from the editor's InputManager. Text fields and toolbar/outside
clicks do not reach gameplay. Losing focus leaves simulation running; transitions
discard edge events and relative motion. SDL events are mapped to this data in
normal editor execution; tests directly supply data and never inject OS input.

`SessionAudio` tracks each owned voice, its original volume and whether the
session paused it. Resume does not restart script-paused sounds. A sound channel
or music slot owned elsewhere cannot be replaced. Stop/all-channel operations
only affect the session's voices, and restore their volume baselines. A recording
backend makes ownership testable without an audio device.

## Editing and automation

Only Editing allows content or asset writes, scene lifecycle changes and saving.
Playing/Paused allow a separate, read-only runtime selection and Scene camera
navigation. That selection never becomes a public MCP edit entity ID.

Each committed playback transition invalidates edit revisions and MCP history;
runtime frames themselves do not. MCP continues to query the edit Registry and
publishes `playback_state`; legacy `playing` is true for Playing and Paused.
There are no new public playback tools.

## Manual checks (pending)

No computer-use, CUA, synthetic clicks, drag automation or window control is
allowed for verification. Build and native/SDK tests do not certify these items.

| Status | Human steps | Expected result |
| --- | --- | --- |
| Pending manual verification | Inspect Play/Pause/Stop in Editing, Playing and Paused | Correct image, active color and disabled actions; paused Play says Resume Game |
| Pending manual verification | Create a physical object, show Game and Scene together, then Play | Both show the same motion from independent camera angles |
| Pending manual verification | Pause, navigate Scene camera, then Resume | Objects remain still while paused; navigation works; resume has no time jump |
| Pending manual verification | Focus Game, Scene, an Inspector text field and the toolbar in turn | Only Game-image input controls gameplay; typing/navigation does not leak |
| Pending manual verification | Play preloaded music and effects, Pause/Resume/Stop | Actual sound pauses/resumes/stops cleanly and volume is restored |
| Pending manual verification | Try Inspector, Gizmo, Asset/Scene changes, drag switching and Open/Save during Playing/Paused | Writes are unavailable; read-only inspection remains possible |
| Pending manual verification | Stop after moving/spawning/deleting runtime objects | Both viewports return to the untouched edit scene |
