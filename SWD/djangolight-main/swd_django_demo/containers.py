"""
The dependency-injection container: it declares the adapters at the seams between the apps and wires them
together. Views receive them via Provide["name"]; tests can override any provider.
"""

from dependency_injector import containers, providers

from access.services import AccessService, SecurityAlertService
from customers.services import CustomerService
from orders.payments import SimulatedPaymentService
from orders.services import OrderService
from stadiums.services import MatchCatalogService
from stadiums.statistics import StatisticsService
from tickets.services import TicketService


class Container(containers.DeclarativeContainer):
    config = providers.Configuration()

    # adapters of the core.interfaces Protocols
    match_catalog = providers.Singleton(MatchCatalogService)
    statistics_service = providers.Singleton(StatisticsService)
    customer_service = providers.Singleton(CustomerService)
    payment_service = providers.Singleton(SimulatedPaymentService)
    alert_service = providers.Singleton(SecurityAlertService)
    ticket_service = providers.Singleton(TicketService, catalog=match_catalog)

    # use case services
    order_service = providers.Singleton(
        OrderService, catalog=match_catalog, tickets=ticket_service, customers=customer_service,
        payments=payment_service, statistics=statistics_service,
    )
    access_service = providers.Singleton(
        AccessService, catalog=match_catalog, tickets=ticket_service, statistics=statistics_service,
        alerts=alert_service,
    )
