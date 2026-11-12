from typing import Optional

from core.domain import GateInfo, MatchInfo, SeatInfo, SectorInfo
from core.enums import CompetitionType, MatchStatus, PriceCategory
from stadiums.models import Gate, Match, Seat, Sector


class MatchCatalogService:
    """Adapter for core.interfaces.MatchCatalog, backed by the stadiums models."""

    def upcoming_matches(self) -> list[MatchInfo]:
        return [to_match_info(m) for m in Match.objects.upcoming()]

    def get_match(self, match_id: int) -> Optional[MatchInfo]:
        match = Match.objects.with_details().filter(pk=match_id).first()
        return to_match_info(match) if match else None

    def active_match(self, gate_id: int) -> Optional[MatchInfo]:
        gate = Gate.objects.filter(pk=gate_id).first()
        if gate is None:
            return None
        match = Match.objects.active_in_stadium(gate.stadium_id)
        return to_match_info(match) if match else None

    def gates(self) -> list[GateInfo]:
        return [GateInfo(id=g.pk, name=g.name, stadium_id=g.stadium_id, stadium_name=g.stadium.name)
                for g in Gate.objects.select_related("stadium").order_by("stadium__name", "name")]

    def sectors(self, stadium_id: int) -> list[SectorInfo]:
        return [to_sector_info(s) for s in Sector.objects.filter(stadium_id=stadium_id).order_by("name")]

    def get_sector(self, sector_id: int) -> Optional[SectorInfo]:
        sector = Sector.objects.filter(pk=sector_id).first()
        return to_sector_info(sector) if sector else None

    def seats(self, sector_id: int, row: Optional[int] = None) -> list[SeatInfo]:
        seats = Seat.objects.with_sector().filter(sector_id=sector_id)
        if row is not None:
            seats = seats.filter(row=row)
        return [to_seat_info(s) for s in seats.order_by("row", "number")]

    def get_seats(self, seat_ids: list[int]) -> list[SeatInfo]:
        return [to_seat_info(s) for s in Seat.objects.with_sector().filter(pk__in=seat_ids).order_by("pk")]

    def lock_seats(self, seat_ids: list[int]) -> list[SeatInfo]:
        # fixed lock order (by id) so that two buyers of overlapping seats cannot deadlock
        seats = Seat.objects.select_for_update().filter(pk__in=seat_ids).order_by("pk")
        return [to_seat_info(s) for s in seats.select_related("sector")]

    def rows(self, sector_id: int) -> list[int]:
        return list(Seat.objects.filter(sector_id=sector_id).values_list("row", flat=True).distinct().order_by("row"))


def to_match_info(match: Match) -> MatchInfo:
    return MatchInfo(id=match.pk, home_team=match.home_team.name, away_team=match.away_team.name,
                     kickoff=match.kickoff, status=MatchStatus(match.status), stadium_id=match.stadium_id,
                     stadium_name=match.stadium.name, competition_id=match.competition_id,
                     competition_name=str(match.competition),
                     competition_type=CompetitionType(match.competition.type))


def to_sector_info(sector: Sector) -> SectorInfo:
    return SectorInfo(id=sector.pk, name=sector.name, capacity=sector.capacity,
                      category=PriceCategory(sector.category), stadium_id=sector.stadium_id)


def to_seat_info(seat: Seat) -> SeatInfo:
    return SeatInfo(id=seat.pk, sector_id=seat.sector_id, sector_name=seat.sector.name, row=seat.row,
                    number=seat.number)
