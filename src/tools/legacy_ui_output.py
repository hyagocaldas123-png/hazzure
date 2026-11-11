"""Keep historical UI templates from overwriting the maintained modular UI."""
from pathlib import Path
import argparse
def protected_output(default):
    parser=argparse.ArgumentParser(description='Historical UI template export; the maintained modular UI is protected.')
    parser.add_argument('--output',type=Path,default=Path(default))
    destination=parser.parse_args().output
    if destination.exists() and '#include "UIState.inl"' in destination.read_text(encoding='utf-8-sig'):
        parser.error('This is the modular UI. Use --output with an isolated file to inspect a historical template.')
    return destination
