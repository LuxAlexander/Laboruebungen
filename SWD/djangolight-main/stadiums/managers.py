# repositories for the master data models
from datetime import timedelta

from django.db import models
from django.utils import timezone

from core.enums import MatchStatus


class MatchManager(models.Manager):
    def with_details(self) -> models.QuerySet:
        return self.get_queryset().select_related("competition", "stadium", "home_team", "away_team")

    def upcoming(self) -> models.QuerySet:
        """Matches on sale: planned or active, kickoff not more than 3 hours ago."""
        since = timezone.now() - timedelta(hours=3)
        return (self.with_details()
                .filter(status__in=[MatchStatus.PLANNED, MatchStatus.ACTIVE], kickoff__gte=since)
                .order_by("kickoff"))

    def active_in_stadium(self, stadium_id: int):
        return self.with_details().filter(stadium_id=stadium_id, status=MatchStatus.ACTIVE).first()


class SeatManager(models.Manager):
    def with_sector(self) -> models.QuerySet:
        return self.get_queryset().select_related("sector")
