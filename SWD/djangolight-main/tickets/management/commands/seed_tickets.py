from django.core.management.base import BaseCommand

from tickets.seeding import DEFAULT_FILE, read_config, seed_tickets


class Command(BaseCommand):
    help = "Loads the price rules and the demo season tickets. Run after seed_stadium. Idempotent."

    def add_arguments(self, parser):
        parser.add_argument("--file", default=str(DEFAULT_FILE), help="JSON file with prices and season tickets")

    def handle(self, *args, **options):
        created = seed_tickets(read_config(options["file"]))
        self.stdout.write(self.style.SUCCESS(f"Price rules loaded, {len(created)} new season ticket(s)."))
        for ticket in created:
            self.stdout.write(f"  {ticket.number}  {ticket.customer}  seat {ticket.seat}")
