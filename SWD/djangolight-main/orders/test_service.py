from decimal import Decimal
from unittest.mock import Mock

from django.test import TestCase

from core.domain import EmptyBasket, InsufficientCredit
from customers.models import Customer
from orders.basket import Basket
from orders.models import Order, OrderPosition
from orders.services import OrderService
from products.models import Product
from products.services import ProductService

"""
The OrderService is tested through its interface against the SQLite test database.
Only the customer interface is mocked, as credit handling has its own tests in the customers app.
"""


class TestOrderService(TestCase):
    def setUp(self):
        self.apple = Product.objects.create(name="apple", description="", price=Decimal("1.5"))
        self.pear = Product.objects.create(name="pear", description="", price=Decimal("2.0"))
        self.customer = Customer.objects.create(username="alice", credit=100)

        self.customer_service = Mock()
        self.order_service = OrderService(ProductService(), self.customer_service)
        self.basket = Basket([self.apple.id, self.pear.id, self.apple.id])

    def test_create_order_customer_has_credit(self):
        order = self.order_service.create_order(self.customer, self.basket)

        self.assertEqual(Order.objects.get(), order)
        self.assertEqual(order.user, self.customer)
        self.assertEqual(order.total_price, Decimal("5.0"))
        self.customer_service.redeem_credit.assert_called_once_with(self.customer, Decimal("5.0"))

    def test_repeated_product_is_persisted_with_its_quantity(self):
        order = self.order_service.create_order(self.customer, self.basket)

        positions = list(OrderPosition.objects.filter(order=order).order_by("pos")
                         .values_list("pos", "product_id", "quantity", "price"))
        self.assertEqual(positions, [(1, self.apple.id, 2, Decimal("1.5")), (2, self.pear.id, 1, Decimal("2.0"))])

    def test_create_order_customer_without_credit(self):
        self.customer_service.redeem_credit.side_effect = InsufficientCredit(required=Decimal("5.0"), available=Decimal("1.0"))

        with self.assertRaises(InsufficientCredit):
            self.order_service.create_order(self.customer, self.basket)

        self.customer_service.redeem_credit.assert_called_once()
        # credit is redeemed before anything is written, so nothing is persisted
        self.assertEqual(Order.objects.count(), 0)
        self.assertEqual(OrderPosition.objects.count(), 0)

    def test_empty_basket_is_rejected_before_touching_credit(self):
        with self.assertRaises(EmptyBasket):
            self.order_service.create_order(self.customer, Basket())

        self.customer_service.redeem_credit.assert_not_called()
        self.assertEqual(Order.objects.count(), 0)

    def test_get_orders_lists_own_orders_newest_first(self):
        other = Customer.objects.create(username="bob", credit=100)
        first = self.order_service.create_order(self.customer, self.basket)
        second = self.order_service.create_order(self.customer, Basket([self.pear.id]))
        self.order_service.create_order(other, Basket([self.apple.id]))
        self.customer_service.find_customer.return_value = "customer-info"

        summaries = self.order_service.get_orders(self.customer)

        self.assertEqual([s.order_number for s in summaries], [second.pk, first.pk])
        self.assertEqual([s.total_price for s in summaries], [Decimal("2.0"), Decimal("5.0")])
        self.assertEqual(summaries[1].customer, "customer-info")
        self.assertEqual(summaries[1].created_at, first.created_at)
        self.assertEqual([(l.pos, l.product.name, l.quantity, l.price) for l in summaries[1].lines],
                         [(1, "apple", 2, Decimal("1.5")), (2, "pear", 1, Decimal("2.0"))])

    def test_get_orders_is_empty_for_new_customer(self):
        self.assertEqual(self.order_service.get_orders(self.customer), [])

    def test_get_order_summary_attaches_account(self):
        self.customer_service.find_customer.return_value = "customer-info"

        order = self.order_service.get_order_summary("alice", self.basket)

        self.assertEqual(order.customer, "customer-info")
        self.assertEqual(order.total_price, Decimal("5.0"))
        self.customer_service.find_customer.assert_called_once_with(username="alice")
