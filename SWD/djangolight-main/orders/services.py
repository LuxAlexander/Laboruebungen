from decimal import Decimal
from typing import Optional

from django.db import transaction

from core.domain import (BasketLine, EmptyBasket, MatchNotOnSale, OrderSummary, PaymentRequest, SeatInfo,
                         TicketLimitExceeded)
from core.enums import MatchStatus, PaymentMethod, SalesChannel
from core.interfaces import Customers, MatchCatalog, Payments, Statistics, Tickets
from orders.basket import MAX_TICKETS_PER_PERSON, Basket
from orders.logging import logger
from orders.models import Order, OrderPosition

MAX_ANONYMOUS_ONLINE = 1  # anonymous online buyers may buy one ticket; more requires an account


class OrderService:
    """
    Use case 4.1 Single-Match Ticket Sale. Only interfaces of the other apps are used.
    """

    def __init__(self, catalog: MatchCatalog, tickets: Tickets, customers: Customers, payments: Payments,
                 statistics: Statistics):
        self.catalog = catalog
        self.tickets = tickets
        self.customers = customers
        self.payments = payments
        self.statistics = statistics

    def get_order_summary(self, basket: Basket) -> OrderSummary:
        return basket.lines(self.catalog, self.tickets)

    def purchase(self, basket: Basket, customer_id: Optional[int], channel: SalesChannel,
                 payment_method: PaymentMethod, card_number: str = "") -> OrderSummary:
        """
        Sells the selected seats. Everything happens in one transaction: lock seats, re-check availability,
        issue tickets, take payment, write the order, update statistics. Any failure rolls all of it back.
        """
        logger.debug(f"purchase(match={basket.match_id}, seats={basket.seat_ids}, customer={customer_id}, "
                     f"channel={channel}, payment={payment_method})")
        if not basket.seat_ids:
            raise EmptyBasket()
        match = self.catalog.get_match(basket.match_id)
        if match is None or match.status not in (MatchStatus.PLANNED, MatchStatus.ACTIVE):
            raise MatchNotOnSale()
        if len(basket) > MAX_TICKETS_PER_PERSON:
            raise TicketLimitExceeded(MAX_TICKETS_PER_PERSON)
        if channel == SalesChannel.ONLINE and customer_id is None and len(basket) > MAX_ANONYMOUS_ONLINE:
            raise TicketLimitExceeded(MAX_ANONYMOUS_ONLINE, "for anonymous online purchases, please log in")

        with transaction.atomic():
            seats: list[SeatInfo] = self.catalog.lock_seats(basket.seat_ids)
            if customer_id is not None:
                already = self.tickets.count_for_customer(customer_id, match)
                if already + len(seats) > MAX_TICKETS_PER_PERSON:
                    raise TicketLimitExceeded(MAX_TICKETS_PER_PERSON, f"per person (you already hold {already})")

            issued = self.tickets.issue_single_tickets(match, seats, customer_id)  # raises SeatNotAvailable
            total = sum((t.price for t in issued), Decimal("0"))
            reference = self.payments.pay(PaymentRequest(method=payment_method, channel=channel, amount=total,
                                                         card_number=card_number))  # raises PaymentFailed

            order = Order.objects.create(customer_id=customer_id, match_id=match.id, channel=channel,
                                         payment_method=payment_method, payment_reference=reference,
                                         total_price=total)
            OrderPosition.objects.bulk_create(
                OrderPosition(order=order, pos=i, ticket_id=t.id, price=t.price) for i, t in enumerate(issued, 1))
            self.statistics.record_sale(match.id, len(issued))

        logger.info(f"order #{order.pk}: {len(issued)} ticket(s) for match {match.id}, total {total}")
        return OrderSummary(match=match, total_price=total, order_number=order.pk, created_at=order.created_at,
                            channel=channel, payment_method=payment_method,
                            lines=[BasketLine(pos=i, seat=t.seat, price=t.price, ticket_number=t.number)
                                   for i, t in enumerate(issued, 1)],
                            ticket_numbers=[t.number for t in issued])

    def get_order(self, order_id: int) -> Optional[OrderSummary]:
        order = Order.objects.with_details().filter(pk=order_id).first()
        return self._to_summary(order) if order else None

    def get_orders(self, customer_id: int) -> list[OrderSummary]:
        orders = Order.objects.with_details().filter(customer_id=customer_id).order_by("-created_at", "-pk")
        return [self._to_summary(order) for order in orders]

    def order_belongs_to(self, order_id: int, customer_id: Optional[int]) -> bool:
        return customer_id is not None and Order.objects.filter(pk=order_id, customer_id=customer_id).exists()

    def _to_summary(self, order: Order) -> OrderSummary:
        positions = sorted(order.positions.all(), key=lambda p: p.pos)
        lines = []
        for p in positions:
            seat = p.ticket.seat
            lines.append(BasketLine(pos=p.pos, price=p.price, ticket_number=p.ticket.number,
                                    seat=SeatInfo(id=seat.pk, sector_id=seat.sector_id, sector_name=seat.sector.name,
                                                  row=seat.row, number=seat.number)))
        return OrderSummary(match=self.catalog.get_match(order.match_id), lines=lines, total_price=order.total_price,
                            order_number=order.pk, created_at=order.created_at,
                            channel=SalesChannel(order.channel), payment_method=PaymentMethod(order.payment_method),
                            ticket_numbers=[p.ticket.number for p in positions])
