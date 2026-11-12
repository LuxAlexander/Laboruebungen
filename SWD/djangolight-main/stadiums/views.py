from datetime import date

from dependency_injector.wiring import Provide, inject
from django.contrib.admin.views.decorators import staff_member_required
from django.http import HttpRequest, HttpResponse
from django.shortcuts import render
from django.utils import timezone

from core.domain import UtilisationReport
from core.interfaces import Statistics
from stadiums.models import Competition

PERIODS = {
    "all": "All conducted matches",
    "winter": "Winter season (Jul - Dec)",
    "spring": "Spring season (Jan - Jun)",
    "competition": "One competition (e.g. Champions League)",
    "custom": "Custom period",
}


def _parse_date(value: str | None) -> date | None:
    try:
        return date.fromisoformat(value) if value else None
    except ValueError:
        return None


@staff_member_required
@inject
def statistics(request: HttpRequest, statistics_service: Statistics = Provide["statistics_service"]) -> HttpResponse:
    """Utilisation statistics over a selectable period (FR-15, FR-16). Staff only."""
    today = timezone.localdate()
    season_start = today.year if today.month >= 7 else today.year - 1
    period = request.GET.get("period", "all")
    year = int(request.GET.get("year") or season_start)
    competition_id = request.GET.get("competition") or None

    match period:
        case "winter":
            report = statistics_service.utilisation(f"Winter season {year}", date(year, 7, 1), date(year, 12, 31))
        case "spring":
            report = statistics_service.utilisation(f"Spring season {year + 1}", date(year + 1, 1, 1),
                                                    date(year + 1, 6, 30))
        case "competition" if competition_id:
            competition = Competition.objects.filter(pk=competition_id).first()
            report = statistics_service.utilisation(str(competition), competition_id=int(competition_id))
        case "custom":
            start, end = _parse_date(request.GET.get("start")), _parse_date(request.GET.get("end"))
            report = statistics_service.utilisation("Custom period", start, end)
        case _:
            report: UtilisationReport = statistics_service.utilisation("All conducted matches")

    context = {
        "report": report,
        "periods": PERIODS,
        "period": period,
        "year": year,
        "competitions": Competition.objects.order_by("-season", "name"),
        "competition_id": competition_id,
    }
    return render(request, "stadiums/statistics.html", context)
