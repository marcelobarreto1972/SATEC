FROM certbot/certbot:latest

# DuckDNS DNS-01 authenticator plugin
RUN pip install --no-cache-dir certbot-dns-duckdns

# Docker CLI — used by the entrypoint to send SIGHUP to Mosquitto after renewal
RUN apk add --no-cache docker-cli

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]