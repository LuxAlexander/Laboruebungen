import json
from decimal import Decimal
from io import StringIO
from pathlib import Path
from tempfile import TemporaryDirectory

from django.core.management import call_command, CommandError
from django.test import TestCase

from products.models import Product
from products.seeding import DEFAULT_CATALOG, CatalogFileError, import_catalog, read_catalog


class TestReadCatalog(TestCase):
    def test_default_catalog_has_five_priced_items(self):
        items = read_catalog(DEFAULT_CATALOG)
        self.assertEqual(len(items), 5)
        self.assertTrue(all(item["price"] > 0 for item in items))

    def test_prices_are_parsed_as_exact_decimals(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "catalog.json"
            path.write_text('[{"name": "cup", "description": "", "price": 7.90}]')
            self.assertEqual(read_catalog(path)[0]["price"], Decimal("7.90"))

    def test_missing_file(self):
        with self.assertRaises(CatalogFileError):
            read_catalog(Path("/nonexistent/catalog.json"))

    def test_missing_field_is_reported_with_position(self):
        with TemporaryDirectory() as tmp:
            path = Path(tmp) / "catalog.json"
            path.write_text(json.dumps([{"name": "ok", "description": "", "price": 1},
                                        {"name": "no price", "description": ""}]))
            with self.assertRaisesRegex(CatalogFileError, "item 2 is missing field\\(s\\): price"):
                read_catalog(path)


class TestImportCatalog(TestCase):
    def test_import_is_idempotent_and_updates_by_name(self):
        items = read_catalog(DEFAULT_CATALOG)

        self.assertEqual(import_catalog(items), (5, 0))
        self.assertEqual(Product.objects.count(), 5)

        items[0]["price"] = Decimal("99.0")
        self.assertEqual(import_catalog(items), (0, 5))
        self.assertEqual(Product.objects.count(), 5)
        self.assertEqual(Product.objects.get(name=items[0]["name"]).price, Decimal("99.0"))


class TestSeedCatalogCommand(TestCase):
    def test_command_seeds_default_catalog(self):
        out = StringIO()
        call_command("seed_catalog", stdout=out)
        self.assertIn("5 created, 0 updated", out.getvalue())
        self.assertEqual(Product.objects.count(), 5)

    def test_command_reports_bad_file(self):
        with self.assertRaisesRegex(CommandError, "catalog file not found"):
            call_command("seed_catalog", file="/nonexistent/catalog.json")
