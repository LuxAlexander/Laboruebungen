from decimal import Decimal
from typing import MutableMapping, Optional

from core.domain import BasketLine, MatchInfo, OrderSummary, TicketLimitExceeded
from core.interfaces import MatchCatalog, Tickets

MAX_TICKETS_PER_PERSON = 4


class Basket:
    """
    The seats a visitor has selected for ONE match, kept in the session until the order is placed.
    Selecting a seat of another match starts a new basket. At most 4 seats.
    Session data must be JSON serializable, so only ids are stored.
    """

    SESSION_KEY = "seat_basket"

    def __init__(self, match_id: Optional[int] = None, seat_ids: Optional[list[int]] = None,
                 session: Optional[MutableMapping] = None):
        self.match_id = match_id
        self.seat_ids: list[int] = list(seat_ids or [])
        self._session = session

    @classmethod
    def load(cls, session: MutableMapping) -> "Basket":
        data = session.get(cls.SESSION_KEY) or {}
        return cls(data.get("match_id"), data.get("seat_ids"), session)

    def __len__(self) -> int:
        return len(self.seat_ids)

    def add(self, match_id: int, seat_id: int) -> None:
        if match_id != self.match_id:
            self.match_id, self.seat_ids = match_id, []
        if seat_id in self.seat_ids:
            return
        if len(self.seat_ids) >= MAX_TICKETS_PER_PERSON:
            raise TicketLimitExceeded(MAX_TICKETS_PER_PERSON)
        self.seat_ids.append(seat_id)
        self._save()

    def remove(self, seat_id: int) -> None:
        if seat_id in self.seat_ids:
            self.seat_ids.remove(seat_id)
            self._save()

    def clear(self) -> None:
        self.match_id, self.seat_ids = None, []
        if self._session is not None:
            self._session.pop(self.SESSION_KEY, None)

    def lines(self, catalog: MatchCatalog, tickets: Tickets) -> OrderSummary:
        """Price the selection. Unknown seats are skipped."""
        match: Optional[MatchInfo] = catalog.get_match(self.match_id) if self.match_id else None
        if match is None:
            return OrderSummary(match=None, lines=[], total_price=Decimal("0"))
        lines: list[BasketLine] = []
        sectors = {}
        for seat in catalog.get_seats(self.seat_ids):
            if seat.sector_id not in sectors:
                sectors[seat.sector_id] = catalog.get_sector(seat.sector_id)
            lines.append(BasketLine(pos=len(lines) + 1, seat=seat, price=tickets.price(sectors[seat.sector_id], match)))
        return OrderSummary(match=match, lines=lines, total_price=sum((l.price for l in lines), Decimal("0")))

    def _save(self) -> None:
        if self._session is not None:
            self._session[self.SESSION_KEY] = {"match_id": self.match_id, "seat_ids": list(self.seat_ids)}
