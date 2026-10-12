from dependency_injector.wiring import Provide, inject
from django.http import HttpRequest, HttpResponse
from django.template import loader

from core.interfaces import ProductCatalog


@inject
def index(
    request: HttpRequest,
    # Provide is a special object used in dependency injection frameworks to specify that the required object
    # should be provided by the framework itself, rather than being instantiated by the code.
    product_service: ProductCatalog = Provide["product_service"],
) -> HttpResponse:
    """the product catalog: all available offerings (basket_count comes from orders.context_processors.basket)"""
    context = {
        "products_list": product_service.get_all_products(),
    }
    template = loader.get_template("products/index.html")
    return HttpResponse(template.render(context, request))
