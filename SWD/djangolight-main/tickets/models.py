from django.conf import settings
from django.db import models
from django.db.models import Q

from core.enums import CompetitionType, PriceCategory, TicketStatus, TicketType
from tickets.managers import TicketManager

# tickets that occupy their seat (a cancelled ticket frees it again)
OCCUPYING = [TicketStatus.SOLD, TicketStatus.RESERVED]


class PriceRule(models.Model):
    """Ticket price by price category (of the sector) and competition type."""
    category = models.CharField(max_length=10, choices=PriceCategory.choices)
    competition_type = models.CharField(max_length=20, choices=CompetitionType.choices)
    price = models.DecimalField(max_digits=8, decimal_places=2)

    class Meta:
        constraints = [models.UniqueConstraint(fields=["category", "competition_type"], name="unique_price_rule")]

    def __str__(self) -> str:
        return f"{self.get_category_display()} / {self.get_competition_type_display()}: {self.price}"


class Ticket(models.Model):
    """
    An admission ticket bound to exactly one fixed seat.
    Single-match ticket: valid for `match`. Season ticket: valid for all matches of `competition`.
    """
    objects: TicketManager = TicketManager()

    number = models.CharField(max_length=20, unique=True)  # random, e.g. TCK-7F3K9Q
    type = models.CharField(max_length=10, choices=TicketType.choices)
    status = models.CharField(max_length=10, choices=TicketStatus.choices, default=TicketStatus.SOLD)
    seat = models.ForeignKey("stadiums.Seat", on_delete=models.PROTECT, related_name="tickets")
    match = models.ForeignKey("stadiums.Match", on_delete=models.PROTECT, null=True, blank=True,
                              related_name="tickets")
    competition = models.ForeignKey("stadiums.Competition", on_delete=models.PROTECT, null=True, blank=True,
                                    related_name="season_tickets")
    customer = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.PROTECT, null=True, blank=True,
                                 related_name="tickets")  # None: sold anonymously
    price = models.DecimalField(max_digits=8, decimal_places=2)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        constraints = [
            # a single ticket refers to a match, a season ticket to a competition (and requires a customer)
            models.CheckConstraint(
                condition=(Q(type=TicketType.SINGLE, match__isnull=False, competition__isnull=True)
                           | Q(type=TicketType.SEASON, competition__isnull=False, match__isnull=True,
                               customer__isnull=False)),
                name="ticket_type_matches_validity"),
            # safety net against double-selling (the seat row lock is the primary protection)
            models.UniqueConstraint(fields=["seat", "match"],
                                    condition=Q(type=TicketType.SINGLE, status__in=OCCUPYING),
                                    name="one_single_ticket_per_seat_and_match"),
            models.UniqueConstraint(fields=["seat", "competition"],
                                    condition=Q(type=TicketType.SEASON, status__in=OCCUPYING),
                                    name="one_season_ticket_per_seat_and_competition"),
        ]

    def __str__(self) -> str:
        return self.number
