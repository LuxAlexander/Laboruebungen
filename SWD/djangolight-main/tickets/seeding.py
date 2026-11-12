import json
from decimal import Decimal
from pathlib import Path
from typing import Any

from django.contrib.auth import get_user_model
from django.db import transaction

from core.enums import MatchStatus, TicketStatus, TicketType
from stadiums.models import Competition, Seat
from stadiums.statistics import StatisticsService
from tickets.models import PriceRule, Ticket
from tickets.services import generate_ticket_number

"""
Prices and season tickets. Season ticket SALE is use case 4.3 (teams of three only), so our season tickets
come from here: a few registered holders with fixed seats. Run after seed_stadium. Idempotent.
"""

DEFAULT_FILE = Path(__file__).parent / "data" / "tickets.json"


def read_config(path: Path | str = DEFAULT_FILE) -> dict[str, Any]:
    return json.loads(Path(path).read_text(encoding="utf-8"), parse_float=Decimal)


@transaction.atomic
def seed_tickets(config: dict[str, Any]) -> list[Ticket]:
    for competition_type, prices in config["prices"].items():
        for category, price in prices.items():
            PriceRule.objects.update_or_create(category=category, competition_type=competition_type,
                                               defaults={"price": Decimal(price)})

    season = config.get("season_tickets")
    if not season:
        return []
    competition = Competition.objects.get(name=season["competition"], season=season["season"])
    seats = list(Seat.objects.select_related("sector")
                 .filter(sector__name=season["sector"], row=season["row"]).order_by("number"))
    Customer = get_user_model()
    per_holder: int = season["seats_per_holder"]
    created: list[Ticket] = []

    for i, username in enumerate(season["holders"]):
        customer, new = Customer.objects.get_or_create(username=username)
        if new:
            customer.set_password(season["password"])
            customer.save()
        for seat in seats[i * per_holder:(i + 1) * per_holder]:
            if Ticket.objects.filter(seat=seat, competition=competition, type=TicketType.SEASON).exists():
                continue
            created.append(Ticket.objects.create(
                number=generate_ticket_number(), type=TicketType.SEASON, status=TicketStatus.SOLD, seat=seat,
                competition=competition, customer=customer,
                price=Decimal(config["season_ticket_prices"][seat.sector.category])))

    # season tickets are admission-entitled for every open match of the competition
    if created:
        statistics = StatisticsService()
        for match in competition.matches.filter(status__in=[MatchStatus.PLANNED, MatchStatus.ACTIVE]):
            statistics.record_sale(match.pk, len(created))
    return created
