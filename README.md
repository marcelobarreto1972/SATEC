# mqtt-neon

Micro E2 stack: **Certbot (Let's Encrypt / DuckDNS)** + **Mosquitto (TLS :8883)** + **Python subscriber → NeonDB**

```
sensors/dht22  →  Mosquitto :8883 (LE TLS)  →  subscriber.py  →  NeonDB
                       ↑
              certbot (DNS-01 via DuckDNS, auto-renews)
```

---

## First-time setup on the Micro E2

### 1. Open port 8883

In your cloud provider's firewall / security group, allow inbound TCP **8883**.

> Port 80 is NOT needed — we use the **DNS-01** challenge, so no HTTP server is required.

### 2. Point DuckDNS to your E2's IP

Log into https://duckdns.org and make sure `satec5.duckdns.org` points to your E2's public IP. The certbot container will call the DuckDNS API automatically to satisfy the DNS-01 challenge.

### 3. Clone the repo & create `.env`

```bash
git clone https://github.com/<you>/SATEC
cd ~/SATEC
cp .env.example .env
nano .env          # fill in DUCKDNS_TOKEN, CERTBOT_EMAIL, NEON_DSN
```

### 4. Start the stack

```bash
docker compose up -d
```

The **certbot** container starts first. On the very first run it requests a certificate from Let's Encrypt (takes ~60–90 s for DNS propagation). Once the cert is present, **mosquitto** starts automatically. After that, certbot sleeps 24 h and then checks for renewal every day.

### 5. Watch logs

```bash
docker compose logs -f certbot    # issuance / renewal progress
docker compose logs -f mosquitto  # TLS listener status
docker compose logs -f subscriber # DB writes
```

---

## How cert renewal works

1. The `certbot` container runs a loop (default: every 24 h).
2. `certbot renew` is a no-op until < 30 days remain on the cert.
3. When it does renew, the entrypoint runs `docker kill --signal HUP mosquitto`.
4. Mosquitto reloads its TLS config without dropping connections.

No human intervention needed.

---

## NeonDB table (auto-created on first run)

```sql
CREATE TABLE sensor_readings (
    id          BIGSERIAL PRIMARY KEY,
    device_ts   TIMESTAMPTZ NOT NULL,
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    temp        NUMERIC(6, 2) NOT NULL,
    humidity    NUMERIC(6, 2) NOT NULL
);
```
---

## GitHub Actions secrets

| Secret           | Value                                              |
|------------------|----------------------------------------------------|
| `E2_HOST`        | IP / hostname of your Micro E2                     |
| `E2_USER`        | SSH user (`ubuntu`)                                |
| `E2_SSH_KEY`     | Private SSH key for that user                      |
| `NEON_DSN`       | Full NeonDB connection string                      |
| `DUCKDNS_TOKEN`  | Your DuckDNS token                                 |
| `CERTBOT_EMAIL`  | Email for Let's Encrypt expiry notices             |

Every push to `main` → builds `subscriber` + `certbot` images → pushes to GHCR → SSHes into E2 → rolling restart of `certbot` and `subscriber` only (Mosquitto stays up and gets SIGHUP from certbot when certs change).