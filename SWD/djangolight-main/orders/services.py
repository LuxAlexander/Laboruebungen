from typing import Optional

from django.db import transaction

from core.domain import OrderSummary, CustomerInfo, EmptyBasket, CatalogItem, OrderLine
from core.interfaces import Customers, ProductCatalog
from core.models import Customer
from orders.basket import Basket
from orders.logging import logger
from orders.models import Order, OrderPosition

"""
Please note, that we are only importing the interfaces from the core module and not the concrete classes.
So this module can be used with any concrete product or customer module implementation.
"""


class OrderService:
    """
    Order related business logic: pricing a basket for display and turning it into a persisted order.
    """

    def __init__(self, product_service: ProductCatalog, customer_service: Customers):
        logger.debug(f"OrderService.__init__(product_service: {product_service}, customer_service: {customer_service}")
        self.product_service = product_service
        self.customer_service = customer_service

    def get_order_summary(self, customer: str, basket: Basket) -> OrderSummary:
        logger.debug(f"OrderService.get_order_summary(customer: {customer}, basket: {len(basket)} items)")
        customer_info: Optional[CustomerInfo] = self.customer_service.find_customer(username=customer)
        return basket.lines(self.product_service, customer_info)

    def get_orders(self, customer: Customer) -> list[OrderSummary]:
        """The customer's placed orders, newest first, as read-only summaries."""
        logger.debug(f"OrderService.get_orders(customer: {customer})")
        customer_info: Optional[CustomerInfo] = self.customer_service.find_customer(username=customer.username)
        orders = (Order.objects.filter(user=customer)
                  .order_by("-created_at", "-pk")
                  .prefetch_related("order_positions__product"))
        return [self._to_summary(order, customer_info) for order in orders]

    @staticmethod
    def _to_summary(order: Order, customer_info: Optional[CustomerInfo]) -> OrderSummary:
        lines = [
            OrderLine(pos=position.pos,
                      product=CatalogItem(id=position.product_id, name=position.product.name,
                                          description=position.product.description, price=position.price),
                      quantity=position.quantity, price=position.price)
            for position in sorted(order.order_positions.all(), key=lambda p: p.pos)
        ]
        return OrderSummary(order_number=order.pk, customer=customer_info, lines=lines,
                            total_price=order.total_price, created_at=order.created_at)

    def create_order(self, customer: Customer, basket: Basket) -> Order:
        logger.debug(f"OrderService.create_order(customer: {customer}, basket: {len(basket)} items)")

        priced: OrderSummary = basket.lines(self.product_service)
        if not priced.lines:
            raise EmptyBasket()

        # one transaction: credit is redeemed first, so on InsufficientCredit nothing has been written
        with transaction.atomic():
            self.customer_service.redeem_credit(customer, priced.total_price)
            order: Order = Order.objects.create(user=customer, total_price=priced.total_price)
            OrderPosition.objects.bulk_create(
                OrderPosition(order=order, pos=position.pos, product_id=position.product.id,
                              quantity=position.quantity, price=position.price)
                for position in priced.lines
            )
            return order
