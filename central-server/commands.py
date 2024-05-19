from dataclasses import dataclass, field
from typing import Tuple, List, Any

from models import Mode


@dataclass
class MaintenanceCommand:
    contents: bool
    type: str = "maintenance"


@dataclass
class SetModeCommand:
    contents: Tuple[Mode, Mode]
    type: str = "set"
