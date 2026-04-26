#!/bin/sh
# entrypoint.sh — runs inside the certbot container
# 1. Writes the DuckDNS credentials file
# 2. Issues the cert if it doesn't exist yet
# 3. Loops forever, renewing when < 30 days remain and reloading Mosquitto
set -eu

DOMAIN="${DOMAIN:-satec5.duckdns.org}"
EMAIL="${EMAIL:-}"
RENEW_INTERVAL="${RENEW_INTERVAL:-86400}"
CREDENTIALS_FILE="/etc/letsencrypt/duckdns.ini"
LIVE_DIR="/etc/letsencrypt/live/${DOMAIN}"

# ── Write DuckDNS credentials ─────────────────────────────────────────────────
mkdir -p /etc/letsencrypt
cat > "${CREDENTIALS_FILE}" <<EOF
dns_duckdns_token = ${DUCKDNS_TOKEN}
EOF
chmod 600 "${CREDENTIALS_FILE}"

# ── Helper: fix cert permissions so Mosquitto (non-root) can read them ────────
# Let's Encrypt creates live/ and archive/ as chmod 700 (root only).
# Mosquitto runs as uid 1883 and cannot read them without this fix.
fix_permissions() {
    echo "[certbot] Fixing certificate permissions ..."
    chmod 755 /etc/letsencrypt/live/ 2>/dev/null || true
    chmod 755 "${LIVE_DIR}" 2>/dev/null || true
    chmod 644 "${LIVE_DIR}/chain.pem" 2>/dev/null || true
    chmod 644 "${LIVE_DIR}/fullchain.pem" 2>/dev/null || true
    chmod 644 "${LIVE_DIR}/privkey.pem" 2>/dev/null || true
    chmod -R 755 /etc/letsencrypt/archive/ 2>/dev/null || true
    find "/etc/letsencrypt/archive/${DOMAIN}" -type f -exec chmod 644 {} + 2>/dev/null || true
    echo "[certbot] Permissions fixed."
}

# ── Helper: reload Mosquitto without downtime ─────────────────────────────────
reload_mosquitto() {
    echo "[certbot] Sending SIGHUP to mosquitto container to reload TLS certs ..."
    docker kill --signal HUP mosquitto 2>/dev/null || \
        echo "[certbot] Warning: could not signal mosquitto (not running yet?)"
}

# ── Obtain cert if not already present ───────────────────────────────────────
if [ ! -f "${LIVE_DIR}/fullchain.pem" ]; then
    echo "[certbot] No cert found — requesting from Let's Encrypt ..."

    EMAIL_FLAG=""
    if [ -n "${EMAIL}" ]; then
        EMAIL_FLAG="--email ${EMAIL}"
    else
        EMAIL_FLAG="--register-unsafely-without-email"
    fi

    certbot certonly \
        --non-interactive \
        --agree-tos \
        ${EMAIL_FLAG} \
        --authenticator dns-duckdns \
        --dns-duckdns-credentials "${CREDENTIALS_FILE}" \
        --dns-duckdns-propagation-seconds 60 \
        -d "${DOMAIN}" \
        --cert-name "${DOMAIN}"

    echo "[certbot] Certificate issued successfully."
    fix_permissions
    reload_mosquitto
else
    echo "[certbot] Certificate already present at ${LIVE_DIR} — skipping issuance."
    fix_permissions
fi

# ── Renewal loop ──────────────────────────────────────────────────────────────
echo "[certbot] Entering renewal loop (check interval: ${RENEW_INTERVAL}s) ..."
while true; do
    sleep "${RENEW_INTERVAL}"

    echo "[certbot] Running renewal check ..."
    if certbot renew \
        --authenticator dns-duckdns \
        --dns-duckdns-credentials "${CREDENTIALS_FILE}" \
        --dns-duckdns-propagation-seconds 60 \
        --deploy-hook "echo '[certbot] Cert renewed — reloading Mosquitto ...'" \
        --quiet; then

        EXPIRY=$(openssl x509 -enddate -noout \
                   -in "${LIVE_DIR}/fullchain.pem" 2>/dev/null \
                 | cut -d= -f2)
        echo "[certbot] Certificate valid until: ${EXPIRY}"

        fix_permissions
        if certbot certificates 2>/dev/null | grep -q "RENEWED"; then
            reload_mosquitto
        fi
    else
        echo "[certbot] Renewal check finished (no renewal needed or error — see logs)."
    fi
done