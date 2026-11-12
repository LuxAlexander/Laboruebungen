from dependency_injector.wiring import Provide, inject
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from django.http import Http404, HttpRequest, HttpResponse
from django.shortcuts import redirect, render
from django.urls import reverse
from django.views.decorators.http import require_POST

from core.domain import TicketingError
from core.enums import PaymentMethod, SalesChannel
from core.interfaces import MatchCatalog, Tickets

from .basket import MAX_TICKETS_PER_PERSON, Basket
from .logging import logger
from .services import MAX_ANONYMOUS_ONLINE, OrderService

SESSION_ORDERS = "my_order_ids"  # orders placed in this session (lets anonymous buyers see their confirmation)


def _channel(request: HttpRequest) -> SalesChannel:
    """Staff users act as the staffed ticket counter (anonymous sale, cash allowed); everyone else buys online."""
    return SalesChannel.COUNTER if request.user.is_staff else SalesChannel.ONLINE


def _customer_id(request: HttpRequest) -> int | None:
    if request.user.is_authenticated and not request.user.is_staff:
        return request.user.pk
    return None


def _int(value) -> int | None:
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


@inject
def match_list(request: HttpRequest, catalog: MatchCatalog = Provide["match_catalog"]) -> HttpResponse:
    """Step 1: the upcoming matches."""
    return render(request, "orders/match_list.html", {"matches": catalog.upcoming_matches()})


@inject
def match_detail(request: HttpRequest, match_id: int, catalog: MatchCatalog = Provide["match_catalog"],
                 tickets: Tickets = Provide["ticket_service"]) -> HttpResponse:
    """Steps 2-3: choose a sector, a row and an available seat; prices are shown per sector."""
    match = catalog.get_match(match_id)
    if match is None:
        raise Http404("Match does not exist")

    try:
        taken_per_sector = tickets.taken_count_by_sector(match)
        sectors = [{"sector": s, "free": s.capacity - taken_per_sector.get(s.id, 0), "price": tickets.price(s, match)}
                   for s in catalog.sectors(match.stadium_id)]
    except TicketingError as e:  # no price defined for this competition
        messages.error(request, str(e))
        return redirect("orders:match_list")

    sector = catalog.get_sector(_int(request.GET.get("sector"))) if request.GET.get("sector") else None
    rows, seats = [], []
    row = _int(request.GET.get("row"))
    if sector and sector.stadium_id == match.stadium_id:
        rows = catalog.rows(sector.id)
        row = row if row in rows else (rows[0] if rows else None)
        taken = tickets.taken_seat_ids(match, sector.id)
        seats = [{"seat": seat, "free": seat.id not in taken} for seat in catalog.seats(sector.id, row)]

    basket = Basket.load(request.session)
    context = {
        "match": match,
        "sectors": sectors,
        "sector": sector,
        "rows": rows,
        "row": row,
        "seats": seats,
        "selected_ids": basket.seat_ids if basket.match_id == match.id else [],
    }
    return render(request, "orders/match_detail.html", context)


@require_POST
@inject
def add_seat(request: HttpRequest, catalog: MatchCatalog = Provide["match_catalog"],
             tickets: Tickets = Provide["ticket_service"]) -> HttpResponse:
    match = catalog.get_match(_int(request.POST.get("match_id")))
    seat_id = _int(request.POST.get("seat_id"))
    seats = catalog.get_seats([seat_id]) if seat_id else []
    if match is None or not seats:
        raise Http404("Match or seat does not exist")

    if seat_id in tickets.taken_seat_ids(match, seats[0].sector_id):
        messages.error(request, f"Seat {seats[0].label} is not available any more.")
    else:
        try:
            Basket.load(request.session).add(match.id, seat_id)
        except TicketingError as e:
            messages.error(request, str(e))
    return redirect(request.META.get("HTTP_REFERER") or reverse("orders:match_detail", args=[match.id]))


@require_POST
def remove_seat(request: HttpRequest) -> HttpResponse:
    Basket.load(request.session).remove(_int(request.POST.get("seat_id")))
    return redirect("orders:basket_overview")


@require_POST
def clear_basket(request: HttpRequest) -> HttpResponse:
    Basket.load(request.session).clear()
    return redirect("orders:basket_overview")


@inject
def basket_overview(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    """Step 4: the selected seats with prices and the payment form."""
    summary = order_service.get_order_summary(Basket.load(request.session))
    channel = _channel(request)
    context = {
        "order": summary,
        "channel": channel,
        "cash_allowed": channel == SalesChannel.COUNTER,
        "needs_login": (channel == SalesChannel.ONLINE and not request.user.is_authenticated
                        and len(summary.lines) > MAX_ANONYMOUS_ONLINE),
        "max_tickets": MAX_TICKETS_PER_PERSON,
    }
    return render(request, "orders/basket_overview.html", context)


@require_POST
@inject
def place_order(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    """Step 5: pay and buy. Business failures are shown as messages, the basket stays as it is."""
    basket = Basket.load(request.session)
    try:
        method = PaymentMethod(request.POST.get("payment_method", PaymentMethod.CREDIT_CARD))
    except ValueError:
        method = PaymentMethod.CREDIT_CARD
    try:
        summary = order_service.purchase(basket, _customer_id(request), _channel(request), method,
                                         request.POST.get("card_number", ""))
    except TicketingError as e:
        logger.info(f"purchase rejected: {e}")
        messages.error(request, str(e))
        return redirect("orders:basket_overview")

    basket.clear()
    request.session[SESSION_ORDERS] = request.session.get(SESSION_ORDERS, []) + [summary.order_number]
    messages.success(request, f"Thank you! Order #{summary.order_number} is complete.")
    return redirect("orders:confirmation", order_id=summary.order_number)


@inject
def confirmation(request: HttpRequest, order_id: int,
                 order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    """The ticket numbers of an order: for its customer, staff, or the session that placed it."""
    allowed = (order_id in request.session.get(SESSION_ORDERS, []) or request.user.is_staff
               or order_service.order_belongs_to(order_id, _customer_id(request)))
    order = order_service.get_order(order_id) if allowed else None
    if order is None:
        raise Http404("Order not found")
    return render(request, "orders/confirmation.html", {"order": order})


@login_required
@inject
def order_list(request: HttpRequest, order_service: OrderService = Provide["order_service"],
               tickets: Tickets = Provide["ticket_service"]) -> HttpResponse:
    """The logged-in customer's orders and season tickets."""
    context = {
        "orders": order_service.get_orders(request.user.pk),
        "season_tickets": [t for t in tickets.tickets_for_customer(request.user.pk) if t.competition_id],
    }
    return render(request, "orders/order_list.html", context)
