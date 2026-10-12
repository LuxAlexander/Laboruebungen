from decimal import Decimal
from unittest.mock import MagicMock

from dependency_injector import containers, providers
from django.urls import reverse
from django.test import Client, TestCase

import swd_django_demo
from core.domain import CatalogItem
from core.interfaces import ProductCatalog
from customers.models import Customer
from orders.basket import Basket
from products.models import Product

"""
    This is a test container that we will be used to mock the dependencies,
    as we don't want to test the dependencies, but ONLY the views and templates.
"""


class TestContainer(containers.DeclarativeContainer):
    product_factory = providers.Factory(CatalogItem, price=Decimal("0.0"))

    product_service = providers.Singleton(
        MagicMock,
        spec=ProductCatalog
    )


# This test class inherits from the TestCase class, which is provided by Django (not form unittest),
# as it provides some additional functionality that we need for testing Django views
class ViewTests(TestCase):
    def setUp(self):
        # Create a list of mock products
        # We are using the factory provider to create mock products and are not using a concrete product implementation
        self.mock_product_list = [
            TestContainer.product_factory(id=1, name="testproduct1", description="testdescription1"),
            TestContainer.product_factory(id=2, name="testproduct2", description="testdescription2"),
        ]
        # Create an instance of the test container
        c = TestContainer()
        # so we do not use a concrete product service, but a mock, for the service we have our own unit tests
        c.product_service().get_all_products.return_value = self.mock_product_list
        c.product_service().get_product.side_effect = lambda product_id: self.mock_product_list[int(product_id) - 1]
        # Wire the modules to use the TestContainer for the dependencies
        c.wire(modules=["products.views", "orders.views"])

    def tearDown(self):
        # restore the production wiring so later tests do not see the mocks
        swd_django_demo.get_container().wire(modules=["products.views", "orders.views"])

    def _fill_basket(self, *product_ids):
        session = self.client.session
        session[Basket.SESSION_KEY] = [str(product_id) for product_id in product_ids]
        session.save()

    def _fill_basket_with_real_products(self, count):
        # the basket overview is priced by the production OrderService against the database
        products = [Product.objects.create(name=f"product{i}", description="", price=Decimal("1.0")) for i in range(count)]
        self._fill_basket(*(product.id for product in products))

    def test_navigation_shows_basket_count_on_every_page(self):
        self._fill_basket(1, 1, 2)

        for url in (reverse("products:index"), reverse("orders:basket_overview")):
            response = self.client.get(url)
            self.assertEqual(response.context["basket_count"], 3)
            self.assertContains(response, 'id="basket-count">3<')

    def test_navigation_for_anonymous_visitor(self):
        response = self.client.get(reverse("products:index"))

        self.assertContains(response, "Log in")
        self.assertNotContains(response, "Log out")
        self.assertNotContains(response, "Admin site")

    def test_navigation_for_customer(self):
        self.client.force_login(Customer.objects.create(username="alice", credit=10))

        response = self.client.get(reverse("products:index"))

        self.assertContains(response, "alice")
        self.assertContains(response, reverse("core:logout"))
        self.assertNotContains(response, "Admin site")
        for theme in ("light", "dark", "auto"):
            self.assertContains(response, f'data-bs-theme-value="{theme}"')

    def test_navigation_for_staff_shows_admin_site(self):
        self.client.force_login(Customer.objects.create(username="root", credit=10, is_staff=True))

        response = self.client.get(reverse("products:index"))

        self.assertContains(response, "Admin site")

    def test_basket_overview_asks_anonymous_visitor_to_log_in(self):
        self._fill_basket_with_real_products(1)

        response = self.client.get(reverse("orders:basket_overview"))

        self.assertContains(response, "Please log in to place an order")
        self.assertNotContains(response, reverse("orders:place_order"))

    def test_basket_overview_offers_place_order_to_customer(self):
        self.client.force_login(Customer.objects.create(username="alice", credit=10))
        self._fill_basket_with_real_products(2)

        response = self.client.get(reverse("orders:basket_overview"))

        self.assertContains(response, reverse("orders:place_order"))
        self.assertEqual(len(response.context["order"].lines), 2)

    def test_empty_basket_overview(self):
        response = self.client.get(reverse("orders:basket_overview"))

        self.assertContains(response, "Your basket is empty")

    def test_order_list_requires_login(self):
        response = self.client.get(reverse("orders:order_list"))

        self.assertEqual(response.status_code, 302)
        self.assertIn("/admin/login/", response["Location"])
        self.assertNotContains(self.client.get(reverse("products:index")), "My orders")

    def test_order_list_shows_own_orders(self):
        alice = Customer.objects.create(username="alice", credit=100)
        bob = Customer.objects.create(username="bob", credit=100)
        product = Product.objects.create(name="apple", description="", price=Decimal("1.5"))
        order_service = swd_django_demo.get_container().order_service()
        mine = order_service.create_order(alice, Basket([product.id, product.id]))
        order_service.create_order(bob, Basket([product.id]))
        self.client.force_login(alice)

        response = self.client.get(reverse("orders:order_list"))

        self.assertContains(response, "My orders")
        self.assertContains(response, f"Order #{mine.pk}")
        self.assertEqual([o.order_number for o in response.context["orders"]], [mine.pk])
        self.assertContains(response, "3.00 &euro;")

    def test_logout_is_post_and_redirects_to_products(self):
        self.client.force_login(Customer.objects.create(username="alice", credit=10))

        response = self.client.post(reverse("core:logout"))

        self.assertRedirects(response, reverse("products:index"), fetch_redirect_response=False)


# The CSRF protection is switched off in the default test client, so these tests use a client that enforces it,
# exactly like a browser request would be checked in production.
class BasketCsrfTests(TestCase):
    def setUp(self):
        self.client = Client(enforce_csrf_checks=True)
        self.product = Product.objects.create(name="apple", description="", price=Decimal("1.5"))

    def _basket(self):
        return self.client.session.get(Basket.SESSION_KEY, [])

    def _csrf_token(self):
        # a page rendering {% csrf_token %} sets the cookie, as the browser would receive it.
        # the catalog renders one add-to-basket form per product, so this relies on the product from setUp
        self.client.get(reverse("products:index"))
        return self.client.cookies["csrftoken"].value

    def _fill_basket(self):
        session = self.client.session
        session[Basket.SESSION_KEY] = [str(self.product.id)]
        session.save()

    def test_add_to_basket_without_csrf_token_is_rejected(self):
        response = self.client.post(reverse("orders:add_to_basket"), {"product_id": self.product.id})

        self.assertEqual(response.status_code, 403)
        self.assertEqual(self._basket(), [])

    def test_add_to_basket_via_get_is_not_allowed(self):
        response = self.client.get(reverse("orders:add_to_basket"), {"product_id": self.product.id})

        self.assertEqual(response.status_code, 405)
        self.assertEqual(self._basket(), [])

    def test_add_to_basket_with_csrf_token(self):
        response = self.client.post(reverse("orders:add_to_basket"), {"product_id": self.product.id},
                                    HTTP_X_CSRFTOKEN=self._csrf_token())

        self.assertEqual(response.status_code, 302)
        self.assertEqual(self._basket(), [str(self.product.id)])

    def test_clear_basket_via_get_is_not_allowed(self):
        self._fill_basket()

        response = self.client.get(reverse("orders:clear_basket"))

        self.assertEqual(response.status_code, 405)
        self.assertEqual(self._basket(), [str(self.product.id)])

    def test_clear_basket_via_post_with_csrf_token(self):
        self._fill_basket()

        response = self.client.post(reverse("orders:clear_basket"), HTTP_X_CSRFTOKEN=self._csrf_token())

        self.assertRedirects(response, reverse("orders:basket_overview"), fetch_redirect_response=False)
        self.assertEqual(self._basket(), [])

    def test_basket_overview_clears_with_a_post_form(self):
        self._fill_basket()

        response = self.client.get(reverse("orders:basket_overview"))

        self.assertContains(response, f'<form method="post" action="{reverse("orders:clear_basket")}"')
        self.assertNotContains(response, f'href="{reverse("orders:clear_basket")}"')

    def test_place_order_via_get_is_not_allowed(self):
        self.client.force_login(Customer.objects.create(username="alice", credit=Decimal("10")))
        self._fill_basket()

        response = self.client.get(reverse("orders:place_order"))

        self.assertEqual(response.status_code, 405)
        self.assertEqual(self._basket(), [str(self.product.id)])
        self.assertEqual(Customer.objects.get(username="alice").credit, Decimal("10"))
