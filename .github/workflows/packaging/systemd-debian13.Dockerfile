FROM debian:13
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        systemd systemd-sysv dbus ca-certificates \
        zip sudo bash-completion \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Configure systemd
RUN systemctl set-default multi-user.target

CMD ["/lib/systemd/systemd"]
