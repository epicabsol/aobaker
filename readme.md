AOBaker
=======

AOBaker is my shot at making an ambient occlusion baker that I actually like. (XNormal is a massive pain and I don't like the output, SMAK's UI is a nightmare to use and crashes consistently, 3D modeling software baking is unintuitive and frustrating)

Screenshot: (The model is the Nomad from Mass Effect: Andromeda, after separating out shared materials)
![Screenshot](/screenshot.PNG)

AOBaker can bake AO and save the resulting textures, but the UI is still heavily WIP and the bakes currently aren't bled around the borders.

Task List:
----------
 - [X] Direct3D11 rendering with the UI system from the Dear ImGui project
 - [X] Property system (to facilitate serialization and property UI)
 - [X] Mesh rendering
 - [X] Property editing and transform gizmos from the ImGuizmo project
 - [X] Mesh import from the assimp project
 - [ ] Project saving & loading
 - [X] Texture rendering / loading / saving
 - [ ] Bake UI
 - [X] Actual AO baking using the Radeon Rays project
