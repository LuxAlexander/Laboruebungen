import logging
from typing import Optional

from django.db import transaction
from django.utils import timezone

from core.domain import EntryResult, MatchInfo, TicketInfo, normalize_ticket_number
from core.enums import AccessAction, DenyReason, TicketStatus, TicketType
from core.interfaces import MatchCatalog, SecurityAlerts, Statistics, Tickets
from access.models import AccessLog, SecurityAlert, TicketUsage

logger = logging.getLogger(__name__)


class _Denied(Exception):
    def __init__(self, reason: DenyReason, match: Optional[MatchInfo] = None, ticket: Optional[TicketInfo] = None):
        self.reason, self.match, self.ticket = reason, match, ticket


class SecurityAlertService:
    """Adapter for core.interfaces.SecurityAlerts: stores the alert for the staff page and logs a warning."""

    def notify_double_entry(self, ticket_number: str, match_id: int, gate_id: int, attempt_count: int) -> None:
        SecurityAlert.objects.create(ticket_number=ticket_number, match_id=match_id, gate_id=gate_id,
                                     attempt_count=attempt_count)
        logger.warning(f"double entry attempt #{attempt_count} with ticket {ticket_number} at gate {gate_id}")


class AccessService:
    """
    Use case 4.2 Access Control. All checks run on the server, the gate is only a client.

    Consistency (see report, sequence diagram):
    - one transaction per scan; the ticket row is locked (select_for_update), so simultaneous scans of the
      same ticket at two gates are serialised and the second one sees the open entry;
    - safety net: unique constraint "one open entry per ticket and match" on TicketUsage;
    - denied attempts are logged AFTER the transaction block, in their own transaction, so the audit trail
      is kept even though the check itself was rolled back.
    """

    def __init__(self, catalog: MatchCatalog, tickets: Tickets, statistics: Statistics, alerts: SecurityAlerts):
        self.catalog = catalog
        self.tickets = tickets
        self.statistics = statistics
        self.alerts = alerts

    def enter(self, ticket_number: str, gate_id: int) -> EntryResult:
        number = normalize_ticket_number(ticket_number)
        try:
            with transaction.atomic():
                match, ticket = self._load(number, gate_id)
                if not self._valid_for(ticket, match):
                    raise _Denied(DenyReason.NOT_VALID_FOR_MATCH, match, ticket)
                if TicketUsage.objects.filter(ticket_id=ticket.id, match_id=match.id, left_at__isnull=True).exists():
                    raise _Denied(DenyReason.ALREADY_INSIDE, match, ticket)

                first_entry = not TicketUsage.objects.filter(ticket_id=ticket.id, match_id=match.id).exists()
                TicketUsage.objects.create(ticket_id=ticket.id, match_id=match.id, entry_gate_id=gate_id)
                self._log(number, gate_id, AccessAction.ENTER, True, match, ticket)
                if first_entry:  # re-entries are not counted twice
                    self.statistics.record_entry(match.id)
        except _Denied as denied:
            return self._deny(number, gate_id, AccessAction.ENTER, denied)

        return EntryResult(granted=True, message="Welcome! Entry granted." if first_entry else "Welcome back!",
                           ticket_number=number, match_id=match.id, seat_label=ticket.seat.label)

    def leave(self, ticket_number: str, gate_id: int) -> EntryResult:
        number = normalize_ticket_number(ticket_number)
        try:
            with transaction.atomic():
                match, ticket = self._load(number, gate_id)
                usage = (TicketUsage.objects.select_for_update()
                         .filter(ticket_id=ticket.id, match_id=match.id, left_at__isnull=True).first())
                if usage is None:
                    raise _Denied(DenyReason.NOT_INSIDE, match, ticket)
                usage.left_at, usage.exit_gate_id = timezone.now(), gate_id
                usage.save(update_fields=["left_at", "exit_gate"])
                self._log(number, gate_id, AccessAction.LEAVE, True, match, ticket)
        except _Denied as denied:
            return self._deny(number, gate_id, AccessAction.LEAVE, denied)

        return EntryResult(granted=True, message="Exit registered. You can re-enter with the same ticket.",
                           ticket_number=number, match_id=match.id, seat_label=ticket.seat.label)

    # --- helpers ------------------------------------------------------------------------------------------------

    def _load(self, number: str, gate_id: int) -> tuple[MatchInfo, TicketInfo]:
        ticket = self.tickets.lock_ticket(number)
        if ticket is None:
            raise _Denied(DenyReason.UNKNOWN_TICKET)
        match = self.catalog.active_match(gate_id)
        if match is None:
            raise _Denied(DenyReason.NO_ACTIVE_MATCH, ticket=ticket)
        if ticket.status == TicketStatus.CANCELLED:
            raise _Denied(DenyReason.TICKET_CANCELLED, match, ticket)
        return match, ticket

    @staticmethod
    def _valid_for(ticket: TicketInfo, match: MatchInfo) -> bool:
        if ticket.type == TicketType.SINGLE:
            return ticket.match_id == match.id
        return ticket.competition_id == match.competition_id

    def _deny(self, number: str, gate_id: int, action: AccessAction, denied: _Denied) -> EntryResult:
        match_id = denied.match.id if denied.match else None
        self._log(number, gate_id, action, False, denied.match, denied.ticket, denied.reason)
        if denied.reason == DenyReason.ALREADY_INSIDE:
            attempts = AccessLog.objects.filter(ticket_number=number, match_id=match_id,
                                                reason=DenyReason.ALREADY_INSIDE).count()
            self.alerts.notify_double_entry(number, match_id, gate_id, attempts)
        return EntryResult(granted=False, message=denied.reason.label, ticket_number=number, reason=denied.reason,
                           match_id=match_id, seat_label=denied.ticket.seat.label if denied.ticket else "")

    @staticmethod
    def _log(number: str, gate_id: int, action: AccessAction, granted: bool, match: Optional[MatchInfo],
             ticket: Optional[TicketInfo], reason: Optional[DenyReason] = None) -> None:
        AccessLog.objects.create(ticket_number=number, ticket_id=ticket.id if ticket else None,
                                 match_id=match.id if match else None, gate_id=gate_id, action=action,
                                 granted=granted, reason=reason or "")
