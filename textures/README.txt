Place your bus images here (PNG or JPG). The program loads them at startup.

General body (optional, also side fallback):
  bus_body.png / bus_body.jpg
  bus_body1.png / bus_body1.jpg

Per-face textures (recommended for a realistic look):
  bus_front.png / bus_front.jpg   — front cap (+X = direction of travel after steer rotation)
  bus_back.png  / bus_back.jpg    — rear cap (-X); UVs flipped so the image reads correctly from behind
  bus_window.png / bus_window.jpg — long sides (±Z); left side is U-mirrored vs right; also used on the glass band

Search paths (relative to the process working directory) include:
  textures/...
  ../textures/...
  ../../BusStandSimulator/textures/...

Tip: front/back use 0–1 UVs per quad; sides repeat along length (U scale ~2). Window image can be a photo of glass/panels.
