# A multi-stage build docker file
# https://docs.docker.com/build/building/multi-stage/
#
# the first) stage is used to build the application
# the main advantage is, that NO build-tools need to be available locally
# everything is done "inside" the image
#
# there are a couple of images available: https://hub.docker.com/_/python/

FROM python:3.14-slim AS builder
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

RUN apt-get update && apt-get install -y python3-dev default-libmysqlclient-dev build-essential pkg-config 

# Change the working directory to the `app` directory
WORKDIR /app

# Enable bytecode compilation
ENV UV_COMPILE_BYTECODE=1
# Copy from the cache instead of linking since it's a mounted volume
ENV UV_LINK_MODE=copy

# Install dependencies
RUN --mount=type=cache,target=/root/.cache/uv \
    --mount=type=bind,source=uv.lock,target=uv.lock \
    --mount=type=bind,source=pyproject.toml,target=pyproject.toml \
    uv sync --locked --no-install-project --no-editable

# Copy the project into the intermediate image
ADD . /app

# Sync the project
RUN --mount=type=cache,target=/root/.cache/uv \
    uv sync --locked --no-editable --no-dev --group prod

# ---------------------------------------------------------------------------
# the second) stage is used for runtime/production
# we have now a different base-image which is smaller for production
FROM python:3.14-slim

WORKDIR /opt/django-app

ENV PATH="/opt/django-app/.venv/bin:$PATH"

# select the production settings; they extend settings.py and read the secrets from the environment
ENV DJANGO_SETTINGS_MODULE=swd_django_demo.settings_prod

# still we needs some additional stuff
# we need mariadb library (very, very similar to mysql) to access the db
# this is the runtime library dependency of mysqlclient
RUN apt-get update && apt-get install -y default-libmysqlclient-dev curl

RUN mkdir -p /opt/django-app

# it is a best practise to create a seperate user and not execute everything with root
RUN useradd -d /opt/django-app django-app-user

RUN chown django-app-user /opt/django-app

USER django-app-user

# copy our project to the application directory
COPY --chown=django-app-user . /opt/django-app

# Copy the environment, but not the source code
COPY --from=builder --chown=django-app-user /app/.venv /opt/django-app/.venv

COPY --chown=django-app-user ./swd_django_demo/run_gunicorn.py /opt/django-app/run_gunicorn.py
RUN rm /opt/django-app/swd_django_demo/run_gunicorn.py 

# need the entry point as well
COPY --from=builder --chown=django-app-user /app/containers/django-app/entrypoint.sh /opt/django-app/

# defensive: strip carriage returns and (re-)set the executable bit.
# a checkout on Windows may deliver the script with CRLF line endings, the `#!/bin/sh\r`
# shebang then fails at runtime with the misleading "no such file or directory".
# the repository .gitattributes prevents this, this is the second line of defense.
RUN sed -i 's/\r$//' /opt/django-app/entrypoint.sh \
    && chmod +x /opt/django-app/entrypoint.sh

# Set environment variables to optimize Python
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1 

EXPOSE 8000

ENTRYPOINT ["/opt/django-app/entrypoint.sh"]
