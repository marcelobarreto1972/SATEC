"""
subscriber.py
Subscribes to sensors/dht22 over TLS MQTT and persists readings to NeonDB.

Expected message JSON:
    {"temp": 23.3, "humidity": 75.3, "timestamp": "2026-04-24T05:40:43Z"}

Environment variables (all required unless noted):
    MQTT_HOST         — hostname of the Mosquitto container  (default: mosquitto)
    MQTT_PORT         — TLS port                             (default: 8883)
    MQTT_TOPIC        — topic to subscribe to               (default: sensors/dht22)
    MQTT_CLIENT_ID    — unique client id                     (default: subscriber-01)
    MQTT_CA_CERT      — path to CA certificate
    MQTT_CLIENT_CERT  — path to client certificate
    MQTT_CLIENT_KEY   — path to client private key
    NEON_DSN          — NeonDB connection string
                        postgres://user:password@host/dbname?sslmode=require
"""

from __future__ import annotations

import json
import logging
import os
import signal
import sys
from datetime import datetime

import psycopg
import paho.mqtt.client as mqtt

# ── Logging ───────────────────────────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
)
log = logging.getLogger("subscriber")

# ── Config from environment ───────────────────────────────────────────────────
MQTT_HOST        = os.environ.get("MQTT_HOST",        "mosquitto")
MQTT_PORT        = int(os.environ.get("MQTT_PORT",    "8883"))
MQTT_TOPIC       = os.environ.get("MQTT_TOPIC",       "sensors/dht22")
MQTT_CLIENT_ID   = os.environ.get("MQTT_CLIENT_ID",   "subscriber-01")
MQTT_CA_CERT     = os.environ.get("MQTT_CA_CERT",     "/certs/ca.crt")
MQTT_CLIENT_CERT = os.environ.get("MQTT_CLIENT_CERT", "/certs/client.crt")
MQTT_CLIENT_KEY  = os.environ.get("MQTT_CLIENT_KEY",  "/certs/client.key")
NEON_DSN         = os.environ["NEON_DSN"]   # mandatory — fail fast if missing

# ── Database ──────────────────────────────────────────────────────────────────
DDL = """
    CREATE TABLE IF NOT EXISTS sensor_readings (
        id          BIGSERIAL PRIMARY KEY,
        device_ts   TIMESTAMPTZ NOT NULL,
        received_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
        temp        NUMERIC(6, 2) NOT NULL,
        humidity    NUMERIC(6, 2) NOT NULL
    );

    CREATE INDEX IF NOT EXISTS idx_sensor_readings_device_ts
        ON sensor_readings (device_ts DESC);
"""

INSERT = """
    INSERT INTO sensor_readings (device_ts, temp, humidity)
    VALUES (%(device_ts)s, %(temp)s, %(humidity)s)
"""


def get_db() -> psycopg.Connection:
    """Open (or re-open) a connection to NeonDB."""
    log.info("Connecting to NeonDB …")
    conn = psycopg.connect(NEON_DSN, autocommit=False)
    with conn.cursor() as cur:
        cur.execute(DDL)
    conn.commit()
    log.info("NeonDB ready.")
    return conn


def ensure_db(conn: psycopg.Connection | None) -> psycopg.Connection:
    """Return a healthy connection, reconnecting if necessary."""
    if conn is None or conn.closed:
        return get_db()
    try:
        conn.execute("SELECT 1")
        return conn
    except Exception:
        log.warning("DB connection lost — reconnecting …")
        try:
            conn.close()
        except Exception:
            pass
        return get_db()


# ── MQTT callbacks ────────────────────────────────────────────────────────────
_db_conn: psycopg.Connection | None = None


def on_connect(client: mqtt.Client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        log.info("Connected to Mosquitto broker at %s:%s", MQTT_HOST, MQTT_PORT)
        client.subscribe(MQTT_TOPIC, qos=1)
        log.info("Subscribed to topic: %s", MQTT_TOPIC)
    else:
        log.error("Connection refused — reason code %s", reason_code)


def on_message(client: mqtt.Client, userdata, msg: mqtt.MQTTMessage):
    global _db_conn

    raw = msg.payload.decode("utf-8")
    log.debug("Received on %s: %s", msg.topic, raw)

    # ── Parse ──────────────────────────────────────────────────────────────
    try:
        data = json.loads(raw)
        temp     = float(data["temp"])
        humidity = float(data["humidity"])
        device_ts = datetime.fromisoformat(
            data["timestamp"].replace("Z", "+00:00")
        )
    except (json.JSONDecodeError, KeyError, ValueError) as exc:
        log.warning("Malformed payload, skipping: %s — %s", raw, exc)
        return

    # ── Persist ────────────────────────────────────────────────────────────
    try:
        _db_conn = ensure_db(_db_conn)
        with _db_conn.cursor() as cur:
            cur.execute(INSERT, {
                "device_ts": device_ts,
                "temp":      temp,
                "humidity":  humidity,
            })
        _db_conn.commit()
        log.info(
            "Saved  temp=%.1f°C  humidity=%.1f%%  device_ts=%s",
            temp, humidity, device_ts.isoformat(),
        )
    except Exception as exc:
        log.error("DB write failed: %s", exc)
        try:
            _db_conn.rollback()
        except Exception:
            pass
        _db_conn = None   # force reconnect on next message


def on_disconnect(client, userdata, flags, reason_code, properties):
    log.warning("Disconnected from broker (reason=%s) — will auto-reconnect", reason_code)


# ── Main ──────────────────────────────────────────────────────────────────────
def main() -> None:
    global _db_conn
    _db_conn = get_db()

    client = mqtt.Client(
        client_id=MQTT_CLIENT_ID,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    )

    # TLS — client cert is optional (set to empty string to disable mTLS)
    client.tls_set(
        ca_certs=MQTT_CA_CERT or None,
        certfile=MQTT_CLIENT_CERT or None,
        keyfile=MQTT_CLIENT_KEY or None,
    )

    client.on_connect    = on_connect
    client.on_message    = on_message
    client.on_disconnect = on_disconnect

    # Graceful shutdown
    def _shutdown(signum, frame):
        log.info("Signal %s received — shutting down …", signum)
        client.disconnect()
        client.loop_stop()
        if _db_conn and not _db_conn.closed:
            _db_conn.close()
        sys.exit(0)

    signal.signal(signal.SIGTERM, _shutdown)
    signal.signal(signal.SIGINT,  _shutdown)

    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_forever(retry_first_connection=True)


if __name__ == "__main__":
    main()