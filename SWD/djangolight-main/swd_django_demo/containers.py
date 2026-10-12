"""
In this file, a Container class is defined that uses dependency_injector to manage the dependencies of the application.
The container declares the adapters at the seams: product_service, customer_service and order_service are
singletons that provide instances of ProductService, CustomerService and OrderService, respectively.
"""

from dependency_injector import containers, providers

from customers.services import CustomerService
from orders.services import OrderService
from products.services import ProductService


# define a container class and declare its dependencies
class Container(containers.DeclarativeContainer):
    # configuration provider
    config = providers.Configuration()

    # Singleton provider for ProductService
    product_service = providers.Singleton(
        ProductService,
    )

    customer_service = providers.Singleton(
        CustomerService,
    )

    # Singleton provider for OrderService with product_service as a dependency
    order_service = providers.Singleton(
        OrderService, product_service=product_service, customer_service=customer_service
    )
