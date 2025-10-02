ARG BASE_IMAGE=ubuntu:22.04
FROM ${BASE_IMAGE}
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        systemd systemd-sysv dbus ca-certificates \
        zip sudo bash-completion nano curl file \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Configure systemd
RUN systemctl set-default multi-user.target

CMD ["/lib/systemd/systemd"]
