from datetime import date
from decimal import Decimal
from typing import Optional, Protocol, runtime_checkable

from core.domain import (CustomerInfo, EntryResult, GateInfo, MatchInfo, MatchStatisticsInfo, PaymentRequest, SeatInfo,
                         SectorInfo, TicketInfo, UtilisationReport)

"""
The interfaces at the seams between the apps. They are Protocols (PEP 544): an adapter satisfies an interface by
having the right methods, no inheritance needed. Callers depend only on these and on the records in core.domain,
never on another app's models.

Methods marked "inside the caller's transaction" lock rows with select_for_update() and must be called within
transaction.atomic().
"""


@runtime_checkable
class MatchCatalog(Protocol):
    """Master data: stadiums, sectors, seats, matches (stadiums app)."""

    def upcoming_matches(self) -> list[MatchInfo]: ...

    def get_match(self, match_id: int) -> Optional[MatchInfo]: ...

    def active_match(self, gate_id: int) -> Optional[MatchInfo]: ...

    def gates(self) -> list[GateInfo]: ...

    def sectors(self, stadium_id: int) -> list[SectorInfo]: ...

    def get_sector(self, sector_id: int) -> Optional[SectorInfo]: ...

    def rows(self, sector_id: int) -> list[int]: ...

    def seats(self, sector_id: int, row: Optional[int] = None) -> list[SeatInfo]: ...

    def get_seats(self, seat_ids: list[int]) -> list[SeatInfo]: ...

    def lock_seats(self, seat_ids: list[int]) -> list[SeatInfo]:
        """Inside the caller's transaction: locks the seat rows (in id order) so no one else can sell them."""


@runtime_checkable
class Tickets(Protocol):
    """Ticket data and pricing (tickets app)."""

    def taken_seat_ids(self, match: MatchInfo, sector_id: Optional[int] = None) -> set[int]:
        """Seats with a sold/reserved single ticket for the match or a season ticket for its competition."""

    def taken_count_by_sector(self, match: MatchInfo) -> dict[int, int]: ...

    def price(self, sector: SectorInfo, match: MatchInfo) -> Decimal:
        """Raises MatchNotOnSale when no price is defined for the sector category and competition type."""

    def count_for_customer(self, customer_id: int, match: MatchInfo) -> int: ...

    def issue_single_tickets(self, match: MatchInfo, seats: list[SeatInfo],
                             customer_id: Optional[int]) -> list[TicketInfo]:
        """Inside the caller's transaction (seats already locked)."""

    def lock_ticket(self, number: str) -> Optional[TicketInfo]:
        """Inside the caller's transaction: loads and locks the ticket row."""

    def find_ticket(self, number: str) -> Optional[TicketInfo]: ...

    def tickets_for_customer(self, customer_id: int) -> list[TicketInfo]: ...


@runtime_checkable
class Customers(Protocol):
    def find_customer(self, username: str) -> Optional[CustomerInfo]: ...


@runtime_checkable
class Payments(Protocol):
    def pay(self, request: PaymentRequest) -> str:
        """Returns a payment reference or raises PaymentFailed."""


@runtime_checkable
class Statistics(Protocol):
    def record_sale(self, match_id: int, count: int) -> None: ...

    def record_entry(self, match_id: int) -> None:
        """Counts the first entry of a ticket for a match (not re-entries)."""

    def conduct_match(self, match_id: int) -> None:
        """Sets the match to conducted and adds its statistics to the competition statistics."""

    def match_statistics(self, match_id: int) -> Optional[MatchStatisticsInfo]: ...

    def utilisation(self, label: str, start: Optional[date] = None, end: Optional[date] = None,
                    competition_id: Optional[int] = None) -> UtilisationReport: ...


@runtime_checkable
class SecurityAlerts(Protocol):
    def notify_double_entry(self, ticket_number: str, match_id: int, gate_id: int, attempt_count: int) -> None: ...
