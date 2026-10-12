from decimal import Decimal
from unittest import TestCase

from core.domain import CatalogItem
from core.interfaces import ProductCatalog
from orders.basket import Basket

"""
Pure unit tests: the Basket is tested through its interface with a plain dict as session
and an in-memory stand-in for the product interface. No database, no patching.
"""


class InMemoryProducts:
    def __init__(self, *products: CatalogItem):
        self._by_id = {str(product.id): product for product in products}
        self.lookups = 0

    def get_all_products(self) -> list[CatalogItem]:
        return list(self._by_id.values())

    def get_product(self, product_id) -> CatalogItem | None:
        self.lookups += 1
        return self._by_id.get(str(product_id))


APPLE = CatalogItem(id=1, name="apple", description="", price=Decimal("1.5"))
PEAR = CatalogItem(id=2, name="pear", description="", price=Decimal("2.0"))


class TestInMemoryProducts(TestCase):
    def test_satisfies_product_catalog(self):
        self.assertIsInstance(InMemoryProducts(), ProductCatalog)


class TestBasketSession(TestCase):
    def test_load_from_empty_session(self):
        session = {}
        basket = Basket.load(session)
        self.assertEqual(len(basket), 0)
        self.assertNotIn(Basket.SESSION_KEY, session)

    def test_add_writes_ids_as_strings(self):
        session = {}
        basket = Basket.load(session)
        basket.add(1)
        basket.add("2")
        self.assertEqual(session[Basket.SESSION_KEY], ["1", "2"])
        self.assertEqual(len(basket), 2)

    def test_clear_removes_session_key(self):
        session = {Basket.SESSION_KEY: ["1", "1"]}
        basket = Basket.load(session)
        basket.clear()
        self.assertEqual(len(basket), 0)
        self.assertNotIn(Basket.SESSION_KEY, session)

    def test_len_counts_repeats(self):
        self.assertEqual(len(Basket(["1", "1", "2"])), 3)


class TestBasketLines(TestCase):
    def test_total_is_exact_in_cents(self):
        # with floats, 0.1 + 0.2 sums to 0.30000000000000004
        dime = CatalogItem(id=1, name="dime", description="", price=Decimal("0.10"))
        nickel = CatalogItem(id=2, name="two nickels", description="", price=Decimal("0.20"))
        order = Basket(["1", "2"]).lines(InMemoryProducts(dime, nickel))
        self.assertEqual(order.total_price, Decimal("0.30"))

    def test_empty_basket_prices_to_zero(self):
        order = Basket().lines(InMemoryProducts(APPLE))
        self.assertEqual(order.lines, [])
        self.assertEqual(order.total_price, Decimal("0.0"))
        self.assertIsNone(order.customer)

    def test_repeated_product_becomes_one_position_with_quantity(self):
        products = InMemoryProducts(APPLE, PEAR)
        order = Basket(["1", "2", "1"]).lines(products)

        self.assertEqual([(p.pos, p.product.id, p.quantity, p.price) for p in order.lines],
                         [(1, 1, 2, Decimal("1.5")), (2, 2, 1, Decimal("2.0"))])
        self.assertEqual(order.total_price, Decimal("5.0"))
        self.assertEqual([line.total for line in order.lines], [Decimal("3.0"), Decimal("2.0")])
        # each distinct product is looked up exactly once
        self.assertEqual(products.lookups, 2)

    def test_unknown_products_are_skipped_and_positions_stay_contiguous(self):
        order = Basket(["99", "1", "42", "2"]).lines(InMemoryProducts(APPLE, PEAR))
        self.assertEqual([p.pos for p in order.lines], [1, 2])
        self.assertEqual([p.product.id for p in order.lines], [1, 2])
        self.assertEqual(order.total_price, Decimal("3.5"))

    def test_customer_is_attached(self):
        customer = object()
        order = Basket(["1"]).lines(InMemoryProducts(APPLE), customer)
        self.assertIs(order.customer, customer)
