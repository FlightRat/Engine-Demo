# Import fixtures

Tiny original test data: triangle OBJ (without UV/tangents), textured OBJ+MTL+2x2 RGBA PNG,
embedded-image glTF (explicitly unsupported), damaged image, missing MTL, valid and invalid GLSL,
and 0.1 seconds of mono PCM silence. No external downloads or copyrighted assets are required.

The native --gl-tests mode uses an actual hidden GL context and SDL dummy audio backend;
it does not prove audible output or editor UI behavior. Test source-removal checks create
and remove only their own uniquely named file next to these fixtures.
