from pathlib import Path

root = Path(__file__).parents[1]
html = (root / "Web/index.html").read_text(encoding="utf-8")
script = (root / "Web/app.js").read_text(encoding="utf-8")

for element_id in ("home", "games", "gameGrid", "modal", "guide", "settingsPanel"):
    assert f'id="{element_id}"' in html, f"missing #{element_id}"

for feature in ("localStorage", "aurorax-favorites", "aurorax-settings", "toggleGuide", "activeFilter"):
    assert feature in script, f"missing feature: {feature}"

assert 'data-filter="recent"' in html
assert 'data-filter="installed"' in html
print("AuroraX web smoke test passed")
