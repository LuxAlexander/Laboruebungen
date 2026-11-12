from django.db import models
from django.db.models import Q

from core.enums import CompetitionType, MatchStatus, PriceCategory
from stadiums.managers import MatchManager, SeatManager

"""
Master data (Data Management component): stadiums, sectors, seats, gates, teams, competitions and matches,
plus the match and competition statistics. Filled by `manage.py seed_stadium`, maintained in the admin site.
"""


class Stadium(models.Model):
    name = models.CharField(max_length=200, unique=True)
    street = models.CharField(max_length=200)
    postal_code = models.CharField(max_length=20)
    city = models.CharField(max_length=100)

    @property
    def capacity(self) -> int:
        return self.sectors.aggregate(total=models.Sum("capacity"))["total"] or 0

    def __str__(self) -> str:
        return self.name


class Sector(models.Model):
    stadium = models.ForeignKey(Stadium, on_delete=models.CASCADE, related_name="sectors")
    name = models.CharField(max_length=10)  # "A" .. "H"
    capacity = models.PositiveIntegerField()
    category = models.CharField(max_length=10, choices=PriceCategory.choices)

    class Meta:
        ordering = ["stadium", "name"]
        constraints = [models.UniqueConstraint(fields=["stadium", "name"], name="unique_sector_per_stadium")]

    def __str__(self) -> str:
        return f"{self.stadium} / {self.name}"


class Seat(models.Model):
    objects: SeatManager = SeatManager()

    sector = models.ForeignKey(Sector, on_delete=models.CASCADE, related_name="seats")
    row = models.PositiveIntegerField()
    number = models.PositiveIntegerField()

    class Meta:
        ordering = ["sector", "row", "number"]
        constraints = [models.UniqueConstraint(fields=["sector", "row", "number"], name="unique_seat_position")]

    def __str__(self) -> str:
        return f"{self.sector.name}-{self.row}-{self.number}"


class Gate(models.Model):
    """An entry/exit gate of a stadium; every gate can be used for entering and leaving."""
    stadium = models.ForeignKey(Stadium, on_delete=models.CASCADE, related_name="gates")
    name = models.CharField(max_length=50)

    class Meta:
        constraints = [models.UniqueConstraint(fields=["stadium", "name"], name="unique_gate_per_stadium")]

    def __str__(self) -> str:
        return f"{self.stadium} / {self.name}"


class Team(models.Model):
    name = models.CharField(max_length=100, unique=True)

    def __str__(self) -> str:
        return self.name


class Competition(models.Model):
    """A competition/series in one season, e.g. Bundesliga 2026/27. Recurs every season as a new row."""
    name = models.CharField(max_length=100)
    type = models.CharField(max_length=20, choices=CompetitionType.choices)
    season = models.CharField(max_length=9)  # e.g. "2026/27"

    class Meta:
        constraints = [models.UniqueConstraint(fields=["name", "season"], name="unique_competition_per_season")]

    def __str__(self) -> str:
        return f"{self.name} {self.season}"


class Match(models.Model):
    objects: MatchManager = MatchManager()

    competition = models.ForeignKey(Competition, on_delete=models.PROTECT, related_name="matches")
    stadium = models.ForeignKey(Stadium, on_delete=models.PROTECT, related_name="matches")
    home_team = models.ForeignKey(Team, on_delete=models.PROTECT, related_name="home_matches")
    away_team = models.ForeignKey(Team, on_delete=models.PROTECT, related_name="away_matches")
    kickoff = models.DateTimeField()
    status = models.CharField(max_length=10, choices=MatchStatus.choices, default=MatchStatus.PLANNED)

    class Meta:
        ordering = ["kickoff"]
        verbose_name_plural = "matches"
        constraints = [
            # exactly two (different) teams per match
            models.CheckConstraint(condition=~Q(home_team=models.F("away_team")), name="two_different_teams"),
            # entry checks look up "the" active match of a stadium, so there may be at most one
            models.UniqueConstraint(fields=["stadium"], condition=Q(status=MatchStatus.ACTIVE),
                                    name="one_active_match_per_stadium"),
        ]

    def __str__(self) -> str:
        return f"{self.home_team} vs {self.away_team} ({self.kickoff:%Y-%m-%d %H:%M})"


class MatchStatistics(models.Model):
    """Per-match counters, updated atomically (F expressions) on every sale and first entry."""
    match = models.OneToOneField(Match, on_delete=models.CASCADE, related_name="statistics")
    tickets_sold = models.PositiveIntegerField(default=0)  # single tickets sold + season tickets of the competition
    tickets_used = models.PositiveIntegerField(default=0)  # tickets that entered at least once

    class Meta:
        verbose_name_plural = "match statistics"


class CompetitionStatistics(models.Model):
    """Cumulative statistics of a competition; a match is added only once its status is conducted."""
    competition = models.OneToOneField(Competition, on_delete=models.CASCADE, related_name="statistics")
    matches_conducted = models.PositiveIntegerField(default=0)
    tickets_sold = models.PositiveIntegerField(default=0)
    tickets_used = models.PositiveIntegerField(default=0)
    capacity = models.PositiveIntegerField(default=0)  # sum of the stadium capacities of the conducted matches

    class Meta:
        verbose_name_plural = "competition statistics"
