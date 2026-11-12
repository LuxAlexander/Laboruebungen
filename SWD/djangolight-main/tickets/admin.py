from django.contrib import admin

from tickets.models import PriceRule, Ticket


@admin.register(PriceRule)
class PriceRuleAdmin(admin.ModelAdmin):
    list_display = ("category", "competition_type", "price")
    list_filter = ("competition_type",)


@admin.register(Ticket)
class TicketAdmin(admin.ModelAdmin):
    list_display = ("number", "type", "status", "seat", "match", "competition", "customer", "price")
    list_filter = ("type", "status", "competition")
    search_fields = ("number", "customer__username")
    raw_id_fields = ("seat", "match", "competition", "customer")
