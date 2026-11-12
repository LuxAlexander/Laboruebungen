from django.db import models
from django.db.models import Q

from core.enums import AccessAction, DenyReason


class TicketUsage(models.Model):
    """
    One stay of a ticket in the stadium during a match: entered_at .. left_at.
    left_at is None while the spectator is inside. A ticket can have several stays per match (leave + re-entry),
    but never two open ones: that is the database safety net against admitting a ticket twice.
    """
    ticket = models.ForeignKey("tickets.Ticket", on_delete=models.PROTECT, related_name="usages")
    match = models.ForeignKey("stadiums.Match", on_delete=models.PROTECT, related_name="usages")
    entry_gate = models.ForeignKey("stadiums.Gate", on_delete=models.PROTECT, related_name="+")
    entered_at = models.DateTimeField(auto_now_add=True)
    exit_gate = models.ForeignKey("stadiums.Gate", on_delete=models.PROTECT, null=True, blank=True, related_name="+")
    left_at = models.DateTimeField(null=True, blank=True)

    class Meta:
        constraints = [
            models.UniqueConstraint(fields=["ticket", "match"], condition=Q(left_at__isnull=True),
                                    name="one_open_entry_per_ticket_and_match"),
        ]


class AccessLog(models.Model):
    """Every scan at a gate, granted or denied (traceability, marketing). Never changed or deleted."""
    ticket_number = models.CharField(max_length=40)
    ticket = models.ForeignKey("tickets.Ticket", on_delete=models.PROTECT, null=True, blank=True,
                               related_name="access_logs")  # None for unknown numbers
    match = models.ForeignKey("stadiums.Match", on_delete=models.PROTECT, null=True, blank=True)
    gate = models.ForeignKey("stadiums.Gate", on_delete=models.PROTECT)
    action = models.CharField(max_length=10, choices=AccessAction.choices)
    granted = models.BooleanField()
    reason = models.CharField(max_length=30, choices=DenyReason.choices, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        ordering = ["-created_at"]
        indexes = [models.Index(fields=["ticket_number", "match", "reason"])]


class SecurityAlert(models.Model):
    """Raised when a ticket that is already inside is presented again; staff see it on the alerts page."""
    ticket_number = models.CharField(max_length=40)
    match = models.ForeignKey("stadiums.Match", on_delete=models.PROTECT)
    gate = models.ForeignKey("stadiums.Gate", on_delete=models.PROTECT)
    attempt_count = models.PositiveIntegerField()  # > 1: repeated attempt
    created_at = models.DateTimeField(auto_now_add=True)
    acknowledged = models.BooleanField(default=False)

    class Meta:
        ordering = ["acknowledged", "-created_at"]

    @property
    def repeated(self) -> bool:
        return self.attempt_count > 1
