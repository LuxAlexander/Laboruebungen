import json
from decimal import Decimal
from pathlib import Path
from typing import Any

from django.db import transaction

from products.models import Product

"""
Initial catalog data comes from a plain JSON list of items:
    [{"name": "...", "description": "...", "price": 1.0}, ...]
Items are matched by name, so running the import twice updates instead of duplicating.
Prices are parsed as Decimal straight from the JSON text, so 7.90 never passes through a float.
"""

DEFAULT_CATALOG = Path(__file__).parent / "data" / "catalog.json"
REQUIRED_FIELDS = {"name", "description", "price"}


class CatalogFileError(Exception):
    """The JSON file is missing, malformed, or an item lacks a required field."""


def read_catalog(path: Path | str = DEFAULT_CATALOG) -> list[dict[str, Any]]:
    path = Path(path)
    try:
        items = json.loads(path.read_text(encoding="utf-8"), parse_float=Decimal)
    except FileNotFoundError:
        raise CatalogFileError(f"catalog file not found: {path}")
    except json.JSONDecodeError as e:
        raise CatalogFileError(f"catalog file is not valid JSON: {path} ({e})")

    if not isinstance(items, list):
        raise CatalogFileError(f"catalog file must contain a JSON list of items: {path}")
    for index, item in enumerate(items, start=1):
        missing = REQUIRED_FIELDS - set(item) if isinstance(item, dict) else REQUIRED_FIELDS
        if missing:
            raise CatalogFileError(f"item {index} is missing field(s): {', '.join(sorted(missing))}")
    return items


@transaction.atomic
def import_catalog(items: list[dict[str, Any]]) -> tuple[int, int]:
    """Creates or updates products by name. Returns (created, updated)."""
    created = updated = 0
    for item in items:
        _, was_created = Product.objects.update_or_create(
            name=item["name"],
            defaults={"description": item["description"], "price": Decimal(item["price"])},
        )
        created += was_created
        updated += not was_created
    return created, updated
