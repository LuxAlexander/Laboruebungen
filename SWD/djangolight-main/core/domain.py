from dataclasses import dataclass, field
from datetime import date, datetime
from decimal import Decimal
from typing import Optional

from core.enums import (CompetitionType, DenyReason, MatchStatus, PaymentMethod, PriceCategory, SalesChannel,
                        TicketStatus, TicketType)

"""
The records that cross the seams between the apps.

Apps do not pass Django models to each other. A model is bound to the database; views, templates and other
apps only need to read data. So the adapters (services) copy what the caller needs into these frozen
dataclasses. frozen=True: a record cannot change under the caller's feet. slots=True: typos raise.
Money is Decimal (exact cents).
"""


# --- customers --------------------------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class CustomerInfo:
    id: int
    username: str
    email: str
    first_name: str
    last_name: str

    def __str__(self) -> str:
        return self.username


# --- master data (stadiums app) ---------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class SectorInfo:
    id: int
    name: str  # "A" .. "H"
    capacity: int
    category: PriceCategory
    stadium_id: int

    def __str__(self) -> str:
        return f"Sector {self.name}"


@dataclass(frozen=True, slots=True)
class SeatInfo:
    id: int
    sector_id: int
    sector_name: str
    row: int
    number: int

    @property
    def label(self) -> str:
        return f"{self.sector_name}-{self.row}-{self.number}"

    def __str__(self) -> str:
        return self.label


@dataclass(frozen=True, slots=True)
class GateInfo:
    id: int
    name: str
    stadium_id: int
    stadium_name: str

    def __str__(self) -> str:
        return f"{self.stadium_name} / {self.name}"


@dataclass(frozen=True, slots=True)
class MatchInfo:
    id: int
    home_team: str
    away_team: str
    kickoff: datetime
    status: MatchStatus
    stadium_id: int
    stadium_name: str
    competition_id: int
    competition_name: str
    competition_type: CompetitionType

    def __str__(self) -> str:
        return f"{self.home_team} vs {self.away_team}"


# --- tickets ----------------------------------------------------------------------------------------------------

TICKET_NUMBER_PREFIX = "TCK-"


def normalize_ticket_number(number: str) -> str:
    """Upper case, no surrounding blanks, prefix added if the gate user typed only the random part."""
    number = (number or "").strip().upper()
    if number and not number.startswith(TICKET_NUMBER_PREFIX):
        number = TICKET_NUMBER_PREFIX + number
    return number


@dataclass(frozen=True, slots=True)
class TicketInfo:
    id: int
    number: str  # random, e.g. "TCK-7F3K9Q"; presented at the gate
    type: TicketType
    status: TicketStatus
    seat: SeatInfo
    price: Decimal
    match_id: Optional[int]  # single-match ticket
    competition_id: Optional[int]  # season ticket
    customer_id: Optional[int]  # None for anonymous sales


# --- orders -----------------------------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class BasketLine:
    pos: int
    seat: SeatInfo
    price: Decimal
    ticket_number: str = ""  # set once the ticket is issued


@dataclass(frozen=True, slots=True)
class OrderSummary:
    """A priced seat selection: what the customer is about to buy, or a placed order."""
    match: Optional[MatchInfo]
    lines: list[BasketLine]
    total_price: Decimal
    order_number: int = 0  # 0 until the order is persisted
    created_at: Optional[datetime] = None
    channel: Optional[SalesChannel] = None
    payment_method: Optional[PaymentMethod] = None
    ticket_numbers: list[str] = field(default_factory=list)


@dataclass(frozen=True, slots=True)
class PaymentRequest:
    method: PaymentMethod
    channel: SalesChannel
    amount: Decimal
    card_number: str = ""


# --- access control ---------------------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class EntryResult:
    granted: bool
    message: str
    ticket_number: str
    reason: Optional[DenyReason] = None
    match_id: Optional[int] = None
    seat_label: str = ""


# --- statistics -------------------------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class MatchStatisticsInfo:
    match: MatchInfo
    capacity: int
    tickets_sold: int
    tickets_used: int

    @property
    def occupancy(self) -> Decimal:
        """Percentage of the stadium capacity that entered."""
        if not self.capacity:
            return Decimal("0")
        return (Decimal(self.tickets_used) * 100 / self.capacity).quantize(Decimal("0.01"))


@dataclass(frozen=True, slots=True)
class UtilisationReport:
    """Utilisation over a period (match, season, winter/spring season, Champions League ...)."""
    label: str
    start: Optional[date]
    end: Optional[date]
    matches: list[MatchStatisticsInfo]

    @property
    def tickets_sold(self) -> int:
        return sum(m.tickets_sold for m in self.matches)

    @property
    def tickets_used(self) -> int:
        return sum(m.tickets_used for m in self.matches)

    @property
    def capacity(self) -> int:
        return sum(m.capacity for m in self.matches)

    @property
    def occupancy(self) -> Decimal:
        if not self.capacity:
            return Decimal("0")
        return (Decimal(self.tickets_used) * 100 / self.capacity).quantize(Decimal("0.01"))


# --- errors -----------------------------------------------------------------------------------------------------

class TicketingError(Exception):
    """Base class for expected business failures. Views catch this one name and show the message."""


class OrderError(TicketingError):
    """A ticket purchase cannot be completed."""


class EmptyBasket(OrderError):
    def __init__(self) -> None:
        super().__init__("Please select at least one seat.")


class MatchNotOnSale(OrderError):
    def __init__(self) -> None:
        super().__init__("Tickets for this match are not on sale.")


class SeatNotAvailable(OrderError):
    def __init__(self, seat_labels: list[str]) -> None:
        self.seat_labels = seat_labels
        super().__init__(f"Seat(s) no longer available: {', '.join(seat_labels)}")


class TicketLimitExceeded(OrderError):
    def __init__(self, limit: int, reason: str = "per person") -> None:
        self.limit = limit
        super().__init__(f"At most {limit} ticket(s) {reason}.")


class PaymentFailed(OrderError):
    def __init__(self, reason: str) -> None:
        super().__init__(f"Payment failed: {reason}")
