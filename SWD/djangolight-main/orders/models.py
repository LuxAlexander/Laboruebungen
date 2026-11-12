from django.conf import settings
from django.db import models

from core.enums import PaymentMethod, SalesChannel
from orders.managers import OrderManager


class Order(models.Model):
    """One purchase of single-match tickets (up to 4) for one match. customer is None for anonymous sales."""
    objects: OrderManager = OrderManager()

    customer = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.PROTECT, null=True, blank=True,
                                 related_name="orders")
    match = models.ForeignKey("stadiums.Match", on_delete=models.PROTECT, related_name="orders")
    channel = models.CharField(max_length=10, choices=SalesChannel.choices)
    payment_method = models.CharField(max_length=10, choices=PaymentMethod.choices)
    payment_reference = models.CharField(max_length=40)
    total_price = models.DecimalField(max_digits=10, decimal_places=2)
    created_at = models.DateTimeField(auto_now_add=True)

    def __str__(self) -> str:
        return f"Order #{self.pk}"


class OrderPosition(models.Model):
    """One ticket of an order, with the price paid for it."""
    order = models.ForeignKey(Order, on_delete=models.CASCADE, related_name="positions")
    pos = models.PositiveIntegerField()
    ticket = models.OneToOneField("tickets.Ticket", on_delete=models.PROTECT, related_name="order_position")
    price = models.DecimalField(max_digits=8, decimal_places=2)

    class Meta:
        constraints = [models.UniqueConstraint(fields=["order", "pos"], name="unique_position_per_order")]
