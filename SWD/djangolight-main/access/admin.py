from django.contrib import admin

from access.models import AccessLog, SecurityAlert, TicketUsage


@admin.register(TicketUsage)
class TicketUsageAdmin(admin.ModelAdmin):
    list_display = ("ticket", "match", "entry_gate", "entered_at", "exit_gate", "left_at")
    list_filter = ("match",)
    raw_id_fields = ("ticket",)


@admin.register(AccessLog)
class AccessLogAdmin(admin.ModelAdmin):
    list_display = ("created_at", "ticket_number", "action", "granted", "reason", "gate", "match")
    list_filter = ("granted", "action", "reason", "match")
    search_fields = ("ticket_number",)
    raw_id_fields = ("ticket",)


@admin.register(SecurityAlert)
class SecurityAlertAdmin(admin.ModelAdmin):
    list_display = ("created_at", "ticket_number", "attempt_count", "gate", "match", "acknowledged")
    list_filter = ("acknowledged",)
