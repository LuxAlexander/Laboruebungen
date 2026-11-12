from datetime import date
from typing import Optional

from django.db import transaction
from django.db.models import F, Sum

from core.domain import MatchStatisticsInfo, UtilisationReport
from core.enums import MatchStatus
from stadiums.models import CompetitionStatistics, Match, MatchStatistics, Sector
from stadiums.services import to_match_info


class InvalidMatchStatus(Exception):
    pass


class StatisticsService:
    """Adapter for core.interfaces.Statistics. Counters are updated with F() so concurrent updates add up."""

    def record_sale(self, match_id: int, count: int) -> None:
        MatchStatistics.objects.get_or_create(match_id=match_id)
        MatchStatistics.objects.filter(match_id=match_id).update(tickets_sold=F("tickets_sold") + count)

    def record_entry(self, match_id: int) -> None:
        MatchStatistics.objects.get_or_create(match_id=match_id)
        MatchStatistics.objects.filter(match_id=match_id).update(tickets_used=F("tickets_used") + 1)

    def conduct_match(self, match_id: int) -> None:
        with transaction.atomic():
            match = Match.objects.select_for_update().get(pk=match_id)
            if match.status != MatchStatus.ACTIVE:
                raise InvalidMatchStatus(f"only an active match can be conducted, status is {match.status}")
            match.status = MatchStatus.CONDUCTED
            match.save(update_fields=["status"])

            stats, _ = MatchStatistics.objects.get_or_create(match=match)
            CompetitionStatistics.objects.get_or_create(competition_id=match.competition_id)
            CompetitionStatistics.objects.filter(competition_id=match.competition_id).update(
                matches_conducted=F("matches_conducted") + 1,
                tickets_sold=F("tickets_sold") + stats.tickets_sold,
                tickets_used=F("tickets_used") + stats.tickets_used,
                capacity=F("capacity") + self._capacity(match.stadium_id),
            )

    def match_statistics(self, match_id: int) -> Optional[MatchStatisticsInfo]:
        match = Match.objects.with_details().filter(pk=match_id).first()
        if match is None:
            return None
        stats, _ = MatchStatistics.objects.get_or_create(match=match)
        return MatchStatisticsInfo(match=to_match_info(match), capacity=self._capacity(match.stadium_id),
                                   tickets_sold=stats.tickets_sold, tickets_used=stats.tickets_used)

    def utilisation(self, label: str, start: Optional[date] = None, end: Optional[date] = None,
                    competition_id: Optional[int] = None) -> UtilisationReport:
        """Statistics of the conducted matches in a period and/or competition."""
        matches = Match.objects.with_details().filter(status=MatchStatus.CONDUCTED).select_related("statistics")
        if start:
            matches = matches.filter(kickoff__date__gte=start)
        if end:
            matches = matches.filter(kickoff__date__lte=end)
        if competition_id:
            matches = matches.filter(competition_id=competition_id)

        capacities: dict[int, int] = {}
        infos = []
        for match in matches.order_by("kickoff"):
            if match.stadium_id not in capacities:
                capacities[match.stadium_id] = self._capacity(match.stadium_id)
            stats = getattr(match, "statistics", None)
            infos.append(MatchStatisticsInfo(match=to_match_info(match), capacity=capacities[match.stadium_id],
                                             tickets_sold=stats.tickets_sold if stats else 0,
                                             tickets_used=stats.tickets_used if stats else 0))
        return UtilisationReport(label=label, start=start, end=end, matches=infos)

    @staticmethod
    def _capacity(stadium_id: int) -> int:
        return Sector.objects.filter(stadium_id=stadium_id).aggregate(total=Sum("capacity"))["total"] or 0
