from dependency_injector.wiring import Provide, inject
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from django.http import Http404, HttpRequest, HttpResponse, HttpResponseBadRequest
from django.shortcuts import redirect
from django.template import loader
from django.views.decorators.http import require_POST

from core.domain import OrderSummary, CatalogItem, OrderError
from core.interfaces import ProductCatalog

from .basket import Basket
from .logging import logger
from .services import OrderService


@require_POST
@inject
def add_to_basket(
    request: HttpRequest,
    product_service: ProductCatalog = Provide["product_service"],
) -> HttpResponse:
    product_id = request.POST.get("product_id")
    if product_id is None:
        return HttpResponseBadRequest("Missing 'product_id' parameter")

    # check if product exists (get_object_or_404 is not possible, core.models.Product is abstract)
    product: CatalogItem | None = product_service.get_product(product_id)
    if product is None:
        raise Http404("Product does not exist")

    Basket.load(request.session).add(product_id)
    """
    The request.META.get('HTTP_REFERER') value represents the URL of the previous page the user visited.
    By passing it as the argument to redirect(), you can redirect the user back to that page.
    Note that request.META.get('HTTP_REFERER') might be None if the browser or client doesn't
    provide the referrer information.
    """
    return redirect(request.META.get("HTTP_REFERER") or "products:index")


@inject
def basket_overview(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    user = ""

    if request.user.is_authenticated:
        user = request.user.username

    order_summary: OrderSummary = order_service.get_order_summary(user, Basket.load(request.session))

    # The basket is priced into an OrderSummary (see core.domain) instead of an Order model:
    # nothing may be written to the database before the customer places the order, and an
    # unsaved model instance is awkward to work with (no id, no foreign keys). The session
    # itself only holds product ids, because session data must be JSON serializable.
    context = {
        "order": order_summary,
    }
    template = loader.get_template("orders/basket_overview.html")
    return HttpResponse(template.render(context, request))


@login_required
@require_POST
@inject
def place_order(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    try:
        basket = Basket.load(request.session)
        order = order_service.create_order(request.user, basket)
        basket.clear()
        messages.success(request, f"Thank you! Your order #{order.pk} has been placed.")
        return redirect("products:index")

    except OrderError as e:
        # expected business outcomes are shown to the customer; anything else is a bug and surfaces as 500
        logger.info(f"order rejected: {e}")
        messages.error(request, str(e))

    return redirect("orders:basket_overview")


@login_required
@inject
def order_list(request: HttpRequest, order_service: OrderService = Provide["order_service"]) -> HttpResponse:
    """read-only: the orders the logged-in customer has placed"""
    context = {
        "orders": order_service.get_orders(request.user),
    }
    template = loader.get_template("orders/order_list.html")
    return HttpResponse(template.render(context, request))


# changes state, so POST only: a GET link could be triggered by any page (e.g. an <img src=...>)
@require_POST
def clear_basket(request: HttpRequest) -> HttpResponse:
    """
    the basket is cleared by removing entries from the session
    """
    Basket.load(request.session).clear()

    return redirect("orders:basket_overview")
