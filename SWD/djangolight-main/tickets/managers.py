# repository for tickets
from django.db import models
from django.db.models import Q

from core.enums import TicketStatus, TicketType


class TicketManager(models.Manager):
    def with_seat(self) -> models.QuerySet:
        return self.get_queryset().select_related("seat__sector")

    def occupying(self, match_id: int, competition_id: int) -> models.QuerySet:
        """Tickets that block a seat for a match: single tickets of the match, season tickets of its competition."""
        return self.get_queryset().filter(
            Q(type=TicketType.SINGLE, match_id=match_id) | Q(type=TicketType.SEASON, competition_id=competition_id),
            status__in=[TicketStatus.SOLD, TicketStatus.RESERVED],
        )
