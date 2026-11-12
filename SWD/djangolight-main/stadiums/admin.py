from django.contrib import admin, messages
from django.db import IntegrityError

from core.enums import MatchStatus
from stadiums.models import (Competition, CompetitionStatistics, Gate, Match, MatchStatistics, Sector, Stadium,
                             Team)
from stadiums.statistics import InvalidMatchStatus

"""
Master data is maintained here (no bespoke CRUD UI is required). Seats are not listed: 32,000 rows,
created by the seed command. The match status transitions are admin actions.
"""


class SectorInline(admin.TabularInline):
    model = Sector
    extra = 0


class GateInline(admin.TabularInline):
    model = Gate
    extra = 0


@admin.register(Stadium)
class StadiumAdmin(admin.ModelAdmin):
    list_display = ("name", "city", "capacity")
    inlines = [SectorInline, GateInline]


@admin.register(Competition)
class CompetitionAdmin(admin.ModelAdmin):
    list_display = ("name", "type", "season")
    list_filter = ("type", "season")


@admin.register(Match)
class MatchAdmin(admin.ModelAdmin):
    list_display = ("__str__", "competition", "kickoff", "status")
    list_filter = ("status", "competition")
    actions = ["activate", "conduct", "cancel"]

    @admin.action(description="Activate (open the gates)")
    def activate(self, request, queryset):
        for match in queryset.filter(status=MatchStatus.PLANNED):
            match.status = MatchStatus.ACTIVE
            try:
                match.save(update_fields=["status"])
            except IntegrityError:
                self.message_user(request, f"{match}: another match in this stadium is already active.",
                                  messages.ERROR)

    @admin.action(description="Set conducted (adds statistics to the competition)")
    def conduct(self, request, queryset):
        from swd_django_demo import get_container
        statistics = get_container().statistics_service()
        for match in queryset:
            try:
                statistics.conduct_match(match.pk)
            except InvalidMatchStatus as e:
                self.message_user(request, f"{match}: {e}", messages.ERROR)

    @admin.action(description="Cancel")
    def cancel(self, request, queryset):
        queryset.filter(status__in=[MatchStatus.PLANNED, MatchStatus.ACTIVE]).update(status=MatchStatus.CANCELLED)


admin.site.register(Team)
admin.site.register(MatchStatistics)
admin.site.register(CompetitionStatistics)
