from pathlib import Path

from django.core.management.base import BaseCommand, CommandError

from products.seeding import DEFAULT_CATALOG, CatalogFileError, import_catalog, read_catalog


class Command(BaseCommand):
    help = "Load the initial product catalog from a JSON file (idempotent, items are matched by name)."

    def add_arguments(self, parser):
        parser.add_argument("--file", type=Path, default=DEFAULT_CATALOG,
                            help=f"JSON file with a list of items (default: {DEFAULT_CATALOG})")

    def handle(self, *args, **options):
        try:
            items = read_catalog(options["file"])
        except CatalogFileError as e:
            raise CommandError(str(e))

        created, updated = import_catalog(items)
        self.stdout.write(self.style.SUCCESS(f"catalog seeded: {created} created, {updated} updated"))
