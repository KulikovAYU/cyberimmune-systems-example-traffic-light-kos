from typing import Tuple, List, Any
from dataclasses import dataclass, field


@dataclass
class Command:
    contents: Any
    type: str


@dataclass
class Mode:
    r: bool = False
    y: bool = False
    g: bool = False
    blink: bool = False


@dataclass
class Trafficlight:
    is_maintenance: bool = False
    current_mode: Tuple[Mode, Mode] = field(default_factory=tuple)
    commandQueue: List[Command] = field(default_factory=list)
    errors: List = field(default_factory=list)


def M(mode: str) -> Mode:
    mode = mode.lower()
    return Mode(
        'r' in mode,
        'y' in mode,
        'g' in mode,
        'b' in mode
    )
