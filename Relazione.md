# Technical Development Report: 3D Virtual Art Gallery

## 1. Project Objectives

The main objective of this project is to develop an interactive 3D virtual art gallery using modern OpenGL (Core Profile) and SFML. The application allows the user to walk freely through an enclosed exhibition room, view paintings mounted on the walls under simulated museum spotlight lighting, and interact with each painting to display its title and author through a UI overlay.

The project is structured and delivered in incremental, fully compilable stages (Stage 00 through Stage 12), each one building directly on top of the previous one, showing the chronological evolution of the software from a bare room to a fully lit, walkable, interactive gallery.

## 2. Chronological Development Phases

* **Stage 00 (Window Bootstrap):** Minimal SFML window setup with an event-callback-based main loop (using `sf::Window::handleEvents` with overloaded `handle()` functions for `Closed` and `KeyPressed`), no OpenGL involved yet — just a cleared black window that closes on `Escape` or the close button.

* **Stage 01 (First OpenGL Triangle):** Introduced the modern OpenGL (Core Profile) pipeline via GLAD: shader compilation/linking, VAO/VBO setup, and rendering of a single hardcoded, per-vertex-colored triangle directly in normalized device coordinates (no transformation matrices yet).

* **Stage 02 (3D Cube & Free-Fly Camera):** Introduced GLM transformation matrices (model/view/projection) and rendered an indexed, per-vertex-colored unit cube in 3D space. A basic free-fly camera was added, moving along `cameraFront`/`cameraUp`-derived vectors in response to WASD input, with perspective projection recalculated from the window's aspect ratio.

* **Stage 03 (Mouse-Look FPS Camera):** Extended the Stage 02 camera with full mouse-look controls (`yaw`, `pitch`, `sensitivity`), hiding and recentering the cursor every frame to compute rotation from mouse displacement, with pitch clamped to avoid flipping — the same cube from Stage 02 was still the only object rendered.

* **Stage 04 (Room Geometry):** Replaced the single cube with the first version of the exhibition room itself: a floor and three walls (back, left, right) built as four flat-colored quads, still using only position and color vertex attributes.

* **Stage 05 (Basic Lighting):** Added per-vertex normals to the room geometry and introduced a simple Phong-style lighting model (ambient + diffuse term from a single point light) in the fragment shader, tinting the room's flat vertex colors.

* **Stage 06 (Texture Mapping — Base Room):** Introduced `stb_image`-based texture loading and UV texture coordinates, replacing the flat vertex colors of the room with real image textures for the floor, back wall, and side walls — this textured, lit room is the direct foundation Stage 07 builds on by adding paintings to it.

* **Stage 07 (Painting Display):** Three paintings were mounted on the room's walls as textured quads, each with its own position, scale, and Y-axis rotation to match the orientation of the wall it hangs on. During this stage a texture-stretching artifact was identified and corrected (see Section 3.1), together with an aspect-ratio correction so each painting's quad matches the real width/height ratio of its source image.

* **Stage 08 (Floor-Based Walking):** Implemented free movement across the floor using WASD input. The camera direction vector (which includes vertical pitch, used for looking up/down) was deliberately separated from the movement vector (horizontal only), so that looking up or down while moving no longer causes the player to float or sink. Camera height was fixed at a constant eye level, and simple boundary clamping was added to prevent the camera from passing through the room's walls.

* **Stage 09 (Ceiling):** A ceiling quad was added at the top edge of the walls, closing the room from above, with its own dedicated texture and correctly oriented downward-facing normal for lighting purposes.

* **Stage 10 (Spotlight Lighting System):** The single ambient/diffuse point light used in earlier stages was replaced with a per-painting spotlight system: three cone-shaped lights, each positioned near the ceiling and angled toward its corresponding painting, using inner/outer cutoff angles for a soft-edged cone and distance-based attenuation. A low-intensity ambient term was kept so the rest of the room remains dimly visible outside the light cones, simulating a real gallery lighting design.

* **Stage 11 (Enclosing the Room & Window/Input Robustness):** The room was closed with a fourth wall (opposite the back wall), completing the enclosed exhibition space. This stage also focused heavily on making the application window behave correctly: proper handling of window close/focus events, dynamic resizing, and a toggleable mouse-capture mode (see Section 3.3).

* **Stage 12 (Interactive Information Panels):** A proximity- and orientation-based detection system was implemented to determine which painting, if any, the player is currently close to and facing. Rather than showing information automatically, the final design requires the user to press a dedicated interaction key to open an on-screen panel showing the painting's title and author, mixing 2D UI rendering (SFML) with the underlying 3D OpenGL scene.

## 3. Encountered Difficulties and Technical Solutions

### 3.1 Diagonal Texture Stretching
One painting's texture displayed a diagonal stretching artifact not present in the other two. The root cause was OpenGL's default 4-byte row alignment assumption (`GL_UNPACK_ALIGNMENT`) conflicting with how `stb_image` packs pixel rows with no padding; this only manifests when an image's width is not a multiple of 4, which explains why only one of the three images was affected. The fix was a single call to `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` before uploading texture data.

### 3.2 Silent Shader Compilation Failures
While extending the lighting system, the fragment shader referenced an undefined preprocessor macro (`NR_SPOTLIGHTS`) and a struct field (`color`) that did not exist in the GLSL-side `SpotLight` struct. Because shader compilation and program linking errors were never actually being checked (the corresponding code was left as comments), the failure was completely silent: the program failed to link, and the scene rendered as a black screen with no diagnostic information. This was resolved in two parts: fixing the actual GLSL mismatch, and — more importantly for future debugging — implementing proper `glGetShaderiv`/`glGetProgramiv` error-checking functions that print the driver's compiler/linker log to the console.

### 3.3 Mouse Capture Trapping the Window
An FPS-style mouse-look implementation recentred the cursor to the middle of the window on every frame while the window had focus. Because interacting with the window's own title bar, resize borders, and close button does not cause the window to lose focus, the cursor was being forcibly reset before it could ever reach those controls — effectively trapping the user inside the application with no way to resize, move, or close it other than the `Escape` key. Investigation also revealed that the window-close (`X` button) event was never being read at all, since the event polling loop discarded every event without inspecting its type. The fix combined three changes: properly reading and handling the `Closed` event to allow the close button to work, releasing mouse capture automatically on focus loss, and adding a manual toggle key (`Tab`) so the user can switch between "camera mode" (mouse hidden and locked) and "window mode" (mouse free) at will.

### 3.4 Window Resizing Not Reflected in Rendering
`glViewport` was never called after the initial window creation, so resizing or maximizing the window changed the OS-level window size without updating what OpenGL considered its drawable area, causing the rendered scene to remain fixed at the original resolution. This was fixed by calling `glViewport` once at startup and again whenever a `Resized` event is received.

### 3.5 Painting Scale and Aspect Ratio
Painting quads were originally scaled using placeholder width/height values not derived from the actual source images, causing visible distortion once the stretching bug (3.1) was fixed and the true image proportions became visible. This was corrected by recalculating each painting's horizontal scale from its real pixel dimensions, keeping height as the reference axis.

## 4. Use of External Resources and AI Declaration

In compliance with academic integrity guidelines, the following sources are declared:

* **Academic Materials and University Notes:** The core OpenGL pipeline (VAO/VBO/EBO setup, shader compilation and linking, Phong-style diffuse/ambient lighting), the view/projection matrix setup via GLM, and the mouse-look/FPS camera model were developed based on the theoretical fundamentals and documentation provided throughout the academic course.
* **Use of Artificial Intelligence Assistance:** AI was utilized as an engineering support tool for:
  1. Diagnosing and fixing the texture-stretching artifact (Section 3.1) and deriving the correct aspect-ratio scaling formula for each painting.
  2. Designing the multi-spotlight lighting model (cone attenuation, cutoff angles, distance falloff) and diagnosing the shader compilation/linking failure described in Section 3.2, including adding proper GLSL error-reporting utilities.
  3. Diagnosing and fixing the mouse-capture/window-event bugs described in Section 3.3 and 3.4.
  4. Designing the proximity- and orientation-based painting-detection algorithm and the key-press interaction pattern used for the information panels in Stage 12.
* **Third-Party Open-Source Software:** The architecture relies on standard, legitimate libraries: SFML for window/input/2D UI rendering, GLAD for OpenGL function loading, GLM for linear algebra transformations, and stb_image for texture loading.
* **Graphic Textures:** All wall, floor, ceiling, and painting textures used for diffuse rendering are sourced from open, freely licensed texture libraries.
