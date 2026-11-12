from django.core.management.base import BaseCommand

from stadiums.seeding import DEFAULT_FILE, read_config, seed_stadium


class Command(BaseCommand):
    help = "Loads the stadium master data (sectors, seats, gates, teams, competitions, matches). Idempotent."

    def add_arguments(self, parser):
        parser.add_argument("--file", default=str(DEFAULT_FILE), help="JSON file with the master data")

    def handle(self, *args, **options):
        stadium = seed_stadium(read_config(options["file"]))
        self.stdout.write(self.style.SUCCESS(
            f"Seeded {stadium} with {stadium.capacity} seats in {stadium.sectors.count()} sectors, "
            f"{stadium.gates.count()} gates, {stadium.matches.count()} matches."))
