from dependency_injector.wiring import Provide, inject
from django.contrib import messages
from django.contrib.admin.views.decorators import staff_member_required
from django.db import DatabaseError
from django.http import HttpRequest, HttpResponse
from django.shortcuts import redirect, render
from django.views.decorators.http import require_POST

from core.domain import EntryResult
from core.interfaces import MatchCatalog

from .models import SecurityAlert
from .services import AccessService

SESSION_GATE = "gate_id"


@inject
def gate(request: HttpRequest, catalog: MatchCatalog = Provide["match_catalog"],
         access_service: AccessService = Provide["access_service"]) -> HttpResponse:
    """
    Lightweight HTML page that simulates an entry gate: enter a ticket number, press Enter or Leave,
    see grant/deny. It drives the same server-side flow a real turnstile would.
    """
    gates = catalog.gates()
    gate_id = _int(request.POST.get("gate_id") or request.session.get(SESSION_GATE)) or (gates[0].id if gates else None)
    result: EntryResult | None = None

    if request.method == "POST" and gate_id:
        request.session[SESSION_GATE] = gate_id
        number = request.POST.get("ticket_number", "")
        try:
            if request.POST.get("action") == "leave":
                result = access_service.leave(number, gate_id)
            else:
                result = access_service.enter(number, gate_id)
        except DatabaseError:
            # E.5: no automatic entry when the server cannot decide; staff handle it manually
            result = EntryResult(granted=False, ticket_number=number,
                                 message="System error: entry not granted, please contact the staff.")

    context = {
        "gates": gates,
        "gate_id": gate_id,
        "active_match": catalog.active_match(gate_id) if gate_id else None,
        "result": result,
    }
    return render(request, "access/gate.html", context)


@staff_member_required
def alerts(request: HttpRequest) -> HttpResponse:
    """Double entry attempts for security staff."""
    return render(request, "access/alerts.html", {
        "alerts": SecurityAlert.objects.select_related("gate", "match__home_team", "match__away_team")[:200]})


@staff_member_required
@require_POST
def acknowledge(request: HttpRequest, alert_id: int) -> HttpResponse:
    SecurityAlert.objects.filter(pk=alert_id).update(acknowledged=True)
    messages.success(request, "Alert acknowledged.")
    return redirect("access:alerts")


def _int(value) -> int | None:
    try:
        return int(value)
    except (TypeError, ValueError):
        return None
