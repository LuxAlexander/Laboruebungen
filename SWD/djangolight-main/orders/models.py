from django.conf import settings
from django.db import models

from orders.managers import OrderManager


# This file defines the models for our application
# The Order model represents an order made by a customer
class Order(models.Model):
    # We use our own manager to manage instances of the Order model
    objects: OrderManager = OrderManager()
    # The user field is a foreign key to the customer who made the order.
    # AUTH_USER_MODEL is Django's documented way to reference the user model (customers.Customer)
    user: models.ForeignKey = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.PROTECT)
    total_price: models.DecimalField = models.DecimalField(max_digits=10, decimal_places=2)
    created_at: models.DateTimeField = models.DateTimeField(auto_now_add=True)


# The OrderPosition model represents a single product in an order
class OrderPosition(models.Model):
    # Django does not support composite primary keys, so we have to use a unique_together constraint
    # ticket is open for this since 18 years!!! https://code.djangoproject.com/ticket/373
    class Meta:
        # This constraint ensures that each (order, pos) pair is unique
        unique_together = (('order', 'pos'),)

    # The order field is a foreign key to the order that this position belongs to
    # add a related name to the foreign key, so that we can easily access the order positions of an order
    order: models.ForeignKey[Order] = models.ForeignKey(Order, on_delete=models.CASCADE, related_name='order_positions')
    # The pos field is an integer that represents the position of this order position within the order
    pos: models.PositiveIntegerField = models.PositiveIntegerField()
    # The product field is a foreign key to the product being ordered.
    # The orders app depends on the products app at the data level; the services still only use the
    # ProductCatalog interface. on_delete=models.PROTECT ensures that we cannot delete a product that is
    # referenced by an order position.
    product: models.ForeignKey = models.ForeignKey("products.Product", on_delete=models.PROTECT)
    # The quantity field is the number of units of the product being ordered
    quantity: models.IntegerField = models.IntegerField()
    # The price field is the price of the product being ordered
    price: models.DecimalField = models.DecimalField(max_digits=10, decimal_places=2)
