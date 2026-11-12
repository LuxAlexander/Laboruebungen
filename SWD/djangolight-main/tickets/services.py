import secrets
from decimal import Decimal
from typing import Optional

from django.db.models import Count

from core.domain import (TICKET_NUMBER_PREFIX, MatchInfo, MatchNotOnSale, SeatInfo, SeatNotAvailable, SectorInfo,
                         TicketInfo, normalize_ticket_number)
from core.enums import TicketStatus, TicketType
from core.interfaces import MatchCatalog
from tickets.models import PriceRule, Ticket

NUMBER_PREFIX = TICKET_NUMBER_PREFIX
NUMBER_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"  # no 0/O, 1/I: easy to type at the gate
NUMBER_LENGTH = 6


def generate_ticket_number() -> str:
    """Random, hard to guess ticket number such as TCK-7F3K9Q (cryptographically random)."""
    while True:
        number = NUMBER_PREFIX + "".join(secrets.choice(NUMBER_ALPHABET) for _ in range(NUMBER_LENGTH))
        if not Ticket.objects.filter(number=number).exists():
            return number


class TicketService:
    """Adapter for core.interfaces.Tickets, backed by the tickets models."""

    def __init__(self, catalog: MatchCatalog):
        self.catalog = catalog

    def taken_seat_ids(self, match: MatchInfo, sector_id: Optional[int] = None) -> set[int]:
        tickets = Ticket.objects.occupying(match.id, match.competition_id)
        if sector_id is not None:
            tickets = tickets.filter(seat__sector_id=sector_id)
        return set(tickets.values_list("seat_id", flat=True))

    def taken_count_by_sector(self, match: MatchInfo) -> dict[int, int]:
        rows = (Ticket.objects.occupying(match.id, match.competition_id)
                .values("seat__sector_id").annotate(n=Count("id")))
        return {row["seat__sector_id"]: row["n"] for row in rows}

    def price(self, sector: SectorInfo, match: MatchInfo) -> Decimal:
        rule = PriceRule.objects.filter(category=sector.category, competition_type=match.competition_type).first()
        if rule is None:
            raise MatchNotOnSale()
        return rule.price

    def count_for_customer(self, customer_id: int, match: MatchInfo) -> int:
        return Ticket.objects.filter(customer_id=customer_id, match_id=match.id, type=TicketType.SINGLE,
                                     status__in=[TicketStatus.SOLD, TicketStatus.RESERVED]).count()

    def issue_single_tickets(self, match: MatchInfo, seats: list[SeatInfo],
                             customer_id: Optional[int]) -> list[TicketInfo]:
        """Call inside the caller's transaction, after the seats are locked (MatchCatalog.lock_seats)."""
        taken = self.taken_seat_ids(match) & {seat.id for seat in seats}
        if taken:
            raise SeatNotAvailable([seat.label for seat in seats if seat.id in taken])

        issued = []
        for seat in seats:
            sector = self.catalog.get_sector(seat.sector_id)
            ticket = Ticket.objects.create(number=generate_ticket_number(), type=TicketType.SINGLE,
                                           status=TicketStatus.SOLD, seat_id=seat.id, match_id=match.id,
                                           customer_id=customer_id, price=self.price(sector, match))
            issued.append(to_ticket_info(ticket, seat))
        return issued

    def lock_ticket(self, number: str) -> Optional[TicketInfo]:
        ticket = Ticket.objects.select_for_update().filter(number=normalize_ticket_number(number)).first()
        return to_ticket_info(ticket) if ticket else None

    def find_ticket(self, number: str) -> Optional[TicketInfo]:
        ticket = Ticket.objects.with_seat().filter(number=normalize_ticket_number(number)).first()
        return to_ticket_info(ticket) if ticket else None

    def tickets_for_customer(self, customer_id: int) -> list[TicketInfo]:
        return [to_ticket_info(t) for t in Ticket.objects.with_seat().filter(customer_id=customer_id)
                .order_by("-created_at")]


def to_ticket_info(ticket: Ticket, seat: Optional[SeatInfo] = None) -> TicketInfo:
    if seat is None:
        s = ticket.seat
        seat = SeatInfo(id=s.pk, sector_id=s.sector_id, sector_name=s.sector.name, row=s.row, number=s.number)
    return TicketInfo(id=ticket.pk, number=ticket.number, type=TicketType(ticket.type),
                      status=TicketStatus(ticket.status), seat=seat, price=ticket.price, match_id=ticket.match_id,
                      competition_id=ticket.competition_id, customer_id=ticket.customer_id)
