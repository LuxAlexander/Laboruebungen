import json
from datetime import datetime, time, timedelta
from pathlib import Path
from typing import Any

from django.db import transaction
from django.utils import timezone

from core.enums import MatchStatus
from stadiums.models import (Competition, CompetitionStatistics, Gate, Match, MatchStatistics, Seat, Sector, Stadium,
                             Team)

"""
Master data comes from a JSON file (stadiums/data/stadium.json), no CRUD UI is needed.
Running the seed twice does not duplicate anything: rows are matched by their natural keys,
existing matches are left unchanged. Seat rows are created per sector: capacity / seats_per_row rows.
Match dates are relative to today, so the demo data always has past, current and upcoming matches.
"""

DEFAULT_FILE = Path(__file__).parent / "data" / "stadium.json"


def read_config(path: Path | str = DEFAULT_FILE) -> dict[str, Any]:
    return json.loads(Path(path).read_text(encoding="utf-8"))


@transaction.atomic
def seed_stadium(config: dict[str, Any]) -> Stadium:
    data = config["stadium"]
    stadium, _ = Stadium.objects.update_or_create(name=data["name"], defaults={
        "street": data["street"], "postal_code": data["postal_code"], "city": data["city"]})

    seats_per_row: int = config.get("seats_per_row", 50)
    for s in config["sectors"]:
        sector, _ = Sector.objects.update_or_create(stadium=stadium, name=s["name"], defaults={
            "capacity": s["capacity"], "category": s["category"]})
        if not sector.seats.exists():
            Seat.objects.bulk_create(
                Seat(sector=sector, row=i // seats_per_row + 1, number=i % seats_per_row + 1)
                for i in range(sector.capacity)
            )

    for name in config.get("gates", []):
        Gate.objects.get_or_create(stadium=stadium, name=name)

    teams = {name: Team.objects.get_or_create(name=name)[0] for name in config.get("teams", [])}
    competitions = {}
    for c in config.get("competitions", []):
        competition, _ = Competition.objects.get_or_create(name=c["name"], season=c["season"],
                                                           defaults={"type": c["type"]})
        competitions[c["name"]] = competition
        CompetitionStatistics.objects.get_or_create(competition=competition)

    today = timezone.localdate()
    for m in config.get("matches", []):
        kickoff = timezone.make_aware(datetime.combine(today + timedelta(days=m["days_from_today"]),
                                                       time.fromisoformat(m["time"])))
        competition = competitions[m["competition"]]
        match, created = Match.objects.get_or_create(
            competition=competition, home_team=teams[m["home"]], away_team=teams[m["away"]],
            defaults={"stadium": stadium, "kickoff": kickoff, "status": m.get("status", MatchStatus.PLANNED)})
        stats, _ = MatchStatistics.objects.get_or_create(match=match)
        if created and match.status == MatchStatus.CONDUCTED:
            # historical figures for the statistics page, added to the competition like a conducted match
            stats.tickets_sold = m.get("tickets_sold", 0)
            stats.tickets_used = m.get("tickets_used", 0)
            stats.save()
            cs = CompetitionStatistics.objects.get(competition=competition)
            cs.matches_conducted += 1
            cs.tickets_sold += stats.tickets_sold
            cs.tickets_used += stats.tickets_used
            cs.capacity += stadium.capacity
            cs.save()
    return stadium
